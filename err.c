
/* ========== <err.c>							==========	*/
/* Bailey Jia-Tao Brown			2022						*/


/* ========== INCLUDES							==========	*/
#include "err.h"


/* ========== INTERNAL FUNCTIONS				==========	*/
static void verrFileWriteCompletionCallback(DWORD dwErrorCode, 
	DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	if (dwErrorCode) vCoreCreateFatalError("File Write Failed");
}

static void verrDumpIfTimeThreshPassed(void)
{
	/* take time diff in msecs and the shr by 10. (div by 1024) */
	/* this gives and approximate number of seconds since the	*/
	/* last log dump, and is compared against the dump interval */
	if (((vCoreGetTime() - _vcore->actionLog.lastDump) >> 0xA) >
		ACTION_LOG_DUMP_INTERVAL_SEC) vDumpLogBuffer();
}

/* ========== INTERNAL FUNCTIONS				==========	*/
static void verrAddActionToBuffer(vEnumActionType type, const char* action, 
	const char* remarks)
{
	/* make action index rollover (circular array) */
	if (_vcore->actionLog.actionIndex >
		MAX_ACTIONS_SAVED_IN_MEMORY) _vcore->actionLog.actionIndex = 0;

	_vcore->actionLog.actionLog[_vcore->actionLog.actionIndex].type = type;
	_vcore->actionLog.actionLog[_vcore->actionLog.actionIndex].timeCreated =
		vCoreGetTime();

	/* copy strings to buffer */
	__movsb(&_vcore->actionLog.actionLog[_vcore->actionLog.actionIndex].action,
		action, strlen(action));
	__movsb(&_vcore->actionLog.actionLog[_vcore->actionLog.actionIndex].remark,
		remarks, strlen(remarks));
}

/* ========== INITIALIZATION FUNCTIONS			==========	*/

VAPI void _vErrInit(const char* logFileName)
{
	__movsb(&_vcore->actionLog.actionWriteFileName, logFileName, strlen(logFileName));
}

VAPI void _vErrTerminate(void)
{
	vDumpLogBuffer();
}


/* ========== LOGGING AND ERROR FUNCTIONS		==========	*/

/* logs meaningful events and should be used sparringly and */
/* absolutely not for debug purposes.						*/
VAPI void vLogAction(const char* action, const char* remarks)
{
	verrAddActionToBuffer(vActionType_ACTION, action, remarks);
	verrDumpIfTimeThreshPassed();
}

/* logs an unexpected issue which may cause issues later in */
/* the processes execution									*/
VAPI void vLogWarning(const char* warning, const char* remarks)
{
	verrAddActionToBuffer(vActionType_WARNING, warning, remarks);
	verrDumpIfTimeThreshPassed();
}

/* logs an error which is expected to end the process.		*/
VAPI void vLogError(const char* error, const char* remarks)
{
	verrAddActionToBuffer(vActionType_ERROR, error, remarks);
	vDumpLogBuffer();
}


/* ========== FILE I/O FUNCTIONS				==========	*/

/* writes the entire log buffer to a file specified in the	*/
/* initialization function.									*/
static OVERLAPPED __overlapped = { 0 };
VAPI void vDumpLogBuffer(void)
{
	vLogAction("LogBuffer Dump", "N/A");

	HANDLE fHandle = CreateFileA(_vcore->actionLog.actionWriteFileName, 
		GENERIC_READ | GENERIC_WRITE, NO_FLAGS, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (fHandle == NO_HANDLE) vCoreCreateFatalError("Could Not Open File");

	_vcore->actionLog.lastDump = vCoreGetTime();

	WriteFileEx(fHandle, &_vcore->actionLog, sizeof(_vcore->actionLog),
		&__overlapped, verrFileWriteCompletionCallback);
	CloseHandle(fHandle);
}