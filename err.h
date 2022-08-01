
/* ========== <err.h>							==========	*/
/* Bailey Jia-Tao Brown			2022						*/
/* Low-overhead error handling and logging.					*/


#ifndef _VCORE_ERR_INCLUDE_
#define _VCORE_ERR_INCLUDE_


/* ========== INCLUDES							==========	*/
#include "core.h"


/* ========== INITIALIZATION FUNCTIONS			==========	*/

VAPI void _vErrInit(const char* logFileName);
VAPI void _vErrTerminate(void);


/* ========== LOGGING AND ERROR FUNCTIONS		==========	*/

/* logs meaningful events and should be used sparringly and */
/* absolutely not for debug purposes.						*/
VAPI void vLogAction(const char* action, const char* remarks);

/* logs an unexpected issue which may cause issues later in */
/* the processes execution									*/
VAPI void vLogWarning(const char* warning, const char* remarks);

/* logs an error which is expected to end the process.		*/
VAPI void vLogError(const char* error, const char* remarks);


/* ========== FILE I/O FUNCTIONS				==========	*/

/* writes the entire log buffer to a file specified in the	*/
/* initialization function.									*/
VAPI void vDumpLogBuffer(void);

#endif