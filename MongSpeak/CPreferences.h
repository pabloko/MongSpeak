#pragma once
using namespace winreg;
extern RegKey* g_reg;
class CPreferences
{
public:
	CPreferences() {
	
	}

	~CPreferences() {

	}

	void SetPreferences(wstring key, wstring val) {
		g_reg->SetStringValue(key, val);
	}
	wstring GetPreferences(wstring key) {
		try {
			if (g_reg->QueryValueType(key))
				return g_reg->GetStringValue(key);
		}
		catch (const RegException& e){}
		catch (const exception& e){}
		return L"";
	}

private:
	//0=always, >0 vk_key, <0 db de sensibilidad
	//int speak_action = 0;
	//vector<string> ip_history;
};

