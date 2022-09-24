
/* ========== <vcorefunctions.c>				==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vcorefunctions.h"
#include <stdio.h>
#include <intrin.h>


/* ========== HELPER							==========	*/
static __forceinline void vhZeroMemory(void* target, size_t length)
{
	if ((length & 0b111) == 0)
	{
		__stosq(target, ZERO, length >> 3);
		return;
	}
	else
	{
		__stosb(target, ZERO, length);
		return;
	}
}

static __forceinline void vhMemCopy(void* destination, void* source, size_t length)
{
	if ((length & 0b111) == 0)
	{
		__movsq(destination, source, length >> 3);
		return;
	}

	__movsb(destination, source, length);
}

static __forceinline vBOOL vhIsInitialized(void)
{
	return _vcore.initialized;
}

/* ========== INITIALIZATION AND TERMINATION	==========	*/
VAPI vBOOL vCoreInitialize(void)
{
	if (vhIsInitialized()) return FALSE;

	/* zero everything */
	vhZeroMemory(&_vcore, sizeof(_vCoreInternals));

	/* initialize sync object */
	InitializeCriticalSection(&_vcore.rwPermission);

	/* initialize file locking object */
	InitializeCriticalSection(&_vcore.fileLock);

	/* initialize heap object */
	_vcore.heap = HeapCreate(NO_FLAGS, HEAP_ALLOCATE_MIN, HEAP_ALLOCATE_MAX);
	
	/* setup all other members */
	_vcore.initialized = TRUE;
	_vcore.initializationTime = GetTickCount64();

	/* create logging directory */
	CreateDirectoryA(ENTRYLOG_DIR, NULL);

	/* delete all logfiles in directory */
	char logNameBuff[BUFF_SMALL];
	for (int i = 0; i < MAX_ENTRYLOGS_ON_DISK; i++)
	{
		vhZeroMemory(logNameBuff, sizeof(logNameBuff));
		sprintf_s(logNameBuff, sizeof(logNameBuff),
			"%s\\%s%d%s", ENTRYLOG_DIR, ENTRYLOG_FILENAME, i,
			ENTRYLOG_FILEEXTENSION);
		DeleteFileA(logNameBuff);
	}

	/* initialize all locks to be unused */
	for (int i = 0; i < MAX_LOCKS; i++)
	{
		*(DWORD*)&_vcore.locks[i] = UNUSED_LOCK;
	}

	/* log startup */
	vLogInfo(__func__, "VCore initialized.");

	return TRUE;
}

VAPI vBOOL vCoreTerminate(void)
{
	if (!vhIsInitialized()) return FALSE;

	/* log shutdown and dump */
	vLogInfo(__func__, "VCore terminating.");
	vDumpEntryBuffer();

	/* sync files and core */
	EnterCriticalSection(&_vcore.rwPermission);
	EnterCriticalSection(&_vcore.fileLock);

	/* remove all locks */
	for (int i = 0; i < MAX_LOCKS; i++)
	{
		/* if lock doesn't exist, skip */
		if (*(DWORD*)&_vcore.locks[i] == UNUSED_LOCK) continue;

		/* lock and delete */
		EnterCriticalSection(&_vcore.locks[i]);
		DeleteCriticalSection(&_vcore.locks[i]);
	}

	/* destroy sync objects */
	DeleteCriticalSection(&_vcore.rwPermission);
	DeleteCriticalSection(&_vcore.fileLock);

	/* destroy heap */
	HeapDestroy(_vcore.heap);

	return TRUE;
}


/* ========== TIME								==========	*/
VAPI void vCoreTime(vPTIME outTime)
{
	*outTime = GetTickCount64() - _vcore.initializationTime;
}

VAPI void vCoreInitializationTime(vPTIME outTime)
{
	*outTime = _vcore.initializationTime;
}


/* ========== CONCURRENCY						==========	*/
VAPI void vCoreLock(void)
{
	EnterCriticalSection(&_vcore.rwPermission);
}

VAPI void vCoreUnlock(void)
{
	LeaveCriticalSection(&_vcore.rwPermission);
}


/* ========== CORE ERROR HANDLING				==========	*/
VAPI void vCoreFatalError(vPCHAR function, vPCHAR remarks)
{
	/* SYNC		*/ vCoreLock();

	/* write error message to stringbuffer */
	vhZeroMemory(_vcore.stringBuffer, sizeof(_vcore.stringBuffer));
	sprintf_s(_vcore.stringBuffer, sizeof(_vcore.stringBuffer),
		"Thread ID: %d\n"
		"Time: %I64d\n"
		"Win32 Error Code: %d\n"
		"Function Failed: %s\n"
		"Remarks: %s\n",
		GetCurrentThreadId(), GetTickCount64(), GetLastError(), 
		function, remarks);

	/* make sure messagebox size is correct */
	SetProcessDPIAware();

	/* create messagebox */
	MessageBoxA(NO_WINDOW, _vcore.stringBuffer, FATAL_ERROR_CAPTION,
		MB_OK | MB_ICONERROR);

	/* UNSYNC	*/ vCoreUnlock();

	ExitProcess(ERROR_INVALID_FUNCTION);
}

VAPI void vCoreCrash(void)
{
	__fastfail(FAST_FAIL_FATAL_APP_EXIT);
}


/* ========== ALLOCATION						==========	*/
VAPI vPTR vAlloc(size_t size)
{
	vPTR block = HeapAlloc(_vcore.heap, NO_FLAGS, size + sizeof(vUI64));

	if (block == NULL) vCoreFatalError(__func__,
		"Could not allocate more memory.");

	_vcore.memoryUseage += size;
	*(vUI64*)block = size;

	return (vPBYTE)block + sizeof(vUI64);
}

VAPI vPTR vAllocZeroed(size_t size)
{
	vPTR block = HeapAlloc(_vcore.heap, NO_FLAGS, size + sizeof(vUI64));

	if (block == NULL) vCoreFatalError(__func__,
		"Could not allocate more memory.");
	vhZeroMemory((vPBYTE)block + sizeof(vUI64), size);

	*(vUI64*)block = size;
	_vcore.memoryUseage += size;

	return (vPBYTE)block + sizeof(vUI64);
}

VAPI void vFree(vPTR ptr)
{
	vPUI16 blockBase = (vPBYTE)ptr - sizeof(vUI64);
	_vcore.memoryUseage -= *blockBase;
	BOOL result = HeapFree(_vcore.heap, NO_FLAGS, blockBase);
	if (result == FALSE) vCoreFatalError(__func__,
		"Could not free memory.");
}

VAPI vUI64 vGetMemoryUseage(void)
{
	return _vcore.memoryUseage;
}


/* ========== MEMORY MANIPULATION				==========	*/
VAPI void vZeroMemory(vPTR block, size_t length)
{
	vhZeroMemory(block, length);
}

VAPI void vMemCopy(vPTR destination, vPTR source, size_t length)
{
	vhMemCopy(destination, source, length);
}

VAPI _vPCoreInternals vGetInternals(void)
{
	return &_vcore;
}
