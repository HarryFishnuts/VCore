
/* ========== <buffer.c>						==========	*/
/* Bailey Jia-Tao Brown			2022						*/


/* ========== INCLUDES							==========	*/
#include "buffer.h"
#include <stdio.h>


/* ========== INTERNAL FUNCTIONS				==========	*/

static inline void vbuffCaptureRWPermission(void)
{
	EnterCriticalSection(&_vcore->bufferHandler.rwPermission);
}

static inline void vbuffReleaseRWPermission(void)
{
	LeaveCriticalSection(&_vcore->bufferHandler.rwPermission);
}

static inline vI32 vbuffFindFreeBufferIndex(void)
{
	int startIndex = _vcore->bufferHandler.bufferCount >> 1;
	for (vI32 i = startIndex; i < MAX_BUFFER_OBJECTS; i++)
	{
		if (_vcore->bufferHandler.buffers[i]) continue;
		return i;
	}

	/* on reached here, no buffers left */
	vCoreCreateFatalError("Buffer Objects Full");
}


/* ========== INITIALIZATION FUNCTIONS			==========	*/

VAPI void _vBufferInit(void)
{
	InitializeCriticalSection(&_vcore->bufferHandler.rwPermission);
}

VAPI void _vBufferTerminate(void)
{
	DeleteCriticalSection(&_vcore->bufferHandler.rwPermission);
}


/* ========== BUFFER CREATION AND DESTRUCTION	==========	*/

/* creates a buffer behavior for buffer objects to adhere	*/
/* to. buffer behaviors cannot be destroyed.				*/
static char __VCBBremarkFmtBuff[BUFF_MEDIUM];
VAPI vHNDL vCreateBufferBehavior(const char* name, SIZE_T elementSize,
	vI32 bufferSize, vBOOL threadSafe, vBOOL lockPerElement,
	vBOOL zeroElements, vPFBUFFINITIALIZER elementInitCallback,
	vPFBUFFDESTRUCTOR  elementDestroyCallback)
{
	/* CRITICAL SECT ENTER */ vbuffCaptureRWPermission();

	if (_vcore->bufferHandler.behaviorCount >= MAX_BUFFER_BEHAVIORS)
		vCoreCreateFatalError("Buffer Behaviors Full");

	vPBufferBehavior bhvPtr = _vcore->bufferHandler.behaviors + _vcore->bufferHandler.behaviorCount;

	/* set members */
	__movsb(&bhvPtr->name, name, strlen(name));
	bhvPtr->initializer = elementInitCallback;
	bhvPtr->destructor  = elementDestroyCallback;
	bhvPtr->elementSize = elementSize;
	bhvPtr->bufferSize  = bufferSize;

	/* set flags */
	if (threadSafe)		_bittestandset(&bhvPtr->flags, 0);
	if (lockPerElement) _bittestandset(&bhvPtr->flags, 1);
	if (zeroElements)	_bittestandset(&bhvPtr->flags, 2);
	
	/* log creation */
	sprintf_s(__VCBBremarkFmtBuff, sizeof(__VCBBremarkFmtBuff), "NAME: %s\nINDEX: %d\nSIZE: %d\n"
		"LENGTH: %d\nTHREADSAFE: %d\n", name, _vcore->bufferHandler.behaviorCount,
		(vI32)elementSize, bufferSize, threadSafe);
	vLogAction("Created Buffer Behavior", __VCBBremarkFmtBuff);

	_vcore->bufferHandler.behaviorCount++; /* INCREMENT BUFFERBHV COUNT */

	/* CRITICAL SECT LEAVE */ vbuffReleaseRWPermission();

	return _vcore->bufferHandler.behaviorCount - 1;
}

/* creates a buffer object to holds things within.			 */
static char _VCBremarkFmtBuff[BUFF_MEDIUM];
VAPI vHNDL vCreateBuffer(const char* name, vHNDL behavior)
{
	/* CRITICAL SECT ENTER */ vbuffCaptureRWPermission();

	vPBufferBehavior bhv = _vcore->bufferHandler.behaviors + behavior;

	/* allocate buffer object to heap */
	vI32 buffIndex = vbuffFindFreeBufferIndex();
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffIndex];
	buff = vAllocZeroed(sizeof(vBufferObject));

	/* copy name */
	__movsb(buff->name, name, strlen(name));

	/* allocate field and data members */
	vI32 fieldSize = (bhv->bufferSize >> 0x6) + 1;
	buff->field = vAllocZeroed(sizeof(vUI64) * fieldSize);
	buff->data  = vAllocZeroed(bhv->elementSize * bhv->bufferSize);

	/* initialize buffer rw mutex */
	InitializeCriticalSection(&buff->rwPermission);

	_vcore->bufferHandler.bufferCount++;

	/* log buffer creation */
	sprintf_s(__VCBBremarkFmtBuff, sizeof(__VCBBremarkFmtBuff),
		"NAME: %s\nBEHAVIOR:%s\nINDEX:%d\n", buff->name, bhv->name,
		buffIndex);
	vLogAction("Created Buffer", __VCBBremarkFmtBuff);

	/* CRITICAL SECT LEAVE */ vbuffReleaseRWPermission();

}

/* destroys a buffer object									 */
VAPI vBOOL vDestroyBuffer(vHNDL bufferHndl);


/* ========== BUFFER ELEMENT OPERATIONS			==========	*/

/* finds an empty spot in the buffer and returns the		*/
/* pointer to that element for the user to store or modify  */
/* NOT TO BE USED WHEN MULTITHREADING!!						*/
VAPI vPTR vBufferAdd(vHNDL buffer);

/* finds an empty spot in the buffer and then performs the	*/
/* operation specified (if any) to the element.				*/
VAPI void vBufferAddSafe(vHNDL buffer, vPFBUFFOPERATION operation);

/* returns a pointer to an element in the buffer specified  */
/* by an index. NOT TO BE USED WHEN MULTITHREADING!!		*/
VAPI vPTR vBufferGet(vHNDL buffer, vI32 index);

/* finds the element in the buffer specified by an index    */
/* performs the function passed on it						*/
VAPI void vBufferOperate(vHNDL buffer, vI32 index, vPFBUFFOPERATION operation);

/* finds the element within the buffer by index and removes */
/* it, applying the destructor function if any.				*/
VAPI void vBufferRemoveIndex(vHNDL buffer, vI32 index);

/* loops through every element and performs the specified	*/
/* operation on it. buffer may be synced depending on bhv	*/
VAPI void vBufferIterate(vHNDL buffer, vPFBUFFITERATOR operation);