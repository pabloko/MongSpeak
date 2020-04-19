#pragma once
using namespace Gdiplus;
extern WebWindow* g_webWindow;
//Helper to get encoder's CLSID interface for gdi+
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	ImageCodecInfo* pImageCodecInfo = NULL;
	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  
	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1; 
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  
		}
	}
	free(pImageCodecInfo);
	return 0;
}
//Handle a clipboard event
BOOL mm_getclipboardimage() {
	//OpenCipboard grant access to data
	OpenClipboard(g_webWindow->hWndWebWindow);
	ReleaseClipboard release_OpenClipboard;
	//Check if its a image
	if (!IsClipboardFormatAvailable(CF_BITMAP)) {
		//Check if its a copied file
		if (IsClipboardFormatAvailable(CF_HDROP)) {
			HGLOBAL hGlobal = (HGLOBAL)GetClipboardData(CF_HDROP);
			if (hGlobal) {
				HDROP hDrop = (HDROP)GlobalLock(hGlobal);
				if (hDrop) {
					//Pipe to DROPFILES handler
					mm_drop_handler(hDrop);
					GlobalUnlock(hDrop);
				}
			}
			return TRUE;
		}
		return FALSE; //Not supported format
	}
	//Obtain bitmap handle
	HBITMAP handle = (HBITMAP)GetClipboardData(CF_BITMAP);
	if (handle) {
		Bitmap* bmp = Bitmap::FromHBITMAP(handle, NULL);
		if (bmp) {
			//Encode bitmap to JPEG 75%
			CLSID clsid;
			GetEncoderClsid(L"image/jpeg", &clsid);
			EncoderParameters encoderParameters;
			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].Guid = EncoderQuality;
			encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1;
			ULONG quality = 75;
			encoderParameters.Parameter[0].Value = &quality;
			//Create a clipboard.jpg in temp dir
			wchar_t szTmpPath[MAX_PATH];
			GetTempPath(MAX_PATH, szTmpPath);
			wstring file = wstring_format(L"%s\\clipboard.jpg", szTmpPath);
			//Save the file
			bmp->Save(file.c_str() , &clsid, &encoderParameters);
			//Start the upload
			CreateThread(NULL, NULL, FileUpload, _com_util::ConvertBSTRToString((BSTR)file.c_str()), NULL, NULL);
		}
	}
	return TRUE;
}