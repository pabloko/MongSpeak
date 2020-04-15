#pragma once
#include <shellapi.h>

long GetFileSize(const char* fileName)
{
	BOOL fOk;
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (NULL == fileName)
		return -1;
	fOk = GetFileAttributesExA(fileName, GetFileExInfoStandard, (void*)&fileInfo);
	if (!fOk)
		return -1;
	return (long)fileInfo.nFileSizeLow;
}

size_t FileUploadCb(void *ptr, size_t size, size_t nmemb, void *stream) {
	printf("RESPONSE(%d): %s\n\n", nmemb,(CHAR*)ptr);
	vector<uint8_t> pv;
	char* ret = (char*)_com_util::ConvertStringToBSTR((const char*)ptr);
	pv.assign(&ret[0], &ret[nmemb * sizeof(wchar_t)]);
	g_network->Send(RPCID::USER_CHAT, &pv);
	return 0;
}

DWORD WINAPI FileUpload(void* szFile) {
	HRESULT hr = S_OK;
	CURL *curl;
	CURLcode res;
	struct stat file_info;
	double speed_upload, total_time;
	FILE *fd;
	fd = fopen((char*)szFile, "rb");
	if (!fd)
		return 1;
	LONG filesize = GetFileSize((char*)szFile);
	if (filesize <= 0)
		return 1;
	curl = curl_easy_init();
	if (curl) {
		char szUrl[MAX_PATH];
		sprintf(szUrl, "%s", (char*)szFile);
		int strend = strlen(szUrl);
		for (; strend > 0; strend--) if (szUrl[strend] == '\\') break;
		sprintf(szUrl, "http://127.0.0.1/upload/%s", &szUrl[strend+1]);

		
		curl_easy_setopt(curl, CURLOPT_URL, szUrl);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, fread);
		curl_easy_setopt(curl, CURLOPT_READDATA, fd);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)filesize);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileUploadCb);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			//fprintf(stdout, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			//AQUI ES CUANDO PASA UN ERROR
		} else {
			//curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
			//curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
			//fprintf(stdout, "Speed: %f bytes/sec during %f seconds\n", speed_upload, total_time);//dis?
			//AQUI ES CUANDO TODO SE SUBIO OK, PERO EL OTRO ES EL READFN, SE LLAMA CUANDO HACES RES.WRITE/END...
		}
		curl_easy_cleanup(curl);
	}
	fclose(fd);
	return hr;
}

void mm_drop_handler(HDROP drop) {
	if (g_network->Status() != WebSocket::OPEN)
		return;
	wchar_t szFile[MAX_PATH];
	DragQueryFile(drop, 0, szFile, MAX_PATH);
	CreateThread(NULL, NULL, FileUpload, _com_util::ConvertBSTRToString(szFile), NULL, NULL);
}
