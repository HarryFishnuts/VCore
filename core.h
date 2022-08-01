
/* ========== <core.h>							==========	*/
/* Bailey Jia-Tao Brown			2022						*/
/* All headers are connected to core for fast use.			*/
/* Holds global library object which has all library data.	*/


#ifndef _VCORE_CORE_INCLUDE_
#define _VCORE_CORE_INCLUDE_


/* ========== INCLUDES							==========	*/
#include "internal.h"
#include "mem.h"
#include "err.h"


/* ========== LIBRARY OBJECT					==========	*/
vPCoreLibrary _vcore;


/* ========== CORE INITIALIZATION FUNCTIONS		==========	*/

/* initializes library object and all other modules			*/
VAPI vBOOL vCoreInitialize(void);

/* checks whether the core has been initialized				*/
VAPI vBOOL vCoreIsInitialized(void);

/* terminates the library and all other modules				*/
VAPI vBOOL vCoreTerminate(void);


/* ========== TIME FUNCTIONS					 ========== */

/* returns the GetTickCount value for when vCoreInitialize  */
/* was called.												*/
VAPI vTIME vCoreGetTimeInitialized(void);

/* returns the miliseconds passed since core initialization */
VAPI vTIME vCoreGetTime(void);


/* ========== MESSAGE BOX ERROR FUNCTION		 ========== */

/* this should not be used by the user. instead, use the	*/
/* err module of core. this is for very low-level failures	*/
VAPI void vCoreCreateFatalError(const char* description);

#endif
