#pragma once
using namespace Gdiplus;
extern WebWindow* g_webWindow;

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

BOOL mm_getclipboardimage() {
	OpenClipboard(g_webWindow->hWndWebWindow);
	ReleaseClipboard release_OpenClipboard;
	if (!IsClipboardFormatAvailable(CF_BITMAP)) return FALSE;
	HDC hdc = GetDC(0);
	HBITMAP handle = (HBITMAP)GetClipboardData(CF_BITMAP);
	if (handle) {
		Bitmap* bmp = Bitmap::FromHBITMAP(handle, NULL);
		if (bmp) {
			CLSID clsid;
			GetEncoderClsid(L"image/jpeg", &clsid);
			EncoderParameters encoderParameters;
			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].Guid = EncoderQuality;
			encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1;
			ULONG quality = 75;
			encoderParameters.Parameter[0].Value = &quality;
			wchar_t szTmpPath[MAX_PATH];
			GetTempPath(MAX_PATH, szTmpPath);
			wstring file = wstring_format(L"%s\\clipboard.jpg", szTmpPath);
			bmp->Save(file.c_str() , &clsid, &encoderParameters);
			CreateThread(NULL, NULL, FileUpload, _com_util::ConvertBSTRToString((BSTR)file.c_str()), NULL, NULL);
		}
	}
	return TRUE;
}