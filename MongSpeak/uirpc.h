#pragma once
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr

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
#ifndef UIRPC_INCLUDED
#define UIRPC_INCLUDED 1
using namespace std;
enum RPCID {
	USER_IDENT,
	USER_JOIN,
	USER_LEAVE,
	USER_CHAT,
	OPUS_DATA,
	ROOMS,
	CHANGE_ROOM,
	/*ROOM_ENTER,
	ROOM_LEAVE,
	ROOM_INFO,*/
	UI_COMMAND
};


//uiRPC
//UNSAFE ILLEGAL RPC SYSTEM: architecture relies on unsafe union and illegal access to its members :S
//also rpc handler are hackish and not thread-safe, and string conversion is pure shit for window$ only.
/*typedef  void(rpc_handler_t)(vector<uint8_t>*pv);
rpc_handler_t* pRPCFn[512];

bool rpc_check_size(vector<uint8_t>*vt, int s) {
	return (vt->size() < s);
}

void rpc_join(uint8_t id, void* fn) {
	pRPCFn[id] = (rpc_handler_t*)fn;
}

int rpc_process(vector<uint8_t>* vt) {
	if (rpc_check_size(vt, 1)) return -1;
	uint8_t id = vt->at(0);
	if (pRPCFn[id] != NULL) {
		rpc_handler_t* rpc = pRPCFn[id];
		vt->erase(vt->begin());
		rpc(vt);
		return 1;
	}
	return 0;
}
*/
#define rpc_type_header(type)\
extern void rpc_write_##type(vector<uint8_t>* vt, type ob, int pos); \
extern void rpc_read_##type(vector<uint8_t>* vt, type* out, int pos);


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


/*rpc_type_header(bool)
rpc_type_header(char)
rpc_type_header(byte)
rpc_type_header(uint8_t)
rpc_type_header(short)
rpc_type_header(uint16_t)
rpc_type_header(int)
rpc_type_header(uint32_t)
rpc_type_header(long)
rpc_type_header(ulong_t)
rpc_type_header(float)
rpc_type_header(double)
rpc_type_header(int64_t)
rpc_type_header(uint64_t)*/


int rpc_read_string(vector<uint8_t>* vt, wchar_t* out, int pos, int len = NULL) {
	if (len == NULL) {
		len = 0;
		for (vector<uint8_t>::iterator it = vt->begin() + pos; it != vt->end(); ++it) {
			if (*it == 0x00) break;
			len++;
		}
	}
	char* utf8str = new char[len + 1];
	copy(vt->begin() + pos, vt->begin() + pos + len, utf8str);
	utf8str[len] = '\0';
	int output_size = MultiByteToWideChar(CP_UTF8, NULL, utf8str, len, NULL, NULL);
	if (out != NULL) {
		MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, out, output_size);
		out[output_size] = '\0';
	}
	delete[] utf8str;
	return output_size;
}

void rpc_write_string(vector<uint8_t>* vt, const wchar_t* in, int pos = -1, int len = NULL) {
	if (len == NULL)
		len = lstrlenW(in);
	int output_size = WideCharToMultiByte(CP_UTF8, NULL, in, len, NULL, NULL, NULL, NULL);
	char* wid = new char[output_size + 1];
	WideCharToMultiByte(CP_UTF8, NULL, in, len, wid, output_size, NULL, NULL);
	if (pos == -1)
		vt->insert(vt->end(), &wid[0], &wid[output_size]);
	else
		vt->insert(vt->begin() + pos, &wid[0], &wid[output_size]);
	delete[] wid;
}

#endif

