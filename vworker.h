
/* ========== <vworker.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Interruptable and dispatchable worker thread system		*/

#ifndef _VCORE_WORKER_INCLUDE_
#define _VCORE_WORKER_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vPWorker vCreateWorker(vTIME cycleInterval, vPFWORKERINIT initFunc,
	vPFWORKEREXIT exitFunc, vPFWORKERCYCLE cycleFunc, vUI64 persistentSizeBytes,
	vPTR initInput);
VAPI vBOOL vDestroyWorker(vPWorker worker);


/* ========== RUNTIME MANIPULATION				==========	*/
VAPI void  vWorkerPause(vPWorker worker);
VAPI void  vWorkerUnpause(vPWorker worker);
VAPI vBOOL vWorkerIsPaused(vPWorker worker);


/* ========== DISPATCH AND SYNCHRONIZATION		==========	*/
VAPI vTIME vWorkerGetCycle(vPWorker worker);
VAPI vTIME vWorkerDispatchLock(vPWorker worker);
VAPI vTIME vWorkerDispatchUnlock(vPWorker worker);
VAPI vTIME vWorkerDispatchTask(vPFWORKERTASK taskFunc, vPTR input);
VAPI void  vWorkerWaitCycleCompletion(vTIME lastCycle, vTIME maxWaitTime);


#endif
