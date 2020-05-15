#pragma once
#include <shellapi.h>

extern void set_taskbar_state();
typedef struct {
	FILE* fd;
	wchar_t* filename;
	double size;
	int sent_pc;
	double sent_bt;
	long time_limit;
} upload_metadata_t;

double GetFileSize(const char* fileName) {
	BOOL fOk;
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (NULL == fileName)
		return -1;
	fOk = GetFileAttributesExA(fileName, GetFileExInfoStandard, (void*)&fileInfo);
	if (!fOk)
		return -1;
	return (double)((fileInfo.nFileSizeHigh * (MAXDWORD + 1)) + fileInfo.nFileSizeLow);
}

void NotifyStatus(int status, wstring status_text) {
	g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, status, (wchar_t*)status_text.c_str());
}

size_t FileUploadCb(void *ptr, size_t size, size_t nmemb, /*void**/ upload_metadata_t* stream) {
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
	if (g_network != NULL && strcmp(szFileUploadedName, _com_util::ConvertBSTRToString(stream->filename)) == 0) {
		sprintf(szMessage, "**%s** *(%.1fMb)*\nhttp://%s/file/%s", szFileUploadedName, stream->size / 1000000, &g_network->GetServerURL()[5], szFileUploaded);
		char* ret = (char*)_com_util::ConvertStringToBSTR(szMessage);
		pv.assign(&ret[0], &ret[strlen(szMessage) * sizeof(wchar_t)]);
		g_network->Send(RPCID::USER_CHAT, &pv);
	} else
		NotifyStatus(-11, wstring(L"Something happened while uploading"));
	if (g_Taskbar) {
		g_Taskbar->SetProgressState(g_webWindow->hWndWebWindow, TBPF_NORMAL);
		g_Taskbar->SetProgressValue(g_webWindow->hWndWebWindow, 100, 100);
	}
	return nmemb;
}

size_t  FileReadCb(void*  _Buffer, size_t _ElementSize, size_t _ElementCount, upload_metadata_t*  _Stream) {
	size_t tosend = fread(_Buffer, _ElementSize, _ElementCount/10, _Stream->fd);
	_Stream->sent_bt += tosend * _ElementSize;
	if (g_network)
		g_network->nBytesWritten += tosend * _ElementSize;
	int uploaded_pc = _Stream->sent_bt / _Stream->size * 100;
	if (uploaded_pc > _Stream->sent_pc) {
		_Stream->sent_pc = uploaded_pc;
		if (_Stream->time_limit + 50 < GetTickCount()) {
			_Stream->time_limit = GetTickCount();
			NotifyStatus(-8, wstring_format(L"Uploading: %s (%d%%)", _Stream->filename, _Stream->sent_pc));
			if (g_Taskbar) {
				g_Taskbar->SetProgressState(g_webWindow->hWndWebWindow, TBPF_NORMAL);
				g_Taskbar->SetProgressValue(g_webWindow->hWndWebWindow, _Stream->sent_pc, 100);
			}
		}
	}
	Sleep(5);
	return tosend;
}

static HANDLE hUploadThread = NULL;
static BOOL bPreventSimultaneousUpload = FALSE;

void CancelUploadIfAny() {
	if (bPreventSimultaneousUpload && hUploadThread != NULL) {
		TerminateThread(hUploadThread,0);
		NotifyStatus(-11, wstring(L"Upload cancelled"));
		bPreventSimultaneousUpload = FALSE;
		hUploadThread = NULL;
	}
}

DWORD WINAPI FileUpload(void* szFile) {
	if (bPreventSimultaneousUpload) return ERROR_DS_BUSY;
	if (g_network->GetServerURL() == NULL) return E_NOT_SET;
	bPreventSimultaneousUpload = TRUE;
	BoolToggleCleanup bPreventSimultaneousUpload_cleanup(&bPreventSimultaneousUpload);
	HRESULT hr = S_OK;
	CURL *curl;
	CURLcode res;
	struct stat file_info;
	double speed_upload, total_time;
	FILE *fd;
	fd = fopen((char*)szFile, "rb");
	if (!fd)
		return 1;
	double filesize = GetFileSize((char*)szFile);
	if (filesize <= 0 || filesize > 50000000) {
		NotifyStatus(-10, wstring_format(L"File too big (%.1f Mb/50.0 Mb)", filesize / 1000000));
		return 1;
	}
	curl = curl_easy_init();
	if (curl) {
		char szFileName[MAX_PATH];
		char szURL[MAX_PATH];
		sprintf(szFileName, "%s", (char*)szFile);
		int iStrStartPos = strlen(szFileName);
		for (; iStrStartPos > 0; iStrStartPos--) { if (szFileName[iStrStartPos] == ' ') szFileName[iStrStartPos] = '_'; if (szFileName[iStrStartPos] == '\\') break; }
		upload_metadata_t* meta = (upload_metadata_t*)malloc(sizeof(upload_metadata_t));
		meta->fd = fd;
		meta->filename = _com_util::ConvertStringToBSTR(&szFileName[iStrStartPos + 1]);
		meta->size = filesize;
		meta->sent_pc = 0;
		meta->sent_bt = 0;
		meta->time_limit = GetTickCount();
		sprintf(szURL, "http://%s/upload/%s", &g_network->GetServerURL()[5], &szFileName[iStrStartPos + 1]);
		struct curl_slist *chunk = NULL;
		char* header = new char[500];
		sprintf(header, "user-mong: %s", &hwProfileInfo.szHwProfileGuid[1]);
		chunk = curl_slist_append(chunk, header);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl, CURLOPT_URL, szURL);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, FileReadCb);
		curl_easy_setopt(curl, CURLOPT_READDATA, meta);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)filesize);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileUploadCb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, meta /*&szFileName[iStrStartPos + 1]*/);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		NotifyStatus(-9,wstring_format(L"Uploading: %s", meta->filename));
		Sleep(100);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			hr = E_FAIL;  
			NotifyStatus(-11, wstring(L"Something happened while uploading"));
		} else {
			NotifyStatus(-12, wstring_format(L"Uploading: %s (%.1f Mb)", meta->filename, (double)((double)meta->size / 1000000.0f)));
		}
		Sleep(1000);
		curl_slist_free_all(chunk);
		delete meta;
		delete[] header;
		curl_easy_cleanup(curl);
		if (g_Taskbar) 
			set_taskbar_state();
	}
	fclose(fd);
	return hr;
}

void mm_drop_handler(HDROP drop) {
	if (g_network->Status() != WebSocket::OPEN)
		return;
	wchar_t szFile[MAX_PATH];
	DragQueryFile(drop, 0, szFile, MAX_PATH);
	if (wcsstr(szFile, L"\\INetCache\\") != NULL) 
		return;
	hUploadThread = CreateThread(NULL, NULL, FileUpload, _com_util::ConvertBSTRToString(szFile), NULL, NULL);
}

LRESULT mm_messageFilter(UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_ACTIVATE: {
		g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -19, (short)IsIconic(g_webWindow->hWndWebWindow));
		//g_webWindow->webForm->RunJSFunctionW(wstring_format(L"WindowActiveChanged(%d, %d);", IsIconic(g_webWindow->hWndWebWindow), wParam).c_str());
	} break;
	}
	return false;
}