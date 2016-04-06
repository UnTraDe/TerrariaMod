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
char patternCheckAutoSwing[] = { 0x0F, 0x84, 0x1B, 0x01, 0x00, 0x00, 0x8B, 0x85, 0xDC, 0xE6, 0xFF, 0xFF };
// only for expert mode
char patternSetRespawnTimer[] = { 0xF2, 0x0F, 0x10, 0x45, 0x84, 0xF2, 0x0F, 0x2C, 0xC0, 0x89, 0x86, 0xC4, 0x02, 0x00, 0x00 };
std::atomic<bool> running = true;

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
	/*
	void* patternSetRespawnTimerLocation = ScanPattern(0, 0xffffffff, patternSetRespawnTimer, sizeof(patternCheckAutoSwing)); // TODO make this faster

	if (patternSetRespawnTimerLocation == NULL)
	{
		MessageBox(NULL, "ERROR: patternSetRespawnTimer not found. Are you playing a different version?", modName, MB_OK);
		return;
	}

	char shorterRespawnTime[] = { 0xC7, 0x86, 0xC4, 0x02, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, // mov [esi+000002C4],000000F0
					0x90, 0x90, 0x90, 0x90, 0x90 }; // fill remaing with nops
	*/
	SuspendThread(mainThreadHandle);
	
	NopMemory(patternCheckAutoSwingLocation, 6, nullptr);
	//WriteToMemory(patternSetRespawnTimerLocation, shorterRespawnTime, sizeof(shorterRespawnTime));

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
