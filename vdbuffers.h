
/* ========== <vdbuffers.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Medium-Overhead dynamic sized buffering system			*/

#ifndef _VCORE_DBUFFERS_INCLUDE_
#define _VCORE_DBUFFERS_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"

/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vHNDL vCreateDBuffer(const char* dBufferName, vUI16 elementSize);
VAPI vBOOL vDestroyDBuffer(vHNDL dBuffer);


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vDBufferLock(vHNDL dBuffer);
VAPI void vDBufferUnlcok(vHNDL dBuffer);


/* ========== ELEMENT MANIPULATION				==========	*/
VAPI vPTR vDBufferAdd(vHNDL dBuffer);
VAPI void vDBufferRemove(vHNDL dBuffer, vPTR element);
VAPI void vDBufferIterate(vHNDL buffer, vPFBUFFERITERATEFUNC function);
VAPI void vDBufferClear(vHNDL buffer);

#endif
