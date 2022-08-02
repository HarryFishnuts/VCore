
/* ========== <core.c>							========== */
/* Bailey Jia-Tao Brown			2022					   */


/* ========== INCLUDES							========== */
#include "core.h"
#include <stdio.h>


/* ========== CORE INITIALIZATION FUNCTIONS		========== */

/* initializes library object and all other modules			*/
VAPI vBOOL vCoreInitialize(void)
{
	if (_vcore) return FALSE;
	
	_vMemInit(); /* allow for memory management */
	_vcore = vAllocZeroed(sizeof(vCoreLibrary));
	
	_vErrInit("logfile.txt");	/* allow for error logging			*/
	_vBufferInit();				/* allow for threadsafe buffering	*/

	_vcore->initializeTime = GetTickCount64();

	vLogAction("VCore Initialized", "VCore is ready for use.");
}

/* checks whether the core has been initialized				*/
VAPI vBOOL vCoreIsInitialized(void)
{
	return (_vcore != NULL);
}

/* terminates the library and all other modules				*/
VAPI vBOOL vCoreTerminate(void)
{
	if (!_vcore) return FALSE;
	_vBufferTerminate();	/* disable buffering systems */
	_vErrTerminate();		/* disable logging			 */
	
	vFree(_vcore);
	_vMemTerminate();		/* free all memory			 */

	_vcore = NULL;

	return TRUE;
}


/* ========== TIME FUNCTIONS					 ========== */

/* returns the GetTickCount value for when vCoreInitialize  */
/* was called.												*/
VAPI vTIME vCoreGetTimeInitialized(void)
{
	return _vcore->initializeTime;
}

/* returns the miliseconds passed since core initialization */
VAPI vTIME vCoreGetTime(void)
{
	return GetTickCount64() - _vcore->initializeTime;
}


/* ========== MESSAGE BOX ERROR FUNCTION		 ========== */

/* this should not be used by the user. instead, use the	*/
/* err module of core. this is for very low-level failures	*/
static char* __msgBoxCaption = "VCore Fatal Error";
static char  __msgBoxDesc[BUFF_MEDIUM];
VAPI void vCoreCreateFatalError(const char* description)
{
	__movsb(__msgBoxDesc, description, strlen(description));
	MessageBoxA(NO_WINDOW, __msgBoxDesc, __msgBoxCaption,
		MB_OK | MB_ICONERROR);
	ExitProcess(ERROR_INVALID_FUNCTION);
}
