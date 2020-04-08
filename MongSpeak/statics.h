#pragma once
using namespace winreg;
CAudioSessionMixer* g_mix;
JSObject* g_jsObject;
WebWindow* g_webWindow;
IMMDevice* g_dev_in;
IMMDevice* g_dev_out;
CAudioDevice* g_audio_in;
CAudioDevice* g_audio_out;
CAudioStream* g_stream;
CNetwork* g_network;
RegKey* g_reg;
HW_PROFILE_INFOA hwProfileInfo;
vector<wstring> g_jsStack;
CPreferences* g_preferences;