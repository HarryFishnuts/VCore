
/* ========== <vstructs.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* All structure definitions for VCore						*/

#ifndef _VCORE_STRUCTS_INCLUDE_
#define _VCORE_STRUCTS_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vtypes.h"
#include "vdefs.h"
#include <Windows.h>


/* ========== ENTRY								==========	*/
typedef struct vEntry
{
	vUI32 entryNumber;

	vBYTE entryType;
	vTIME timeCreated;				/* as dictated by vCoreTime			*/
	DWORD threadID;

	vCHAR function[BUFF_SMALL];		/* name of function that generated	*/
									/* the entry						*/
	vCHAR remarks[BUFF_LARGE];
} vEntry, *vPEntry;


/* ========== ENTRY BUFFER						==========	*/
typedef struct vEntryBuffer
{
	vTIME lastWriteTime;

	vUI32 entriesTotal;			/* total entries created				*/
	vUI16 entriesInMemory;		/* entries in memory					*/
	vUI16 diskWriteCount;		/* times written to disk				*/
	vUI16 logFileNumber;		/* number which rollovers once it		*/
								/* reaches the MAX_ENTRYLOGS_ON_DISK	*/

	vEntry buffer[MAX_ENTRIES_IN_MEMORY];
} vEntryBuffer, *vPEntryBuffer;


/* ========== BUFFER							==========	*/
typedef struct vBuffer
{
	vBOOL inUse;

	CRITICAL_SECTION rwPermission;	/* thread synchronization object		*/

	vTIME timeCreated;
	vCHAR name[BUFF_SMALL];

	vUI16 elementSize;				/* size of each element					*/
	vUI16 capacity;					/* max amount of elements storable		*/
	vUI32 sizeBytes;				/* data size in bytes					*/

	vUI16 elementsUsed;				/* amount of elements used				*/

	vUI16  useFieldLength;			/* useage field array size				*/
	vPUI64 useField;				/* usage bitfield (stored on heap)		*/

	vPBYTE data;					/* ptr to data on heap					*/
} vBuffer, *vPBuffer;


/* ========== BUFFER INFORMATION				==========	*/
typedef struct vBufferInfo
{
	vCHAR* name;

	vTIME timeCreated;
	vUI16 elementUsed;

	vUI16 elementSize;
	vUI16 capacity;
	vUI32 sizeBytes;

	float usePercentage;
} vBufferInfo, *vPBufferInfo;


/* ========== VCORE INTERNAL MEMORY LAYOUT		==========	*/
/* A single instance of this struct exists to be shared		*/
/* across all source files of VCore.						*/
/* All library-side data will be stored in the instance,	*/
/* and is not intended to be directly acessed by the user	*/
typedef struct _vCoreInternals
{
	CRITICAL_SECTION rwPermission;		/* synchronization object		*/
	
	vBOOL  initialized;					/* checks whether library has	*/
										/* been initialized				*/

	HANDLE heap;						/* heap handle for allocation	*/
	vCHAR  stringBuffer[BUFF_LARGE];	/* pre-allocated string buffer	*/
	vTIME  initializationTime;			/* time initialized in msecs	*/

	vEntryBuffer entryBuffer;			/* entry system container		*/

	vBuffer buffers[MAX_BUFFERS];		/* buffer list					*/

} _vCoreInternals, *_vPCoreInternals;

_vCoreInternals _vcore;	/* INSTANCE	*/

#endif