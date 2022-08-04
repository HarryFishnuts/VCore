
/* ========== <vfileio.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vfileio.h"


/* ========== HELPER AND CALLBACK				==========	*/
static void vhReadCompleteCallback(DWORD errorCode, DWORD bytesTransferred,
	LPOVERLAPPED readInstructions)
{
	if (errorCode == NO_ERROR) return;

	vLogErrorFormatted(__func__, "File read failed. Error code: %d.",
		errorCode);
	return;
}

static void vhWriteCompleteCallback(DWORD errorCode, DWORD bytesTransferred,
	LPOVERLAPPED readInstructions)
{
	if (errorCode == NO_ERROR) return;

	vLogErrorFormatted(__func__, "File write failed. Error code: %d.",
		errorCode);
	return;
}


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI HANDLE vFileCreate(const char* fileName)
{
	/* create new file */
	HANDLE fHndl = CreateFileA(fileName, (GENERIC_READ | GENERIC_WRITE),
		NO_FLAGS, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (fHndl == INVALID_HANDLE_VALUE)
	{
		vLogErrorFormatted(__func__, "Unable to open file '%s'. Win32 Error: %d.",
			fileName, GetLastError());
		return INVALID_HANDLE_VALUE;
	}

	vLogInfoFormatted(__func__, "Create file with handle: %p.", fHndl);
	return fHndl;
}

VAPI HANDLE vFileOpen(const char* fileName)
{
	/* open file that already exists */
	HANDLE fHndl = CreateFileA(fileName, (GENERIC_READ | GENERIC_WRITE),
		NO_FLAGS, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (fHndl == INVALID_HANDLE_VALUE)
	{
		vLogErrorFormatted(__func__, "Unable to open file '%s'. Win32 Error: %d.",
			fileName, GetLastError());
		return INVALID_HANDLE_VALUE;
	}

	vLogInfoFormatted(__func__, "Opened file with handle: %p.", fHndl);
	return fHndl;
}

VAPI void   vFileDelete(const char* fileName)
{
	BOOL result = DeleteFileA(fileName);
	if (result == FALSE)
		vLogWarningFormatted(__func__, "Unable to delete file '%s'.", fileName);
}

VAPI void   vFileClose(HANDLE fHndl)
{
	vLogInfoFormatted(__func__, "Closing file with handle: %p.", fHndl);
	BOOL result = CloseHandle(fHndl);
	if (result == FALSE)
	{
		vLogWarning(__func__, "Could not close file handle.");
		return;
	}
}


/* ========== READ AND WRITE					==========	*/
VAPI vBOOL vFileRead(HANDLE fHndl, vUI32 readOffset, vUI32 readAmount,
	vPTR outBuffer)
{
	OVERLAPPED readInstructions;
	vZeroMemory(&readInstructions, sizeof(readInstructions));
	readInstructions.Offset = readOffset;

	BOOL result = ReadFileEx(fHndl, outBuffer, readAmount, &readInstructions,
		vhReadCompleteCallback);
	if (result == ZERO) 
	{
		vLogErrorFormatted(__func__, "File read failed. Win32 Error: %d.",
			GetLastError());
	}
	return result;
}

VAPI vBOOL vFileWrite(HANDLE fHndl, vUI32 writeOffset, vUI32 writeAmount,
	vPTR inBuffer)
{
	OVERLAPPED writeInstructions;
	vZeroMemory(&writeInstructions, sizeof(writeInstructions));
	writeInstructions.Offset = writeOffset;

	BOOL result = WriteFileEx(fHndl, inBuffer, writeAmount, &writeInstructions,
		vhWriteCompleteCallback);
	if (result == ZERO)
	{
		vLogErrorFormatted(__func__, "File write failed. Win32 Error: %d.",
			GetLastError());
	}
		
	return result;
}

VAPI vBOOL vFileReadLocked(HANDLE fHndl, vUI32 readOffset, vUI32 readAmount,
	vPTR outBuffer)
{
	/* SYNC		*/ EnterCriticalSection(&_vcore.fileLock);

	OVERLAPPED readInstructions;
	vZeroMemory(&readInstructions, sizeof(readInstructions));
	readInstructions.Offset = readOffset;

	BOOL result = ReadFile(fHndl, outBuffer, readAmount, NULL, &readInstructions);
	if (result == ZERO)
	{
		vLogErrorFormatted(__func__, "File read failed. Win32 Error: %d.",
			GetLastError());
	}

	/* UNSYNC	*/ LeaveCriticalSection(&_vcore.fileLock);

	return result;
}

VAPI vBOOL vFileWriteLocked(HANDLE fHndl, vUI32 writeOffset, vUI32 writeAmount,
	vPTR inBuffer)
{
	/* SYNC		*/ EnterCriticalSection(&_vcore.fileLock);

	OVERLAPPED writeInstructions;
	vZeroMemory(&writeInstructions, sizeof(writeInstructions));
	writeInstructions.Offset = writeOffset;

	BOOL result = WriteFile(fHndl, inBuffer, writeAmount, NULL, &writeInstructions);
	if (result == ZERO)
	{
		vLogErrorFormatted(__func__, "File write failed. Win32 Error: %d.",
			GetLastError());
	}

	/* UNSYNC	*/ LeaveCriticalSection(&_vcore.fileLock);
}