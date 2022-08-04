
/* ========== <vlock.h>							==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Low-overhead thread synchronization system				*/

#ifndef _VCORE_LOCK_INCLUDE_
#define _VCORE_LOCK_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vHNDL vCreateLock(void);
VAPI void  vDestroyLock(vHNDL lock);


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vLock(vHNDL lock);
VAPI void vUnlock(vHNDL lock);

#endif