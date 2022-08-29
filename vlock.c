
/* ========== <vlock.c>							==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vlock.h"
#include <intrin.h>


/* ========== HELPER							==========	*/
static __forceinline vUI32 vhFindFreeLockIndex(void)
{
	for (vI64 i = 0; i < MAX_LOCKS; i++)
	{
		if (*(DWORD*)&_vcore.locks[i] == UNUSED_LOCK) return i;
	}

	/* on reached here, all used, end process */
	vLogError(__func__,  "All locks have been used.");
	vCoreFatalError(__func__, "Tried to create a new lock but all locks have been used. "
		"Error is unrecoverable.");
}


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vHNDL vCreateLock(void)
{
	/* SYNC		*/ vCoreLock();

	vUI32 lockIndex = vhFindFreeLockIndex();
	InitializeCriticalSection(&_vcore.locks[lockIndex]);
	vLogInfoFormatted(__func__, "Created lock with handle: %d.", lockIndex);

	/* UNSYNC	*/ vCoreUnlock();

	return lockIndex;
}

VAPI void  vDestroyLock(vHNDL lock)
{
	/* SYNC		*/ vCoreLock();

	/* ensure lock is active */
	if (*(DWORD*)&_vcore.locks[lock] == UNUSED_LOCK)
	{
		vLogWarningFormatted(__func__, 
			"Tried to destroy lock with handle '%d' but it doesn't exist.", lock);
		return;
	}

	/* lock and delete */
	EnterCriticalSection(&_vcore.locks[lock]);
	DeleteCriticalSection(&_vcore.locks[lock]);

	/* set use flag to false */
	*(DWORD*)&_vcore.locks[lock] = UNUSED_LOCK;

	vLogInfoFormatted(__func__, "Destroyed lock with handle: %d.", lock);

	/* UNSYNC	*/ vCoreUnlock();
}


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vLock(vHNDL lock)
{
	EnterCriticalSection(&_vcore.locks[lock]);
}

VAPI void vUnlock(vHNDL lock)
{
	LeaveCriticalSection(&_vcore.locks[lock]);
}