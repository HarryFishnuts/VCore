
/* ========== <ventriesformatted.h>				==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Formatted event logging system							*/

#ifndef _VCORE_ENTRIES_FORMATTED_INCLUDE_
#define _VCORE_ENTRIES_FORMATTED_INCLUDE_

/* ========== INCLUDES							==========	*/
#include <stdarg.h>
#include "ventries.h"


/* ========== FORMATTED EVENT LOGGING			==========	*/
VAPI void vLogEventFormatted(const char* funcName, const char* remarks, ...);
VAPI void vLogWarningFormatted(const char* funcName, const char* remarks, ...);
VAPI void vLogErrorFormatted(const char* funcName, const char* remarks, ...);


#endif