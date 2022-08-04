
/* ========== <ventries.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Low-overhead event logging system						*/

#ifndef _VCORE_ENTRIES_INCLUDE_
#define _VCORE_ENTRIES_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== EVENT LOGGING						==========	*/
VAPI void vLogInfo(const char* funcName, const char* remarks);
VAPI void vLogWarning(const char* funcName, const char* remarks);
VAPI void vLogError(const char* funcName, const char* remarks);


/* ========== FILE I/O							==========	*/
VAPI void  vDumpEntryBuffer(void);
VAPI vBOOL vReadEntryBuffer(const char* fileName, vPEntryBuffer buffer);

#endif
