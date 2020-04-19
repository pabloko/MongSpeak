#pragma once
using namespace winreg;
class CPreferences
{
public:
	CPreferences() {
		pPrefKey = new RegKey(HKEY_CURRENT_USER, L"SOFTWARE\\MongSpeak");
	}
	~CPreferences() {
		pPrefKey->Close();
		delete pPrefKey;
	}
	void SetPreferences(wstring key, wstring val) {
		pPrefKey->SetStringValue(key, val);
	}
	wstring GetPreferences(wstring key) {
		try {
			if (pPrefKey->QueryValueType(key))
				return pPrefKey->GetStringValue(key);
		} catch (...) {}
		return L"";
	}
private:
	RegKey* pPrefKey;
};

