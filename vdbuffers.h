
/* ========== <vdbuffers.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Medium-Overhead dynamic sized buffering system			*/

#ifndef _VCORE_DBUFFERS_INCLUDE_
#define _VCORE_DBUFFERS_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"

/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vHNDL vCreateDBuffer(const char* dBufferName, vUI16 elementSize, 
	vUI32 nodeSize, vPFDBUFFERINITIALIZEELEMENT initializeFunc,
	vPFDBUFFERDESTROYELEMENT destroyFunc);
VAPI vBOOL vDestroyDBuffer(vHNDL dBuffer);


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vDBufferLock(vHNDL dBuffer);
VAPI void vDBufferUnlock(vHNDL dBuffer);


/* ========== ELEMENT MANIPULATION				==========	*/
VAPI vPTR vDBufferAdd(vHNDL dBuffer, vPTR input);
VAPI void vDBufferRemove(vHNDL dBuffer, vPTR element);
VAPI void vDBufferIterate(vHNDL dBuffer, vPFDBUFFERITERATEFUNC function, vPTR input);
VAPI void vDBufferClear(vHNDL dBuffer);


/* ========== BUFFER INFORMATION				==========	*/
VAPI vUI32 vDBufferGetElementCount(vHNDL dBuffer);

#endif
