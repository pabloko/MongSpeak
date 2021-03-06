#pragma once

IMMDeviceEnumerator *pMMDeviceEnumerator = NULL; //static

//Retrive the default output audio device
HRESULT get_default_device(IMMDevice **ppMMDevice, EDataFlow pDevMode) {
	HRESULT hr = S_OK;
	if (pMMDeviceEnumerator == NULL) {
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pMMDeviceEnumerator);
		if (FAILED(hr)) return hr;
	}
	//ReleaseIUnknown release_pMMDeviceEnumerator(pMMDeviceEnumerator);
	hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(pDevMode, eConsole, ppMMDevice);
	if (FAILED(hr)) return hr;
	return hr;
}

//Retrive a list of audio output devices
HRESULT list_devices(EDataFlow pDevMode, wstring* res) {
	HRESULT hr = S_OK;
	//IMMDeviceEnumerator *pMMDeviceEnumerator;
	if (pMMDeviceEnumerator == NULL) {
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pMMDeviceEnumerator);
		if (FAILED(hr)) return hr;
	}
	//ReleaseIUnknown release_pMMDeviceEnumerator(pMMDeviceEnumerator);
	IMMDeviceCollection *pMMDeviceCollection;
	hr = pMMDeviceEnumerator->EnumAudioEndpoints(pDevMode, DEVICE_STATE_ACTIVE, &pMMDeviceCollection);
	if (FAILED(hr)) return hr;
	ReleaseIUnknown release_pMMDeviceCollection(pMMDeviceCollection);
	UINT count;
	hr = pMMDeviceCollection->GetCount(&count);
	if (FAILED(hr)) return hr;
	res->append(wstring_format(L"%d, [\"Default device\"", count));
	for (UINT i = 0; i < count; i++) {
		IMMDevice *pMMDevice;
		hr = pMMDeviceCollection->Item(i, &pMMDevice);
		if (FAILED(hr)) return hr;
		ReleaseIUnknown release_pMMDevice(pMMDevice);
		IPropertyStore *pPropertyStore;
		hr = pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
		if (FAILED(hr)) return hr;
		ReleaseIUnknown release_pPropertyStore(pPropertyStore);
		PROPVARIANT pv;
		PropVariantInit(&pv);
		hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
		if (FAILED(hr)) return hr;
		PropVariantClearCleanup release_pv(&pv);
		if (VT_LPWSTR != pv.vt) return E_UNEXPECTED;
		res->append(wstring_format(L", \"%s\"", pv.pwszVal));
	}
	res->append(L"]");
	return hr;
}

//Retrive output device by its id
HRESULT get_specific_device(UINT32 iDevice, IMMDevice **ppMMDevice, EDataFlow pDevMode) {
	HRESULT hr = S_OK;
	*ppMMDevice = NULL;
	//IMMDeviceEnumerator *pMMDeviceEnumerator;
	if (pMMDeviceEnumerator == NULL) {
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pMMDeviceEnumerator);
		if (FAILED(hr)) return hr;
	}
	//ReleaseIUnknown release_pMMDeviceEnumerator(pMMDeviceEnumerator);
	IMMDeviceCollection *pMMDeviceCollection;
	hr = pMMDeviceEnumerator->EnumAudioEndpoints(pDevMode, DEVICE_STATE_ACTIVE, &pMMDeviceCollection);
	if (FAILED(hr)) return hr;
	ReleaseIUnknown release_pMMDeviceCollection(pMMDeviceCollection);
	UINT count;
	hr = pMMDeviceCollection->GetCount(&count);
	if (FAILED(hr)) return hr;
	if (count <= iDevice) return FWP_E_OUT_OF_BOUNDS;
	hr = pMMDeviceCollection->Item(iDevice, ppMMDevice);
	if (FAILED(hr)) return hr;
	if (NULL == *ppMMDevice) return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	return hr;
}
