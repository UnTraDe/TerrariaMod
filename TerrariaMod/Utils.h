#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <Windows.h>

template< typename T >
std::wstring int_to_hex(T i)
{
	std::wostringstream oss;
	oss << L"0x" << std::hex << i;

	return oss.str();
}

DWORD GetProccessMainThreadId(DWORD processId);
DWORD GetMainThreadId();
