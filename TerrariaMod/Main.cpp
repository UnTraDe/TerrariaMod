#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <Windows.h>
#include "Common.h"
#include "MemoryHelper.h"
#include "Utils.h"

const char* modName = "TerrariaMod";
char pattern[] = { 0x0F, 0x84, 0x1B, 0x01, 0x00, 0x00, 0x8B, 0x85, 0xDC, 0xE6, 0xFF, 0xFF };
char backup[6];
std::atomic<bool> running = true;

void DoStuff()
{
	// TODO move this to assembly inside Codecave?
	/*
	unsigned int selectedItemIndex = *(unsigned int*)(playerBase + 0x25C);
	void* inventoryArray = (void*)*(unsigned int*)(playerBase + 0xAC);
	unsigned int inventorySize = *((int*)((unsigned int)inventoryArray + 0x4));
	void* firstElementPointer = (void*)((unsigned int)inventoryArray + 0x8);
	void* itemPointer = (void*)((unsigned int)firstElementPointer + selectedItemIndex * 4);
	void* item = (void*)*((unsigned int*)itemPointer);
	char* autoReuse = (char*)((unsigned int)item + 0x12E);
	*autoReuse = 1;
	*/
}

void Initialize()
{
	DWORD mainThreadId = GetMainThreadId();

	if (mainThreadId == 0)
	{
		MessageBox(NULL, "ERROR: Main thread not found!", modName, MB_OK);
		return;
	}

	HANDLE mainThreadHandle = OpenThread(THREAD_ALL_ACCESS, false, mainThreadId); // TODO consider using THREAD_SUSPEND_RESUME

	if (mainThreadHandle == NULL)
	{
		MessageBox(NULL, "ERROR: Cannot open the main thread!", modName, MB_OK);
		return;
	}

	void* result = ScanPattern(0, 0xffffffff, pattern, sizeof(pattern)); // TODO make this faster

	if (result == NULL)
	{	
		MessageBox(NULL, "ERROR: Pattern not found. Are you playing a different version?", modName, MB_OK);
		return;
	}

	SuspendThread(mainThreadHandle);

	NopMemory(result, 6, backup);

	ResumeThread(mainThreadHandle);

	CloseHandle(mainThreadHandle);

	MessageBox(NULL, "Ready!", modName, MB_OK);

	while (running)
	{
		if (GetAsyncKeyState(VK_F2))
		{
			MessageBox(NULL, "Sup?", modName, MB_OK);
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100)); // TODO maybe we can sleep for longer?
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH: // DLL loaded into process space
							 // lpvReserved is NULL for dynamic loads and non-NULL for static loads
							 // TODO DANGER!
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Initialize, NULL, 0, NULL);
		break;
	case DLL_PROCESS_DETACH: // DLL unloaded from process space
							 // lpvReserved is NULL if FreeLibrary has been called or the DLL load failed and non-NULL if the process is terminating
		running = false;
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE; // relevant only for DLL_PROCESS_ATTACH, ignored otherwise
}
