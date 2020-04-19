#pragma once
#include <shellapi.h>

long GetFileSize(const char* fileName) {
	BOOL fOk;
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (NULL == fileName)
		return -1;
	fOk = GetFileAttributesExA(fileName, GetFileExInfoStandard, (void*)&fileInfo);
	if (!fOk)
		return -1;
	return (long)fileInfo.nFileSizeLow;
}

//Request callback with content
size_t FileUploadCb(void *ptr, size_t size, size_t nmemb, void *stream) {
	//Safe limits
	if (nmemb == 0 || nmemb > MAX_PATH) return nmemb;
	vector<uint8_t> pv;
	char szFileUploaded[MAX_PATH];
	char szMessage[MAX_PATH * 5];
	//Read uploaded file name
	memcpy(szFileUploaded, ptr, nmemb);
	szFileUploaded[nmemb] = '\0';
	//Search filename for end of random id added to filename
	int iFileNameStart = 0; 
	for (; iFileNameStart < strlen(szFileUploaded); iFileNameStart++) 
		if (szFileUploaded[iFileNameStart] == '_') break;
	char* szFileUploadedName = &szFileUploaded[iFileNameStart + 1];
	//Compared original name with uploaded name
	if (strcmp(szFileUploadedName, (char*)stream) == 0) {
		//Send chat with link to file
		sprintf(szMessage, "<i class=\"fa fa-cloud-download\"></i> <b>%s</b><br>http://%s/file/%s", szFileUploadedName, &g_network->GetServerURL()[5], szFileUploaded);
		char* ret = (char*)_com_util::ConvertStringToBSTR(szMessage);
		pv.assign(&ret[0], &ret[strlen(szMessage) * sizeof(wchar_t)]);
		g_network->Send(RPCID::USER_CHAT, &pv);
	}
	return nmemb;
}

/*INT NotifyStatus(int status, wstring status_text) {
	g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, status, (wchar_t*)status_text.c_str());
	return 1;
}*/

//Uploader thread
DWORD WINAPI FileUpload(void* szFile) {
	//Not server, not uploading
	if (g_network->GetServerURL() == NULL)
		return 1;
	HRESULT hr = S_OK;
	//Init curl
	CURL *curl;
	CURLcode res;
	struct stat file_info;
	double speed_upload, total_time;
	//Check file size limits
	LONG filesize = GetFileSize((char*)szFile);
	if (filesize <= 0 || filesize > 50000000)
		return 1; // NotifyStatus(-10, wstring_format(L"File too big (%.1f Mb/50.0 Mb)", (double)(filesize / 1000000)));
	FILE *fd;
	//Open file handle
	fd = fopen((char*)szFile, "rb");
	if (!fd)
		return 1;
	curl = curl_easy_init();
	if (curl) {
		//Setup curl PUT request
		//Todo: add some auth pls
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
		//Perform http request
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			hr = 1; // NotifyStatus(-11, wstring(L"Something happened while uploading"));
		} else {
			//Success, but were handling it on writefunction_cb, so we read the output
		}
		curl_easy_cleanup(curl);
	}
	fclose(fd);
	return hr;
}
//Called on WM_DROPFILES event
//Todo: handle more than 1 file
//Todo2: we need to see something on UI while uploading
//Todo3: ConvertBSTRToString seems to use ACP and paths with special characters fail, fix
void mm_drop_handler(HDROP drop) {
	//If not connected to a server, not uploading
	if (g_network->Status() != WebSocket::OPEN)
		return;
	//Get file path
	wchar_t szFile[MAX_PATH];
	DragQueryFile(drop, 0, szFile, MAX_PATH);
	//Fix, IE produces this event if you drag a image inside, dont upload if path is from browser's cache
	if (wcsstr(szFile, L"\\INetCache\\") != NULL) 
		return;
	//Spawn a thread to upload with curl
	CreateThread(NULL, NULL, FileUpload, _com_util::ConvertBSTRToString(szFile), NULL, NULL);
}
