
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
	vUI32 entriesTotal;		/* total entries created	*/
	vUI16 entriesInMemory;		/* entries in memory		*/
	vUI16 diskWriteCount;		/* buffers written to disk	*/

	vEntry buffer[ENTRIES_SAVED_IN_MEMORY];
} vEntryBuffer, *vPEntryBuffer;


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

	vEntryBuffer entryBuffer;

} _vCoreInternals, *_vPCoreInternals;

_vCoreInternals _vcore;	/* INSTANCE	*/

#endif