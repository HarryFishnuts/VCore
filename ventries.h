
/* ========== <ventries.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Low-overhead event logging system						*/

#ifndef _VCORE_ENTRIES_INCLUDE_
#define _VCORE_ENTRIES_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== EVENT LOGGING						==========	*/
VAPI vLogEvent(const char* funcName, const char* remarks);
VAPI vLogWarning(const char* funcName, const char* remarks);
VAPI vLogError(const char* funcName, const char* remarks);

#endif
