
/* ========== <mem.c>							========== */
/* Bailey Jia-Tao Brown			2022					   */


/* ========== INCLUDES							========== */
#include "mem.h"
#include <intrin.h>


/* ========== INTERNAL DATA						========== */
static HANDLE _vMemHeap = NO_HANDLE;

/* ========== INITIALIZATION FUNCTIONS			========== */

VAPI void _vMemInit(void)
{
	_vMemHeap = HeapCreate(NO_FLAGS, ZERO, ZERO);
	if (!_vMemHeap) vCoreCreateFatalError("Heap Create Failed");
}

VAPI void _vMemTerminate(void)
{
	if (!HeapDestroy(_vMemHeap))
		vCoreCreateFatalError("Heap Destroy Failed");
	_vMemHeap = NO_HANDLE;
}


/* ========== MEMORY FUNCTIONS					========== */

/* allocated memory of size. on failed, will create msgbox */
/* and then terminate process.							   */
VAPI vPTR vAlloc(SIZE_T size)
{
	vPTR memPtr = HeapAlloc(_vMemHeap, NO_FLAGS, size);
	if (!memPtr) vCoreCreateFatalError("Heap Alloc Failure");
	return memPtr;
}

VAPI vPTR vAllocZeroed(SIZE_T size)
{
	vPTR memPtr = HeapAlloc(_vMemHeap, NO_FLAGS, size);
	if (!memPtr) vCoreCreateFatalError("Heap Alloc Failure");
	__stosb(memPtr, ZERO, size);
	return memPtr;
}

/* frees memory pointer. on fail, will msgbox then close   */
/* the process											   */
VAPI void vFree(vPTR ptr)
{
	if (!HeapFree(_vMemHeap, NO_FLAGS, ptr))
		vCoreCreateFatalError("Heap Free Failure");
}
