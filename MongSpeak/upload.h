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
	if (nmemb == 0 || nmemb > MAX_PATH) return nmemb;
	vector<uint8_t> pv;
	char szFileUploaded[MAX_PATH];
	char szMessage[MAX_PATH * 5];
	memcpy(szFileUploaded, ptr, nmemb);
	szFileUploaded[nmemb] = '\0';
	int iFileNameStart = 0; 
	for (; iFileNameStart < strlen(szFileUploaded); iFileNameStart++) 
		if (szFileUploaded[iFileNameStart] == '_') break;
	char* szFileUploadedName = &szFileUploaded[iFileNameStart + 1];
	if (strcmp(szFileUploadedName, (char*)stream) == 0) {
		sprintf(szMessage, "<i class=\"fa fa-cloud-download\"></i> <a href=\"http://%s/file/%s\">%s</a>", &g_network->GetServerURL()[5], szFileUploaded, szFileUploadedName);
		char* ret = (char*)_com_util::ConvertStringToBSTR(szMessage);
		pv.assign(&ret[0], &ret[strlen(szMessage) * sizeof(wchar_t)]);
		g_network->Send(RPCID::USER_CHAT, &pv);
	}
	return nmemb;
}

DWORD WINAPI FileUpload(void* szFile) {
	if (g_network->GetServerURL() == NULL)
		return 1;
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
		char szFileName[MAX_PATH];
		char szURL[MAX_PATH];
		sprintf(szFileName, "%s", (char*)szFile);
		int iStrStartPos = strlen(szFileName);
		for (; iStrStartPos > 0; iStrStartPos--) { if (szFileName[iStrStartPos] == ' ') szFileName[iStrStartPos] = '_'; if (szFileName[iStrStartPos] == '\\') break; }
		sprintf(szURL, "http://%s/upload/%s", &g_network->GetServerURL()[5], &szFileName[iStrStartPos + 1]);
		curl_easy_setopt(curl, CURLOPT_URL, szURL);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, fread);
		curl_easy_setopt(curl, CURLOPT_READDATA, fd);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)filesize);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileUploadCb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &szFileName[iStrStartPos + 1]);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			
		} else {
			
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
