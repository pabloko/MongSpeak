// stdafx.h: archivo de inclusión de los archivos de inclusión estándar del sistema
// o archivos de inclusión específicos de un proyecto utilizados frecuentemente,
// pero rara vez modificados
//

#pragma once

#include "targetver.h"
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _SCL_SECURE_NO_WARNINGS
#include "vendor/easywsclient/easywsclient.hpp"
#include "vendor/easywsclient/easywsclient.cpp"
#include "WinReg.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "stdio.h"
#include <windows.h>
#include <time.h>
//#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "ie/webform.h"
#include "ie/webwindow.h"
#include "ie/webformdispatchimpl.h"
#include "ie/jsobject.h"
#include <tchar.h>
#include <map>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <comutil.h>
#include "Shlwapi.h"
#pragma comment(lib, "comsuppw.lib")
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#include <audioclient.h>
#include <mmdeviceapi.h>
#pragma comment( lib, "vendor/libcurl/windows-Win32-v140/lib/libcurl_a.lib" )
#define CURL_STATICLIB
#include "vendor\libcurl\windows-Win32-v140\include\curl\curl.h"
#pragma comment( lib, "advapi32.lib" )
#pragma comment( lib, "avrt.lib" )
#pragma comment( lib, "vendor/opus/opus.lib" )
#pragma comment( lib, "vendor/opusenc/opusenc.lib" )
#include "vendor/opus/opus.h"
#include "vendor/opusenc/opusenc.h"
#define OUTSIDE_SPEEX
#define RANDOM_PREFIX libopusenc
#include "vendor/opusenc/speex_resampler.h"
#define DEFAULT_SAMPLERATE 16000
#define DEFAULT_OPUS_BITRATE 42000
#define OPUS_CHANNELS 2

#include "uirpc.h"
#include "pcm.h"
#include "cleanup.h"
#include "audio_device_helper.h"
#include "CAudioQueue.h"
#include "CAudioDevice.h"
#include "CAudioSession.h"
#include "CAudioSessionMixer.h"
#include "CNetwork.h"
#include "CAudioStream.h"
#include "CPreferences.h"
#include "statics.h"
#include "upload.h"
#include "clipboard_data_helper.h"
#include "javascript_external.h"
