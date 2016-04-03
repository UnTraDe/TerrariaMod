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

char pattern[] = { 0x8B, 0x86, 0x18, 0x03, 0x00, 0x00, 0x8B, 0x95, 0xE0, 0xFD, 0xFF, 0xFF };
void* instruction = 0;
unsigned int playerBase = 0;
char backup[6];

std::mutex m;
std::condition_variable cv;
bool ready = false;
std::atomic<bool> running = true;

void Signal()
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
	}

	cv.notify_one();
}

bool firstTime = true;
void* jump;

void __declspec(naked) Codecave()
{
	__asm
	{
		mov playerBase, esi
		pushad
	}
	
	//WriteToMemory(instruction, backup, 6);

	if (firstTime)
	{
		firstTime = false;
		jump = (void*)((unsigned int)instruction + 5);
		Signal();
	}
	
	__asm
	{
		popad
		mov eax, [esi + 0x00000318]
		jmp [jump]
	}
}

void Initialize()
{
	void* result = ScanPattern(0, 0xffffffff, pattern, sizeof(pattern)); // TODO make this faster

	if (result == NULL)
	{	
		MessageBox(NULL, "ERROR: Pattern not found. Are you playing a different version?", modName, MB_OK);
		return;
	}
	
	unsigned int codeCave = (unsigned int)Codecave;
	unsigned int relJump = codeCave - ((unsigned int)result + 5); // -5 because E9 jmp instruction takes 5 bytes. relative jmp (E9) is from the NEXT instruction
	instruction = result;

	char jmp[6] = { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90 }; // JMP 00 00 00  NOP
	memcpy(&jmp[1], &relJump, 4);

	memcpy(backup, result, 6);

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

	SuspendThread(mainThreadHandle);
	
	WriteToMemory(result, jmp, 6);

	ResumeThread(mainThreadHandle);
	CloseHandle(mainThreadHandle);

	std::unique_lock<std::mutex> lk(m);
	cv.wait(lk, [] { return ready; });

	MessageBox(NULL, "Ready!", modName, MB_OK);

	while (running)
	{
		if (GetAsyncKeyState(VK_F2))
		{
			//MessageBox(NULL, std::to_string(playerBase).c_str(), "playerBase@", MB_OK);
			/*
			float* velocityX = (float*)(playerBase + 0x2C);
			float* velocityY = (float*)(playerBase + 0x30);

			*velocityX = 50;
			*velocityY = -50;
			*/

			void* inventoryArray = (void*)*(unsigned int*)(playerBase + 0xAC);
			unsigned int inventorySize = *((int*)((unsigned int)inventoryArray + 0x4));			
			void* firstElementPointer = (void*)((unsigned int)inventoryArray + 0x8);
			
			for (int i = 0; i < inventorySize; i++)
			{
				void* itemPointer = (void*)((unsigned int)firstElementPointer + i * 4);
				void* item = (void*)*((unsigned int*)itemPointer);
				char* autoReuse = (char*)((unsigned int)item + 0x12E);
				*autoReuse = 1;
			}
			
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100)); // maybe we can sleep for longer?
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
