
/* ========== <vcorefunctions.h>				==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Library critical functions								*/

#ifndef _VCORE_COREFUNCTIONS_INCLUDE_
#define _VCORE_COREFUNCTIONS_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== INITIALIZATION AND TERMINATION	==========	*/
VAPI vBOOL vCoreInitialize(void);
VAPI vBOOL vCoreTerminate(void);


/* ========== TIME								==========	*/
VAPI void vCoreTime(vPTIME outTime);
VAPI void vCoreInitializationTime(vPTIME outTime);


/* ========== CONCURRENCY						==========	*/
VAPI void vCoreLock(void);
VAPI void vCoreUnlock(void);


/* ========== CORE ERROR HANDLING				==========	*/
VAPI void vCoreFatalError(vPCHAR function, vPCHAR remarks);
VAPI void vCoreCrash(void);


/* ========== ALLOCATION						==========	*/
VAPI vPTR vAlloc(size_t size);
VAPI vPTR vAllocZeroed(size_t size);
VAPI void vFree(vPTR ptr);
VAPI vUI64 vGetMemoryUseage(void);


/* ========== MEMORY MANIPULATION				==========	*/
VAPI void vZeroMemory(vPTR block, size_t length);
VAPI void vMemCopy(vPTR destination, vPTR source, size_t length);
VAPI _vPCoreInternals vGetInternals(void);

#endif
