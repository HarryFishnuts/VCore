
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

/* Initializes _vCore object by first zeroing memory and then populating	*/
/* all of the members of the struct. Will terminate process if failes		*/
VAPI vBOOL vCoreInitialize(void)
{
	/* check if already initialized */
	if (vCoreIsInitialized()) return FALSE;

	/* zero memory of core object */
	__stosb(&_vCore, 0, sizeof(_vCore));

	/* set initialization time */
	_vCore.initializationTime = GetTickCount64();

	/* create windows heap object */
	_vCore.heap = HeapCreate(NOTHING, NOSIZE, NOSIZE);
	if (_vCore.heap == NULL) vCoreCreateErrorMessage("VCore Initialization Heap Creation",
		"This should really not happen.", TRUE);

	/* set state to initialized and return sucess */
	_bittestandset(&_vCore.coreState, 0);
	return TRUE;
}


/* Returns whether the _vCore object is initialized or not. Will return		*/
/* incorrect values if the coreState member is corrupted or tampered with	*/
VAPI vBOOL vCoreIsInitialized(void) 
{ 
	return _bittest(&_vCore.coreState, 0); 
}


/* Checks if the first bit of the coreState member in the core object is	*/
/* set to 1. If not, will create an error and then terminate the process	*/
VAPI vVOID vCoreEnsureInitialized(void)
{
	if (!vCoreIsInitialized()) vCoreCreateErrorMessage(NULL,
		"Someone tried to call a core function before the core was"
		"initialized. Most core functions will check if the core is"
		"initialized before actually running.", TRUE);
}



/* Creates and error message which specifies the attempted action which		 */
/* failed, any remarks from the dev, and whether to terminate the process	 */
/* once the messagebox is acknowleged. action and remarks params can be NULL */
VAPI vVOID vCoreCreateErrorMessage(const char* triedAction, const char* remarks,
	vBOOL terminateAfterMessageClose)
{
	/* initialize message buffer */
	char messageBuffer[STR_MEDIUMBUFF];
	__stosb(messageBuffer, 0, sizeof(messageBuffer));

	/* check for bad params */
	if (!triedAction)	triedAction = "NOT SPECIFIED";
	if (!remarks)		remarks = "N/A";

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
	if (terminateAfterMessageClose) ExitProcess(ERROR_INVALID_FUNCTION);
}



/* Takes a core version structure and populates it with relevant information */
VAPI vVOID vCoreGetVersion(vPCOREVERSION out)
{
	out->major = VCORE_VERSION_MAJOR;
	out->minor = VCORE_VERSION_MINOR;
	out->patch = VCORE_VERSION_PATCH;
}