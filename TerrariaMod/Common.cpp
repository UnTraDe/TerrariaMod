#include "Common.h"
#include <Windows.h>

void DebugLog(const std::string& message)
{
#ifdef _DEBUG

	std::string msg("TerrariaMod: ");
	msg.append(message);
	msg.append("\n");
	OutputDebugString(msg.c_str());
#endif
}