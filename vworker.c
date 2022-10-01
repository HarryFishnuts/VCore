
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
}

static DWORD WINAPI vhWorkerThreadProc(vPWorkerInput input)
{
	vPWorker worker = input->worker;
	vLogInfoFormatted(__func__, "Worker '%s' started with thread [%p].", 
		worker->name, worker->thread);

	/* call initialization function */
	if (worker->initFunc)
		worker->initFunc(worker, worker->persistentData, input->userInput);

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

		/* check for kill signal */
		if (_bittest64(&worker->workerState, 1) == TRUE)
		{
			vLogInfoFormatted(__func__, "Worker '%s' recieved kill signal and is exiting.",
				worker->name);

			if (worker->exitFunc)
				worker->exitFunc(worker, worker->persistentData);

			ExitThread(1);
		}

		/* complete all tasks */
		vDBufferIterate(worker->taskList, vhWorkerTaskIterateFunc, worker);
		vDBufferClear(worker->taskList);

		/* if thread is suspended, ignore cycle */
		if (_bittest64(&worker->workerState, 0) == TRUE) continue;

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

		/* if used, skip */
		if (worker->thread != NULL) continue;

		/* on found free worker, init worker */
		vZeroMemory(worker, sizeof(vWorker));

		vMemCopy(worker->name, name, min(BUFF_SMALL, strlen(name)));

		InitializeCriticalSection(&worker->cycleLock);
		worker->cycleIntervalMiliseconds = cycleInterval;
		worker->initFunc  = initFunc;
		worker->exitFunc  = exitFunc;
		worker->cycleFunc = cycleFunc;
		worker->persistentDataSizeBytes = persistentSizeBytes;
		worker->persistentData = vAllocZeroed(max(4, worker->persistentDataSizeBytes));

		/* initialize task buffer */
		char taskListNameBuffer[BUFF_SMALL];
		vZeroMemory(taskListNameBuffer, sizeof(taskListNameBuffer));
		sprintf_s(taskListNameBuffer, BUFF_SMALL, "Worker '%s' Taskbuffer",
			worker->name);
		worker->taskList = vCreateDBuffer(taskListNameBuffer, sizeof(vWorkerTaskData),
			0x100, vhWorkerTaskListElementInitFunc, NULL);

		/* prepare worker input */
		vWorkerInput workerInput;
		workerInput.worker    = worker;
		workerInput.userInput = initInput;

		/* create thread and log */
		vLogInfoFormatted(__func__, "Creating worker '%s'.", worker->name);
		worker->thread = CreateThread(NULL, ZERO, vhWorkerThreadProc, 
			&workerInput, NO_FLAGS, NULL);

		vCoreUnlock();
		return worker;
	}

	vLogError(__func__, "Unable to create worker.");

	vCoreUnlock();
	return NULL;
}

VAPI vBOOL vDestroyWorker(vPWorker worker)
{
	EnterCriticalSection(&worker->cycleLock);
	_bittestandset64(&worker->workerState, 1);
	LeaveCriticalSection(&worker->cycleLock);
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
	EnterCriticalSection(&worker->cycleLock);
	
	vWorkerTaskData taskData;
	taskData.task  = taskFunc;
	taskData.input = input;
	vDBufferAdd(worker->taskList, &taskData);

	LeaveCriticalSection(&worker->cycleLock);

	return vWorkerGetCycle(worker);
}

VAPI vBOOL vWorkerWaitCycleCompletion(vPWorker worker, vTIME lastCycle, vTIME maxWaitTime)
{
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

