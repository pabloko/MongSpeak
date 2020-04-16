#pragma once
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	ImageCodecInfo* pImageCodecInfo = NULL;
	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure
	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}
	free(pImageCodecInfo);
	return 0;
}

void GetClipboardImage() {
	if (!IsClipboardFormatAvailable(CF_DIB)) return;

	HDC hdc = GetDC(0);
	HANDLE handle = GetClipboardData(CF_DIB);
	if (handle)
	{
		BITMAPINFO* bmpinfo = (BITMAPINFO*)GlobalLock(handle);
		if (bmpinfo)
		{
			/*int offset = (bmpinfo->bmiHeader.biBitCount > 8) ?
				0 : sizeof(RGBQUAD) * (1 << bmpinfo->bmiHeader.biBitCount);
			const char* bits = (const char*)(bmpinfo)+bmpinfo->bmiHeader.biSize + offset;
			HBITMAP hbitmap = CreateDIBitmap(hdc, &bmpinfo->bmiHeader, CBM_INIT, bits, bmpinfo, DIB_RGB_COLORS);

			//convert to 32 bits format (if it's not already 32bit)
			BITMAP bm;
			GetObject(hbitmap, sizeof(bm), &bm);
			int w = bm.bmWidth;
			int h = bm.bmHeight;
			char *bits32 = new char[w*h * 4];

			BITMAPINFOHEADER bmpInfoHeader = { sizeof(BITMAPINFOHEADER), w, h, 1, 32 };
			HDC hdc = GetDC(0);
			GetDIBits(hdc, hbitmap, 0, h, bits32, (BITMAPINFO*)&bmpInfoHeader, DIB_RGB_COLORS);
			ReleaseDC(0, hdc);
			*/
			//use bits32 for whatever purpose...
			GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
			CLSID   encoderClsid;
			Status  stat;


			Gdiplus::Bitmap bitmap(bmpinfo, NULL);
			CLSID clsid;
			GetEncoderClsid(L"image/jpeg", &clsid);
			
			// Setup encoder parameters
			EncoderParameters encoderParameters;
			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].Guid = EncoderQuality;
			encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1;

			// setup compression level
			ULONG quality = 75;
			encoderParameters.Parameter[0].Value = &quality;
			bitmap.Save(L"c:/clipboard.jpg", &clsid, &encoderParameters);

			GdiplusShutdown(gdiplusToken);




			//cleanup
			//delete[]bits32;
			GlobalUnlock(bmpinfo);
		}
	}
}