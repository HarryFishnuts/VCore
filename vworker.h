
/* ========== <vworker.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Interruptable and dispatchable worker thread system		*/

#ifndef _VCORE_WORKER_INCLUDE_
#define _VCORE_WORKER_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vPWorker vCreateWorker(vPCHAR name, vTIME cycleInterval, vPFWORKERINIT initFunc,
	vPFWORKEREXIT exitFunc, vPFWORKERCYCLE cycleFunc, vUI64 persistentSizeBytes,
	vPTR initInput);
VAPI vBOOL vDestroyWorker(vPWorker worker);


/* ========== RUNTIME MANIPULATION				==========	*/
VAPI void  vWorkerPause(vPWorker worker);
VAPI void  vWorkerUnpause(vPWorker worker);
VAPI vBOOL vWorkerIsPaused(vPWorker worker);
VAPI vBOOL vWorkerIsAlive(vPWorker worker);


/* ========== DISPATCH AND SYNCHRONIZATION		==========	*/
VAPI vTIME vWorkerGetCycle(vPWorker worker);
VAPI void  vWorkerLock(vPWorker worker);
VAPI void  vWorkerUnlock(vPWorker worker);
VAPI vTIME vWorkerDispatchTask(vPWorker worker, vPFWORKERTASK taskFunc, vPTR input);
VAPI vBOOL vWorkerWaitCycleCompletion(vPWorker worker, vTIME lastCycle, vTIME maxWaitTime);


#endif
