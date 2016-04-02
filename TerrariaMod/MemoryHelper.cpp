#include "MemoryHelper.h"
#include <Windows.h>
#include <Psapi.h>
#include "Common.h"

void WriteToMemory(void* dst, void* src, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dst, src, size);
	DWORD temp;
	VirtualProtect(dst, size, oldProtect, &temp); // According to the MSDN documentation the function will fail if lpflOldProtect is null
}


// TODO change return type to something else
int FindPattern(void* block, size_t blockSize, void* pattern, size_t patternSize)
{
	for (unsigned int i = 0; i < blockSize - patternSize; i++)
	{
		bool found = true;

		for (unsigned int j = 0; j < patternSize; j++)
			found &= ((char*)pattern)[j] == ((char*)block)[i + j];

		if (found)
			return i;
	}

	return -1;
}

void* ScanPattern(void* blockAddress, size_t blockSize, void* pattern, size_t patternSize)
{
	HMODULE hModule = GetModuleHandle("TerrariaMod.dll");
	MODULEINFO moduleInfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo));
	unsigned int currentAddress = (unsigned int)blockAddress;

	while (currentAddress <= ((unsigned int)blockAddress + blockSize) - patternSize) // -patternSize because pattern can't fit in the end
	{
		MEMORY_BASIC_INFORMATION info;
		VirtualQuery((void*)currentAddress, &info, sizeof(info));

		if (currentAddress >= (unsigned int)moduleInfo.lpBaseOfDll && currentAddress <= ((unsigned int)moduleInfo.lpBaseOfDll + (unsigned int)moduleInfo.SizeOfImage))
		{
#ifdef _DEBUG
			std::string dbg;
			dbg += std::to_string(currentAddress);
			dbg += " is inside TerrairaMod.dll space, skipping to ";
			dbg += std::to_string((unsigned int)moduleInfo.lpBaseOfDll + (unsigned int)moduleInfo.SizeOfImage + 1);
			DebugLog(dbg);
#endif
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
				return (void*)((unsigned int)info.BaseAddress + result);

			/*
			for (unsigned int i = 0; i < info.RegionSize - patternSize; i++)
			{
				bool found = true;

				for (unsigned int j = 0; j < patternSize; j++)
					found &= ((char*)pattern)[j] == buffer[i + j];

				if (found)
				{
					delete buffer;
					return (void*)((unsigned int)info.BaseAddress + i);
				}
			}
			*/
			
		}

		currentAddress += info.RegionSize;
	}

	return NULL;
}