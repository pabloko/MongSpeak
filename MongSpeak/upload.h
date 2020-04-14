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
		curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1/upload");
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
		} else {
			//curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
			//curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
			//fprintf(stdout, "Speed: %f bytes/sec during %f seconds\n", speed_upload, total_time);
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
