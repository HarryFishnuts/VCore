
/* ========== <ventries.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "ventries.h"
#include <stdio.h>


/* ========== HELPER							==========	*/
void vfFileWriteCompletionCallback(DWORD errorCode, DWORD bytesWritten,
	LPOVERLAPPED olStruct)
{
	if (errorCode == ZERO) return;
	vCoreFatalError(__func__, "Attempted to write entry buffer to"
		" a file but failed.");
}


/* ========== HELPER							==========	*/
static __forceinline void vfWriteEntryBufferToFile(const char* filename)
{
	vPEntryBuffer buffer = &_vcore.entryBuffer;

	/* create new file of name */
	HANDLE fHndl = CreateFileA(filename, (GENERIC_READ | GENERIC_WRITE), NO_FLAGS,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_COMPRESSED, NULL);

	/* write to file asynchronously */
	OVERLAPPED overLapped = { 0 };
	BOOL result = WriteFileEx(fHndl, buffer, sizeof(vEntryBuffer), &overLapped,
		vfFileWriteCompletionCallback);

	if (result == ZERO) vCoreFatalError(__func__, "Attempted to write entry buffer to"
		" a file but failed.");

	/* close file */
	CloseHandle(fHndl);
}

static __forceinline void vfEntryWriteRollover(void)
{
	/* construct name of file to write to */
	vZeroMemory(_vcore.stringBuffer, sizeof(_vcore.stringBuffer));
	sprintf_s(_vcore.stringBuffer, sizeof(_vcore.stringBuffer),
		"%s\\%s%d%s", EVENTLOG_DIR, EVENTLOG_FILENAME, _vcore.entryBuffer.logFileNumber,
		EVENTLOG_FILEEXTENSION);

	/* write to file */
	vfWriteEntryBufferToFile(_vcore.stringBuffer);

	/* update all entrybuffer members */
	_vcore.entryBuffer.diskWriteCount++;
	_vcore.entryBuffer.logFileNumber++;
	_vcore.entryBuffer.logFileNumber %= MAX_ENTRYLOGS_ON_DISK;
	vCoreTime(&_vcore.entryBuffer.lastWriteTime);

	/* clear all entries in memory */
	_vcore.entryBuffer.entriesInMemory = 0;
	vZeroMemory(_vcore.entryBuffer.buffer, 
		sizeof(_vcore.entryBuffer.buffer));
}

static __forceinline void vfEntryWriteUpdate(void)
{
	/* construct name of file to write to */
	vZeroMemory(_vcore.stringBuffer, sizeof(_vcore.stringBuffer));
	sprintf_s(_vcore.stringBuffer, sizeof(_vcore.stringBuffer),
		"%s\\%s%d%s", EVENTLOG_DIR, EVENTLOG_FILENAME, _vcore.entryBuffer.logFileNumber,
		EVENTLOG_FILEEXTENSION);

	/* write to file */
	vfWriteEntryBufferToFile(_vcore.stringBuffer);

	/* update all entrybuffer members */
	_vcore.entryBuffer.diskWriteCount++;
	vCoreTime(&_vcore.entryBuffer.lastWriteTime);
}

static __forceinline void vfAddEntry(vBYTE entryType,
	const char* funcName, const char* remarks)
{
	/* SYNC		*/ vCoreLock();

	vPEntryBuffer buff = &_vcore.entryBuffer;

	/* check if max amount of entries are in memory */
	/* if so, create new file to write logs to		*/
	if (buff->entriesInMemory >= MAX_ENTRIES_IN_MEMORY) vfEntryWriteRollover();

	/* get next entry */
	vPEntry entry = &buff->buffer[buff->entriesInMemory];

	/* setup all info */
	entry->entryNumber = buff->entriesTotal;
	
	vCoreTime(&entry->timeCreated);
	entry->entryType = entryType;
	entry->threadID  = GetCurrentThreadId();
	
	vZeroMemory(entry->function, sizeof(entry->function));
	vMemCopy(entry->function, funcName, strlen(funcName));

	vZeroMemory(entry->remarks, sizeof(entry->remarks));
	vMemCopy(entry->remarks, remarks, strlen(remarks));

	/* update entrybuffer info */
	buff->entriesTotal++;
	buff->entriesInMemory++;

	/* if time threshold has passed, or entrytype is error	*/
	/* update current logfile incase of impending crash		*/
	vTIME currentTime; vCoreTime(&currentTime);
	if (entryType == ENTRY_ERROR || 
		((currentTime - buff->lastWriteTime) >> 0xA) >= ENTRYLOG_DUMP_INTERVAL_SEC)
	{
		vfEntryWriteUpdate();
	}

	/* UNSYNC	*/ vCoreUnlock();
}

/* ========== EVENT LOGGING						==========	*/
VAPI void vLogEvent(const char* funcName, const char* remarks)
{
	vfAddEntry(ENTRY_EVENT, funcName, remarks);
}

VAPI void vLogWarning(const char* funcName, const char* remarks)
{
	vfAddEntry(ENTRY_WARNING, funcName, remarks);
}

VAPI void vLogError(const char* funcName, const char* remarks)
{
	vfAddEntry(ENTRY_ERROR, funcName, remarks);
}


/* ========== FILE I/O							==========	*/
VAPI void vDumpEntryBuffer(void)
{
	/* SYNC		*/ vCoreLock();
	vfEntryWriteUpdate();
	/* UNSYNC	*/ vCoreUnlock();
}

VAPI vBOOL vReadEntryBuffer(const char* fileName, vPEntryBuffer buffer)
{
	/* make sure file exists */
	DWORD result = GetFileAttributesA(fileName);
	if (result == INVALID_FILE_ATTRIBUTES || result & FILE_ATTRIBUTE_DIRECTORY)
		return FALSE;

	/* get file */
	HANDLE fHndl = CreateFileA(fileName, (GENERIC_READ | GENERIC_WRITE), NO_FLAGS,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_COMPRESSED, NULL);
	if (fHndl == NULL) vCoreFatalError(__func__, "Could not open existing file to "
		"read from.");

	/* zero buffer */
	vZeroMemory(buffer, sizeof(vEntryBuffer));

	/* read to buffer */
	result = ReadFile(fHndl, buffer, sizeof(vEntryBuffer), NULL, NULL);
	if (result == NULL) vCoreFatalError(__func__, "Could not read from file.");

	/* close file */
	CloseHandle(fHndl);

	return TRUE;
}

