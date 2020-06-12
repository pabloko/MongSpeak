#pragma once
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr

using namespace std;

enum RPCID {
	USER_IDENT,
	USER_JOIN,
	USER_LEAVE,
	USER_CHAT,
	OPUS_DATA,
	ROOMS,
	CHANGE_ROOM,
	UI_COMMAND
};

//uiRPC
//UNSAFE ILLEGAL RPC SYSTEM: architecture relies on unsafe union and illegal access to its members :S
//also rpc handler are hackish and not thread-safe, and string conversion is pure shit for window$ only.

#define rpc_type_init(type)\
void rpc_write_##type(vector<uint8_t>* vt, type ob, int pos=-1) { \
	union { \
		type v; \
		char c[sizeof(type)]; \
	} bo; \
	bo.v = ob; \
	if (pos == -1) \
		vt->insert(vt->end(), &bo.c[0], &bo.c[sizeof(type)]); \
	else \
		vt->insert(vt->begin() + pos, &bo.c[0], &bo.c[sizeof(type)]); \
} \
void rpc_read_##type(vector<uint8_t>* vt, type* out, int pos) { \
	union { \
		type v; \
		char c[sizeof(type)]; \
	} un; \
	for(int cx=0;cx<sizeof(type);cx++) { \
		un.c[cx]=vt->at(pos+cx); } \
	out[0]=un.v; \
}

typedef unsigned long ulong_t;
rpc_type_init(bool)
rpc_type_init(char)
rpc_type_init(byte)
rpc_type_init(uint8_t)
rpc_type_init(short)
rpc_type_init(uint16_t)
rpc_type_init(int)
rpc_type_init(uint32_t)
rpc_type_init(long)
rpc_type_init(ulong_t)
rpc_type_init(float)
rpc_type_init(double)
rpc_type_init(int64_t)
rpc_type_init(uint64_t)

std::string string_format(const std::string fmt_str, ...) {
	int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
	std::unique_ptr<char[]> formatted;
	va_list ap;
	while (1) {
		formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
		strcpy(&formatted[0], fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::string(formatted.get());
}
std::wstring wstring_format(const std::wstring fmt_str, ...) {
	int final_n, n = ((int)fmt_str.size()) * 4; /* Reserve two times as much as the length of the fmt_str */
	std::unique_ptr<wchar_t[]> formatted;
	va_list ap;
	while (1) {
		formatted.reset(new wchar_t[n]); /* Wrap the plain char array into the unique_ptr */
		wcscpy(&formatted[0], fmt_str.c_str());
		va_start(ap, fmt_str);
		final_n = vswprintf(&formatted[0], n, fmt_str.c_str(), ap);
		va_end(ap);
		if (final_n < 0 || final_n >= n)
			n += abs(final_n - n + 1);
		else
			break;
	}
	return std::wstring(formatted.get());
}

DWORD getVolumeHash() {
	DWORD serialNum = 1337;
	GetVolumeInformation(L"C:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
	return serialNum;
}
char* cpu_id(void) {
	unsigned long s1 = 0;
	unsigned long s2 = 0;
	unsigned long s3 = 0;
	unsigned long s4 = 0;
	__asm
	{
		mov eax, 00h
		xor edx, edx
		cpuid
		mov s1, edx
		mov s2, eax
	}
	__asm
	{
		mov eax, 01h
		xor ecx, ecx
		xor edx, edx
		cpuid
		mov s3, edx
		mov s4, ecx
	}

	static char buf[36 + 1];
	sprintf_s(buf, "%04X%04X%04X%04X%04X", s1, s2, s3, s4, getVolumeHash());
	return buf;
}

void crypt(char *data, int data_len, char* _v, int _vlen) {
	for (int i = 0; i < data_len; i += 2)
		sprintf_s(&data[i], 36, "%02X", (BYTE)((data[i + 0] ^ _v[(i + 0) % _vlen]) ^ (data[i + 1] + _v[(i + 1) % _vlen])));
}

extern HW_PROFILE_INFOA hwProfileInfo;

void GenerateHWID() {
	if (GetCurrentHwProfileA(&hwProfileInfo) != NULL)
		hwProfileInfo.szHwProfileGuid[strlen(&hwProfileInfo.szHwProfileGuid[1])] = '\0';
	char* wid = cpu_id();
	crypt(&hwProfileInfo.szHwProfileGuid[1], strlen(&hwProfileInfo.szHwProfileGuid[1]) - 1, wid, strlen(wid) - 1);
}
