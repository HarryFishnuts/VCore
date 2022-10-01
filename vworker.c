
/* ========== <vworker.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Interruptable and dispatchable worker thread system		*/

/* ========== INCLUDES							==========	*/
#include "vworker.h"
#include <stdio.h>


/* ========== INTERNAL THREAD LOGIC				==========	*/
static void vhWorkerTaskIterate(vHNDL buffer, vPWorkerTaskData taskData, vPWorker worker)
{
	if (taskData->task)
		taskData->task(worker, worker->persistentData, taskData->input);
}

static DWORD WINAPI vhWorkerThreadProc(vPWorkerInput input)
{
	vPWorker worker = input->worker;
	vLogInfoFormatted(__func__, "Worker thread [%p] started.", worker);

	/* call initialization function */
	if (worker->initFunc)
		worker->initFunc(worker, worker->persistentData, input->userInput);

	/* start main loop */
	while (TRUE)
	{
		/* LOCK THREAD */
		EnterCriticalSection(&worker->cycleLock);

		/* wait for interval to execute cycle */
		ULONGLONG currentTime = GetTickCount64();
		ULONGLONG nextCycleTime = worker->lastCycleTime + worker->cycleIntervalMiliseconds;
		if (currentTime < nextCycleTime)
		{
			Sleep(nextCycleTime - currentTime);
		}

		/* check for kill signal */
		if (_bittest64(&worker->workerState, 1) == TRUE)
		{
			if (worker->exitFunc)
				worker->exitFunc(worker, worker->persistentData);

			ExitThread(1);
		}

		/* complete all tasks */
		vDBufferIterate(worker->taskList, vhWorkerTaskIterate, worker);
		vDBufferClear(worker->taskList);

		/* if thread is suspended, ignore cycle */
		if (_bittest64(&worker->workerState, 0) == TRUE) continue;

		/* complete next cycle */
		worker->cycleFunc(worker, worker->persistentData);
		worker->lastCycleTime = GetTickCount64();
		worker->cycleCount++;

		/* UNLOCK THREAD */
		LeaveCriticalSection(&worker->cycleLock);
	}
}

/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vPWorker vCreateWorker(vTIME cycleInterval, vPFWORKERINIT initFunc,
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
		InitializeCriticalSection(&worker->cycleLock);
		worker->cycleIntervalMiliseconds = cycleInterval;
		worker->initFunc  = initFunc;
		worker->exitFunc  = exitFunc;
		worker->cycleFunc = cycleFunc;
		worker->persistentDataSizeBytes = persistentSizeBytes;
		worker->persistentData = vAllocZeroed(worker->persistentDataSizeBytes);

		/* initialize task buffer */
		char taskListNameBuffer[BUFF_SMALL];
		vZeroMemory(taskListNameBuffer, sizeof(taskListNameBuffer));
		sprintf_s(taskListNameBuffer, BUFF_SMALL, "Worker [%d] [%p] Taskbuffer",
			i, worker);
		worker->taskList = vCreateDBuffer(taskListNameBuffer, sizeof(vWorkerTaskData),
			0x100, NULL, NULL);

		/* prepare worker input */
		vWorkerInput workerInput;
		workerInput.worker    = worker;
		workerInput.userInput = initInput;

		vLogInfoFormatted(__func__, "Creating worker [%p].", worker);
		worker->thread = CreateThread(NULL, ZERO, vhWorkerThreadProc, 
			&workerInput, NO_FLAGS, NULL);

		vCoreUnlock();
		return worker;
	}

	vLogError(__func__, "Unable to create worker.");

	vCoreUnlock();
	return NULL;
}

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
