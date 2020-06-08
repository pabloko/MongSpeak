#pragma once
#include <rtcapi.h>

FILE* LogFile = NULL;

void WriteToLogFile(const char* str, ...) {
	if (LogFile == NULL) {
		SYSTEMTIME time;
		GetLocalTime(&time);
		char filename[MAX_PATH];
		sprintf(filename, "MongCrashDump_%02d_%02d__%02d_%02d_%02d.txt", time.wDay, time.wMonth, time.wHour, time.wMinute, time.wSecond);
		LogFile = fopen(filename, "a");
		if (LogFile == NULL)
			return;
	}
	char buffer[512];
	memset(buffer, 0, 512);
	va_list args;
	va_start(args, str);
	vsprintf_s(buffer, 512, str, args);
	va_end(args);
	fprintf(LogFile, "%s\n", buffer);
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

LONG WINAPI unhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo) {
	WriteToLogFile(" ---------------------------------------------------------------------");
	WriteToLogFile(" Base address: 0x%p Base instance: 0x%p", __ImageBase, HINST_THISCOMPONENT);
	WriteToLogFile(" Exception at address: 0x%p", ExceptionInfo->ExceptionRecord->ExceptionAddress);
	WriteToLogFile(" ----------------------------------------------------------------------");
	int m_ExceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;
	int m_exceptionInfo_0 = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
	int m_exceptionInfo_1 = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
	int m_exceptionInfo_2 = ExceptionInfo->ExceptionRecord->ExceptionInformation[2];
	switch (m_ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:
		WriteToLogFile(" Cause: EXCEPTION_ACCESS_VIOLATION");
		if (m_exceptionInfo_0 == 0) {
			// bad read
			WriteToLogFile(" Attempted to read from: 0x%08x", m_exceptionInfo_1);
		} else if (m_exceptionInfo_0 == 1) {
			// bad write
			WriteToLogFile(" Attempted to write to: 0x%08x", m_exceptionInfo_1);
		} else if (m_exceptionInfo_0 == 8) {
			// user-mode data execution prevention (DEP)
			WriteToLogFile(" Data Execution Prevention (DEP) at: 0x%08x", m_exceptionInfo_1);
		} else {
			// unknown, shouldn't happen
			WriteToLogFile(" Unknown access violation at: 0x%08x", m_exceptionInfo_1);
		}
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		WriteToLogFile(" Cause: EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
		break;
	case EXCEPTION_BREAKPOINT:
		WriteToLogFile(" Cause: EXCEPTION_BREAKPOINT");
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		WriteToLogFile(" Cause: EXCEPTION_DATATYPE_MISALIGNMENT");
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		WriteToLogFile(" Cause: EXCEPTION_FLT_DENORMAL_OPERAND");
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		WriteToLogFile(" Cause: EXCEPTION_FLT_DIVIDE_BY_ZERO");
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		WriteToLogFile(" Cause: EXCEPTION_FLT_INEXACT_RESULT");
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		WriteToLogFile(" Cause: EXCEPTION_FLT_INVALID_OPERATION");
		break;
	case EXCEPTION_FLT_OVERFLOW:
		WriteToLogFile(" Cause: EXCEPTION_FLT_OVERFLOW");
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		WriteToLogFile(" Cause: EXCEPTION_FLT_STACK_CHECK");
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		WriteToLogFile(" Cause: EXCEPTION_FLT_UNDERFLOW");
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		WriteToLogFile(" Cause: EXCEPTION_ILLEGAL_INSTRUCTION");
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		WriteToLogFile(" Cause: EXCEPTION_IN_PAGE_ERROR");
		if (m_exceptionInfo_0 == 0) {
			// bad read
			WriteToLogFile(" Attempted to read from: 0x%08x", m_exceptionInfo_1);
		} else if (m_exceptionInfo_0 == 1) {
			// bad write
			WriteToLogFile(" Attempted to write to: 0x%08x", m_exceptionInfo_1);
		} else if (m_exceptionInfo_0 == 8) {
			// user-mode data execution prevention (DEP)
			WriteToLogFile(" Data Execution Prevention (DEP) at: 0x%08x", m_exceptionInfo_1);
		} else {
			// unknown, shouldn't happen
			WriteToLogFile(" Unknown access violation at: 0x%08x", m_exceptionInfo_1);
		}
		// WriteToLogFile NTSTATUS
		WriteToLogFile(" NTSTATUS: 0x%08x", m_exceptionInfo_2);
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		WriteToLogFile(" Cause: EXCEPTION_INT_DIVIDE_BY_ZERO");
		break;
	case EXCEPTION_INT_OVERFLOW:
		WriteToLogFile(" Cause: EXCEPTION_INT_OVERFLOW");
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		WriteToLogFile(" Cause: EXCEPTION_INVALID_DISPOSITION");
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		WriteToLogFile(" Cause: EXCEPTION_NONCONTINUABLE_EXCEPTION");
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		WriteToLogFile(" Cause: EXCEPTION_PRIV_INSTRUCTION");
		break;
	case EXCEPTION_SINGLE_STEP:
		WriteToLogFile(" Cause: EXCEPTION_SINGLE_STEP");
		break;
	case EXCEPTION_STACK_OVERFLOW:
		WriteToLogFile(" Cause: EXCEPTION_STACK_OVERFLOW");
		break;
	case DBG_CONTROL_C:
		WriteToLogFile(" Cause: DBG_CONTROL_C (WTF!)");
		break;
	default:
		WriteToLogFile(" Cause: %08x", m_ExceptionCode);
	}
	WriteToLogFile(" EAX: 0x%08x || ESI: 0x%08x", ExceptionInfo->ContextRecord->Eax, ExceptionInfo->ContextRecord->Esi);
	WriteToLogFile(" EBX: 0x%08x || EDI: 0x%08x", ExceptionInfo->ContextRecord->Ebx, ExceptionInfo->ContextRecord->Edi);
	WriteToLogFile(" ECX: 0x%08x || EBP: 0x%08x", ExceptionInfo->ContextRecord->Ecx, ExceptionInfo->ContextRecord->Ebp);
	WriteToLogFile(" EDX: 0x%08x || ESP: 0x%08x", ExceptionInfo->ContextRecord->Edx, ExceptionInfo->ContextRecord->Esp);
	WriteToLogFile(" ---------------------------------------------------------------------");
	fflush(LogFile);
	fclose(LogFile);
	return EXCEPTION_CONTINUE_SEARCH;
}

int runtime_check_handler(int errorType, const char* filename, int linenumber, const char* moduleName, const char* format, ...) {
	WriteToLogFile(" ---------------------------------------------------------------------");
	WriteToLogFile(" MongSpeak (%s %s) has crashed.", __DATE__, __TIME__);
	WriteToLogFile(" RUNTIME CHECK: ");
	WriteToLogFile(" Error type %d at %s line %d in %s", errorType, filename, linenumber, moduleName);
	WriteToLogFile(" ---------------------------------------------------------------------");
	fflush(LogFile);
	fclose(LogFile);
	//PostQuitMessage(0);
	//exit(0);
	//exit(1);
	return 1;
}

void RegisterExceptionHandler() {
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	SetUnhandledExceptionFilter(unhandledExceptionFilter);
	_RTC_SetErrorFunc(&runtime_check_handler);
}
