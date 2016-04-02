#include <iostream>
#include <string>
#include <assert.h>
#include <Windows.h>
#include <TlHelp32.h>

DWORD FindProcessIdByName(std::string name, bool caseSensitive);
DWORD FindMainThreadOfProcessId(DWORD processId);

char* dllPath = "C:\\Users\\UnTraDe\\Documents\\Visual Studio 2015\\Projects\\TerrariaMod\\Debug\\TerrariaMod.dll";

int main(int argc, char const *argv[])
{
	std::string procName("terraria.exe");
	DWORD processId = FindProcessIdByName(procName, false);
	assert(processId);
	std::cout << procName << " found: " << processId << std::endl;

	HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
	assert(procHandle != NULL);

	size_t pathLength = strlen(dllPath) + 1; // strlen does not include the null-terminating character
	LPVOID memAddr = VirtualAllocEx(procHandle, NULL, pathLength, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	assert(memAddr);
	bool success = WriteProcessMemory(procHandle, memAddr, dllPath, pathLength, NULL);
	assert(success);

	FARPROC loadLibraryAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	
	success = CreateRemoteThread(
		procHandle,
		NULL, // Use default security attributes
		0, // Let the system choose the stack size
		(LPTHREAD_START_ROUTINE)loadLibraryAddr, // TODO consider calculating the address in the remote thread to be sure
		memAddr, // Pass the parameter to LoadLibraryA
		0, // Run thread immediatley after creation
		NULL // Don't need the created thread id
		);

	assert(success);


	//std::cout << loadLibraryAddr << std::endl;
	//GetModuleInformation(procHandle, )

	CloseHandle(procHandle);

	std::cout << "Done" << std::endl;
	//getchar();

	return 0;
}

DWORD FindProcessIdByName(std::string name, bool caseSensitive)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (caseSensitive)
			{
				if (strcmp(entry.szExeFile, name.c_str()) == 0)
					return entry.th32ProcessID;
			}
			else
			{
				if (stricmp(entry.szExeFile, name.c_str()) == 0)
					return entry.th32ProcessID;
			}
		}
	}

	CloseHandle(snapshot);

	return 0;
}

DWORD FindMainThreadOfProcessId(DWORD processId)
{
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);

		if (Thread32First(h, &te))
		{
			unsigned long long earliest = MAXUINT64;
			DWORD earlistThreadId = 0;

			do
			{
				if (te.th32OwnerProcessID == processId)
				{
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, false, te.th32ThreadID);
					FILETIME fileTimes[4];
					GetThreadTimes(hThread, &fileTimes[0], &fileTimes[1], &fileTimes[2], &fileTimes[3]);
					unsigned long long time = *(unsigned long long*)&fileTimes[0];

					assert(time != earliest);

					if (time < earliest)
					{
						earliest = time;
						earlistThreadId = te.th32ThreadID;
					}

					CloseHandle(hThread);
				}


				te.dwSize = sizeof(te);
			} while (Thread32Next(h, &te));

			return earlistThreadId;
		}

		CloseHandle(h);
	}

	return 0;
}