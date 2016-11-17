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

char patternCheckAutoSwing[] = { 0x80, 0xB8, 0x32, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x84, 0xE3, 0x00, 0x00, 0x00 };

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

	void* patternCheckAutoSwingLocation = ScanPattern(0, 0xffffffff, patternCheckAutoSwing, sizeof(patternCheckAutoSwing)); // TODO make this faster

	if (patternCheckAutoSwingLocation == NULL)
	{	
		MessageBox(NULL, "ERROR: patternCheckAutoSwingLocation not found. Are you playing a different version?", modName, MB_OK);
		return;
	}

	SuspendThread(mainThreadHandle);
	NopMemory((void*)((unsigned int)patternCheckAutoSwingLocation + 7), 6, nullptr);
	ResumeThread(mainThreadHandle);
	CloseHandle(mainThreadHandle);
	MessageBox(NULL, "Ready!", modName, MB_OK);
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
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE; // relevant only for DLL_PROCESS_ATTACH, ignored otherwise
}
