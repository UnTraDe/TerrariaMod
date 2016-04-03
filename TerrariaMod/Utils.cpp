#include "Utils.h"
#include <TlHelp32.h>

DWORD GetMainThreadId()
{
	DWORD processId = GetCurrentProcessId();
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

					// TODO is it possible that time == earliset? (i.e 2 threads have the same start time?)

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