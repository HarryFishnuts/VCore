
/* INCLUDES */
#include "core.h"
#include <stdio.h>

/* DEFINITIONS */
#define STR_SMALLBUFF	0x80
#define STR_MEDIUMBUFF	0x100
#define STR_LARGEBUFF	0x200
#define NOWINDOW		(HWND)NULL
#define NOSIZE			(SIZE_T)0
#define NOTHING			(DWORD)0



/* MEMORY ZEROING HELPER FUNCTION */
static inline vVOID vintZeroMemory(PBYTE destination, BYTE value, SIZE_T length)
{
	__stosb(destination, value, length);
}



/* ERROR MSGBOX HELPER FUNCTION */
static inline vVOID vintCreateErrorMessage(const char* triedAction, const char* remarks,
	vBOOL terminateAfterEndMessage)
{
	/* initialize message buffer */
	char messageBuffer[STR_MEDIUMBUFF];
	vintZeroMemory(messageBuffer, 0, sizeof(messageBuffer));

	/* check for bad params */
	if (!triedAction)	triedAction = "NOT SPECIFIED";
	if (!remarks)		remarks		= "N/A";

	/* write error message to message buffer in the following format:	*/
	/* VCORE ERROR														*/
	/* ===========														*/
	/* ACTION:  triedAction												*/
	/* REMARKS: remarks													*/
	/* ERROR CODE: GetLastError value									*/
	/* ===========														*/
	/*   [OK]															*/
	sprintf_s(messageBuffer, sizeof(messageBuffer), "ACTION: %s\nREMARKS: %s\n"
		"ERROR CODE: %d", triedAction, remarks, GetLastError());

	/* create messagebox (this will block thread until ok is clicked) */
	MessageBoxA(NOWINDOW, messageBuffer, "VCore Error", MB_OK | MB_ICONERROR);

	/* terminate once done, if parameter specifies to do so */
	if (terminateAfterEndMessage) ExitProcess(ERROR_INVALID_FUNCTION);
}








/* CORE INITIALIZATION FUNCTIONS */
VAPI vBOOL vCoreInitialize(void)
{
	/* check if already initialized */
	if (vCoreIsInitialized()) return FALSE;

	/* zero memory of core object */
	vintZeroMemory(&_vCore, 0, sizeof(_vCore));

	/* set initialization time */
	_vCore.initializationTime = GetTickCount64();

	/* create windows heap object */
	_vCore.heap = HeapCreate(NOTHING, NOSIZE, NOSIZE);
	if (_vCore.heap == NULL) vintCreateErrorMessage("VCore Initialization Heap Creation",
		"This should really not happen.", TRUE);

	/* set state to initialized and return sucess */
	_bittestandset(&_vCore.coreState, 0);
	return TRUE;
}



VAPI vBOOL vCoreIsInitialized(void) { return _bittest(&_vCore.coreState, 0); }



VAPI vVOID vCoreEnsureInitialized(void)
{
	if (!vCoreIsInitialized()) vintCreateErrorMessage(NULL,
		"Someone tried to call a core function before the core was"
		"initialized. Most core functions will check if the core is"
		"initialized before actually running.", TRUE);
}








/* ERROR MESSAGE FUNCTION */
VAPI vVOID vCoreCreateErrorMessage(const char* action, const char* remarks,
	vBOOL terminateAfterMessageClose)
{
	vintCreateErrorMessage(action, remarks, terminateAfterMessageClose);
}







/* VERSION GET FUNCTION */
VAPI vVOID vCoreGetVersion(vPCOREVERSION out)
{
	out->major = VCORE_VERSION_MAJOR;
	out->minor = VCORE_VERSION_MINOR;
	out->patch = VCORE_VERSION_PATCH;
}