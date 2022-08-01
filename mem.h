
/* ========== <mem.h>							========== */
/* Bailey Jia-Tao Brown			2022					   */
/* Lightweight memory allocation with error checking	   */


#ifndef _VCORE_MEM_INCLUDE_
#define _VCORE_MEM_INCLUDE_ 


/* ========== INCLUDES							========== */
#include "internal.h"
#include "core.h"


/* ========== INITIALIZATION FUNCTIONS			========== */

VAPI void _vMemInit(void);
VAPI void _vMemTerminate(void);


/* ========== MEMORY FUNCTIONS					========== */

/* allocated memory of size. on failed, will create msgbox */
/* and then terminate process.							   */
VAPI vPTR vAlloc(SIZE_T size);
VAPI vPTR vAllocZeroed(SIZE_T size);

/* frees memory pointer. on fail, will msgbox then close   */
/* the process											   */
VAPI void vFree(vPTR ptr);


#endif
