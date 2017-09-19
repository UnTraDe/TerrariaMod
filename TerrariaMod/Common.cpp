#include "Common.h"
#include <Windows.h>

void DebugLog(const std::wstring& message)
{
	std::wstring msg(L"TerrariaMod: ");
	msg.append(message);
	msg.append(L"\n");
	OutputDebugString(msg.c_str());
}