
/* ========== <vfileio.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vfileio.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI HANDLE vFileCreate(const char* fileName)
{
	/* create new file */
	HANDLE fHndl = CreateFileA(fileName, (GENERIC_READ | GENERIC_WRITE),
		NO_FLAGS, NULL, CREATE_ALWAYS, NO_FLAGS, NULL);
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
		NO_FLAGS, NULL, OPEN_EXISTING, NO_FLAGS, NULL);
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

	BOOL result = ReadFile(fHndl, outBuffer, readAmount, NULL,
		&readInstructions);

	if (result == FALSE) 
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

	BOOL result = WriteFile(fHndl, inBuffer, writeAmount, NULL,
		&writeInstructions);

	if (result == ZERO)
	{
		vLogErrorFormatted(__func__, "File write failed. Win32 Error: %d.",
			GetLastError());
	}
		
	return result;
}


/* ========== FILE INFORMATION					==========	*/
VAPI vBOOL vFileExists(const char* fileName)
{
	DWORD fileAttributes = GetFileAttributesA(fileName);
	return (fileAttributes != INVALID_FILE_ATTRIBUTES) &&
		!(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

VAPI vUI64 vFileSize(HANDLE fHndl)
{
	vUI64 sizeOut = 0;
	BOOL result = GetFileSizeEx(fHndl, &sizeOut);
	if (result == FALSE)
	{
		vLogWarningFormatted(__func__, "Could not get file size. Win32 Error: %d\n",
			GetLastError());
	}

	return sizeOut;
}