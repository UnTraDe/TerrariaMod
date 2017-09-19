#include "MemoryHelper.h"

#include <Windows.h>
#include <Psapi.h>
#include "Common.h"
#include "Utils.h"

void WriteToMemory(void* dst, void* src, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dst, src, size);
	DWORD temp;
	VirtualProtect(dst, size, oldProtect, &temp); // According to the MSDN documentation the function will fail if lpflOldProtect is null
}

void NopMemory(void* dst, size_t size, void* backup)
{
	if(backup != nullptr)
		memcpy(backup, dst, size); // TODO make sure Virtual Protection is ok with reading?
	
	// TODO make sure size < 256
	// TODO probably need to optimize this...
	char nops[256];
	memset(nops, 0x90, sizeof(nops)); 

	WriteToMemory(dst, nops, size);
}

// TODO change return type to something else
int FindPattern(const void* block, size_t blockSize, const void* pattern, size_t patternSize)
{
	for (unsigned int i = 0; i < blockSize - patternSize; i++)
	{
		bool found = true;

		for (unsigned int j = 0; j < patternSize; j++)
		{
			if (((char*)pattern)[j] != ((char*)block)[i + j])
			{
				found = false;
				break;
			}
		}

		if (found)
			return i;
	}

	return -1;
}

void* ScanPattern(void* blockAddress, size_t blockSize, void* pattern, size_t patternSize)
{
	HMODULE hModule = GetModuleHandle(L"TerrariaMod.dll");
	MODULEINFO moduleInfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo));
	unsigned int currentAddress = (unsigned int)blockAddress;
	
	DWORD start = timeGetTime();

	while (currentAddress <= ((unsigned int)blockAddress + blockSize) - patternSize) // -patternSize because pattern can't fit in the end
	{
		MEMORY_BASIC_INFORMATION info;
		VirtualQuery((void*)currentAddress, &info, sizeof(info));

		// skip TerrariaMod.dll address space
		if (currentAddress >= (unsigned int)moduleInfo.lpBaseOfDll && currentAddress <= ((unsigned int)moduleInfo.lpBaseOfDll + (unsigned int)moduleInfo.SizeOfImage))
		{
			std::wstring dbg;
			dbg += int_to_hex(currentAddress);
			dbg += L" is inside TerrairaMod.dll space, skipping to ";
			dbg += int_to_hex((unsigned int)moduleInfo.lpBaseOfDll + (unsigned int)moduleInfo.SizeOfImage + 1);
			DebugLog(dbg);

			currentAddress = (unsigned int)moduleInfo.lpBaseOfDll + (unsigned int)moduleInfo.SizeOfImage + 1;
			continue;
		}

		if (info.State & MEM_COMMIT && !(info.Protect & PAGE_GUARD) && !(info.Protect & PAGE_NOACCESS))
		{
			char* buffer = new char[info.RegionSize];
			memcpy(buffer, info.BaseAddress, info.RegionSize); // TODO consider reading straight from source instead of copy
			int result = FindPattern(buffer, info.RegionSize, pattern, patternSize);
			delete buffer;

			if (result > -1)
			{
				std::wstring dbg;
				dbg += L"Scan finished in ";
				dbg += std::to_wstring(timeGetTime() - start);
				dbg += L" milliseconds";
				DebugLog(dbg);

				return (void*)((unsigned int)info.BaseAddress + result);
			}
				
		}

		if (info.RegionSize >= UINT_MAX - currentAddress)
			break;

		currentAddress += info.RegionSize;
	}

	return NULL;
}