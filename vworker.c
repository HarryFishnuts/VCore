
/* ========== <vworker.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Interruptable and dispatchable worker thread system		*/

/* ========== INCLUDES							==========	*/
#include "vworker.h"
#include <stdio.h>


/* ========== INTERNAL THREAD LOGIC				==========	*/
static void vhWorkerTaskIterateFunc(vHNDL buffer, vPWorkerTaskData taskData, vPWorker worker)
{
	if (taskData->task)
		taskData->task(worker, worker->persistentData, taskData->input);
}

static void vhWorkerTaskListElementInitFunc(vHNDL buffer, vPWorkerTaskData taskData, 
	vPWorkerTaskData input)
{
	taskData->task  = input->task;
	taskData->input = input->input;

	if (taskData->task == NULL)
	{
		vLogWarning(__func__,
			"Dispatched a NULL task to a worker.");
	}

	vFree(input);
}

static void vhWorkerComponentCycleElementInitFunc(vHNDL buffer, 
	vPWorkerComponentCycleData cycleData, vPWorkerComponentCycleData input)
{
	cycleData->component = input->component;
	cycleData->cycleFunc = input->cycleFunc;

	if (input->cycleFunc == NULL)
	{
		vLogWarningFormatted(__func__,
			"Component %p with no cycle function attached to a worker.",
			input->component);
	}
}

static void vhWorkerComponentCycleIterateFunc(vHNDL dBuffer,
	vPWorkerComponentCycleData data, vPWorker input)
{
	if (data->cycleFunc)
		data->cycleFunc(input, input->persistentData, data->component);
}

static void vhWorkerExitBehavior(vPWorker worker)
{
	/* log and run exitfunc */
	vLogInfoFormatted(__func__, "Worker '%s' recieved kill signal and is exiting.",
		worker->name);

	if (worker->exitFunc)
		worker->exitFunc(worker, worker->persistentData);

	/* free all memory and clear flags */
	vDestroyDBuffer(worker->taskList);
	vDestroyDBuffer(worker->componentCycleList);
	vFree(worker->persistentData);

	/* clear thread */
	worker->thread = NULL;

	ExitThread(ERROR_SUCCESS);
}

static DWORD WINAPI vhWorkerThreadProc(vPWorkerInput input)
{
	vPWorker worker = input->worker;
	vLogInfoFormatted(__func__, "Worker '%s' started with thread [%p].", 
		worker->name, worker->thread);

	/* call initialization function */
	if (worker->initFunc)
		worker->initFunc(worker, worker->persistentData, input->userInput);

	/* free input memory */
	vFree(input);

	/* start main loop */
	while (TRUE)
	{
		/* wait for interval to execute cycle */
		ULONGLONG currentTime = GetTickCount64();
		ULONGLONG nextCycleTime = worker->lastCycleTime + worker->cycleIntervalMiliseconds;
		if (currentTime < nextCycleTime)
		{
			Sleep(nextCycleTime - currentTime);
		}

		/* LOCK THREAD */
		EnterCriticalSection(&worker->cycleLock);

		/* complete all component cycles */
		vDBufferIterate(worker->componentCycleList,
			vhWorkerComponentCycleIterateFunc, worker);

		/* complete all tasks */
		vDBufferIterate(worker->taskList, vhWorkerTaskIterateFunc, worker);
		vDBufferClear(worker->taskList);

		/* check for kill signal */
		if (_bittest64(&worker->workerState, 1) == TRUE)
		{
			/* apply exit behavior */
			vhWorkerExitBehavior(worker);
		}

		/* if thread is suspended, ignore cycle */
		if (_bittest64(&worker->workerState, 0) == TRUE)
		{
			LeaveCriticalSection(&worker->cycleLock);
			continue;
		}

		/* complete next cycle */
		if (worker->cycleFunc)
			worker->cycleFunc(worker, worker->persistentData);
		worker->lastCycleTime = GetTickCount64();
		worker->cycleCount++;

		/* UNLOCK THREAD */
		LeaveCriticalSection(&worker->cycleLock);
	}
}

/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vPWorker vCreateWorker(vPCHAR name, vTIME cycleInterval, vPFWORKERINIT initFunc,
	vPFWORKEREXIT exitFunc, vPFWORKERCYCLE cycleFunc, vUI64 persistentSizeBytes,
	vPTR initInput)
{
	vPWorker worker;
	vCoreLock();

	for (int i = 0; i < WORKERS_MAX; i++)
	{
		worker = _vcore.workers + i;

		/* if thread is active, skip */
		DWORD exitCode = ZERO;
		GetExitCodeThread(worker->thread, &exitCode);
		if (exitCode == STILL_ACTIVE) continue;

		/* on found free worker, init worker */
		vZeroMemory(worker, sizeof(vWorker));

		vMemCopy(worker->name, name, min(BUFF_SMALL, strlen(name)));

		InitializeCriticalSection(&worker->cycleLock);
		worker->cycleIntervalMiliseconds = cycleInterval;
		worker->initFunc  = initFunc;
		worker->exitFunc  = exitFunc;
		worker->cycleFunc = cycleFunc;
		worker->persistentDataSizeBytes = persistentSizeBytes;
		worker->persistentData = vAllocZeroed(max(4UL, worker->persistentDataSizeBytes));

		/* initialize task buffer */
		char stringBuffer[BUFF_SMALL];
		vZeroMemory(stringBuffer, sizeof(stringBuffer));

		sprintf_s(stringBuffer, BUFF_SMALL, "Worker '%s' Task List",
			worker->name);
		worker->taskList = vCreateDBuffer(stringBuffer, sizeof(vWorkerTaskData),
			WORKER_TASKLIST_NODE_SIZE, vhWorkerTaskListElementInitFunc, NULL);

		/* initialize component cycle buffer */
		sprintf_s(stringBuffer, BUFF_SMALL, "Worker '%s' Component Cylce List",
			worker->name);
		worker->componentCycleList = vCreateDBuffer(stringBuffer, sizeof(vWorkerComponentCycleData),
			WORKER_COMPONENT_CYCLE_NODE_SIZE, vhWorkerComponentCycleElementInitFunc, NULL);

		/* prepare worker input */
		vPWorkerInput workerInput = vAllocZeroed(sizeof(vWorkerInput));
		workerInput->worker    = worker;
		workerInput->userInput = initInput;

		/* create thread and log */
		vLogInfoFormatted(__func__, "Creating worker '%s'.", worker->name);
		worker->thread = CreateThread(NULL, ZERO, vhWorkerThreadProc, 
			workerInput, NO_FLAGS, NULL);

		vCoreUnlock();
		return worker;
	}

	vLogError(__func__, "Unable to create worker.");

	vCoreUnlock();
	return NULL;
}

VAPI vBOOL vDestroyWorker(vPWorker worker)
{
	
	/* if called on worker to be destroyed */
	if (GetCurrentThreadId() == GetThreadId(worker->thread))
	{
		/* apply worker exit behavior */
		vhWorkerExitBehavior(worker);
	}

	vLogInfoFormatted(__func__, "Sending kill signal to worker '%s'.", worker->name);

	/* set kill signal */
	EnterCriticalSection(&worker->cycleLock);
	_bittestandset64(&worker->workerState, 1);
	LeaveCriticalSection(&worker->cycleLock);

	/* wait for thread to finish */
	DWORD result = WaitForSingleObject(worker->thread, INFINITE);
	if (result != WAIT_OBJECT_0)
	{
		vLogErrorFormatted(__func__, "Error while trying to wait for worker thread '%s'",
			worker->name);
		vCoreFatalError(__func__, "Error while destroying worker.");
	}

	vDumpEntryBuffer();
	return TRUE;
}


/* ========== RUNTIME MANIPULATION				==========	*/
VAPI void  vWorkerPause(vPWorker worker)
{
	EnterCriticalSection(&worker->cycleLock);
	_bittestandset64(&worker->workerState, 0);
	LeaveCriticalSection(&worker->cycleLock);
}

VAPI void  vWorkerUnpause(vPWorker worker)
{
	EnterCriticalSection(&worker->cycleLock);
	_bittestandreset64(&worker->workerState, 0);
	LeaveCriticalSection(&worker->cycleLock);
}

VAPI vBOOL vWorkerIsPaused(vPWorker worker)
{
	BOOLEAN state = FALSE;
	EnterCriticalSection(&worker->cycleLock);
	state = _bittest(&worker->workerState, 0);
	LeaveCriticalSection(&worker->cycleLock);
	return state;
}

VAPI vBOOL vWorkerIsAlive(vPWorker worker)
{
	DWORD exitCode = ZERO;
	GetExitCodeThread(worker->thread, &exitCode);
	return (exitCode == STILL_ACTIVE);
}


/* ========== DISPATCH AND SYNCHRONIZATION		==========	*/
VAPI vTIME vWorkerGetCycle(vPWorker worker)
{
	vTIME cycle = 0;
	EnterCriticalSection(&worker->cycleLock);
	cycle = worker->cycleCount;
	LeaveCriticalSection(&worker->cycleLock);
	return cycle;
}

VAPI void  vWorkerLock(vPWorker worker)
{
	EnterCriticalSection(&worker->cycleLock);
}

VAPI void  vWorkerUnlock(vPWorker worker)
{
	LeaveCriticalSection(&worker->cycleLock);
}

VAPI vTIME vWorkerDispatchTask(vPWorker worker, vPFWORKERTASK taskFunc, vPTR input)
{
	/* if current thread is the worker, execute immediately */
	if (GetCurrentThreadId() == GetThreadId(worker->thread))
	{
		taskFunc(worker, worker->persistentData, input);
		return vWorkerGetCycle(worker);
	}

	EnterCriticalSection(&worker->cycleLock);
	
	vPWorkerTaskData taskData = vAllocZeroed(sizeof(vWorkerTaskData));
	taskData->task  = taskFunc;
	taskData->input = input;

	vDBufferAdd(worker->taskList, taskData);

	LeaveCriticalSection(&worker->cycleLock);

	return vWorkerGetCycle(worker);
}

VAPI vBOOL vWorkerWaitCycleCompletion(vPWorker worker, vTIME lastCycle, vTIME maxWaitTime)
{
	/* if current thread is the worker, return immediately */
	if (GetCurrentThreadId() == GetThreadId(worker->thread)) return TRUE;

	ULONGLONG startTime = GetTickCount64();

	while (TRUE)
	{
		ULONGLONG currentTime = GetTickCount64();

		/* on reached max time, return FALSE */
		if (currentTime > startTime + maxWaitTime) return FALSE;

		/* check the worker's current cycle number */
		vTIME currentCycle = vWorkerGetCycle(worker);

		/* if unfinished, sleep for interval and continue */
		if (currentCycle < lastCycle + 1)
		{
			Sleep(worker->cycleIntervalMiliseconds);
			continue;
		}

		/* on reached here, worker cycle is complete. break */
		break;
	}
	
	return TRUE;
}