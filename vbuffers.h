
/* ========== <vbuffers.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Low-overhead threadsafe fixed-sized buffering system		*/

#ifndef _VCORE_BUFFERS_INCLUDE_
#define _VCORE_BUFFERS_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vHNDL vCreateBuffer(const char* bufferName, vUI16 elementSize,
	vUI16 capacity);
VAPI vBOOL vDestroyBuffer(vHNDL buffer);


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vBufferLock(vHNDL buffer);
VAPI void vBufferUnlock(vHNDL buffer);


/* ========== ELEMENT MANIPULATION				==========	*/
VAPI vPTR  vBufferAdd(vHNDL buffer);
VAPI void  vBufferRemove(vHNDL buffer, vPTR element);
VAPI void  vBufferRemoveIndex(vHNDL buffer, vUI16 index);
VAPI vUI16 vBufferGetElementIndex(vHNDL buffer, vPTR element);
VAPI vPTR  vBufferGetIndex(vHNDL buffer, vUI16 index);
VAPI void  vBufferIterate(vHNDL buffer, vPFBUFFERITERATEFUNC function);


/* ========== BUFFER INFORMATION				==========	*/
VAPI void  vBufferGetInfo(vHNDL buffer, vPBufferInfo infoOut);
VAPI vBOOL vBufferExists(vHNDL buffer);
VAPI vBOOL vBufferIndexUsed(vHNDL buffer, vUI16 index);

#endif
