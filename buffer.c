
/* ========== <buffer.c>						==========	*/
/* Bailey Jia-Tao Brown			2022						*/


/* ========== INCLUDES							==========	*/
#include "buffer.h"
#include <stdio.h>

/* ========== INTERNAL DATA						==========	*/
static char __remarkBuffer[BUFF_MEDIUM];


/* ========== INTERNAL FUNCTIONS				==========	*/

static inline void vbuffCaptureRWPermission(void)
{
	EnterCriticalSection(&_vcore->bufferHandler.rwPermission);
}

static inline void vbuffReleaseRWPermission(void)
{
	LeaveCriticalSection(&_vcore->bufferHandler.rwPermission);
}

static inline vI32 vbuffFindFreeBufferObject(void)
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

static inline void vbuffMapIndexToField(vI32 index, vPI32 chunk, vPI32 bit)
{
	*chunk	= (index >> 0x06);
	*bit	= (index &  0x3F);
}

static inline vI32 vbuffMapFieldToIndex(vI32 chunk, vI32 bit)
{
	return (chunk * 0x40) + bit;
}

static inline void vbuffSetBitField(vPUI64 field, vI32 index, vBOOL value)
{
	vI32 chunk, bit;
	vbuffMapIndexToField(index, &chunk, &bit);
	if (value)
		_bittestandset64(&field[chunk], bit);
	else
		_bittestandreset64(&field[chunk], bit);
}

static inline void vbuffCreateBufferFullMsg(vPBufferObject object)
{
	sprintf_s(__remarkBuffer, sizeof(__remarkBuffer),
		"NAME: %s\nBEHAVIOR: %s\nSIZE: %d\n",
		object->name, object->behavior->name, object->behavior->elementCount);
	vLogError("Buffer Is Full", __remarkBuffer);
	vCoreCreateFatalError("Buffer Full");
}

/* NOTE: THIS FUNCTION WILL MARK THE FOUND SPOT AS TAKEN */
static inline vI32 vbuffFindFreeBufferObjectSpot(vPBufferObject object)
{
	if (object->usedElementCount >= object->behavior->elementCount)
		vbuffCreateBufferFullMsg(object);

	int startChunk = object->usedElementCount >> 0x07;
	for (int i = startChunk; i < object->behavior->fieldChunkCount; i++)
	{
		for (int j = 0; j < 0x40; j++)
		{
			if (_bittest64(&object->field[i], j)) continue;
			_bittestandset64(&object->field[i], j); /* SET FIELD TO TRUE */
			return vbuffMapFieldToIndex(i, j);
		}
	}

	vbuffCreateBufferFullMsg(object);
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
VAPI vHNDL vCreateBufferBehavior(const char* name, SIZE_T elementSize,
	vI32 elementCount, vPFBUFFINITIALIZER elementInitCallback,
	vPFBUFFDESTRUCTOR  elementDestroyCallback)
{
	/* CRITICAL SECT ENTER */ vbuffCaptureRWPermission();

	if (_vcore->bufferHandler.behaviorCount >= MAX_BUFFER_BEHAVIORS)
		vCoreCreateFatalError("Buffer Behaviors Full");

	vPBufferBehavior bhvPtr = _vcore->bufferHandler.behaviors + _vcore->bufferHandler.behaviorCount;

	/* set members */
	__movsb(bhvPtr->name, name, strlen(name));
	bhvPtr->initializer = elementInitCallback;
	bhvPtr->destructor  = elementDestroyCallback;
	bhvPtr->elementSizeBytes = elementSize;
	bhvPtr->elementCount  = elementCount;
	bhvPtr->fieldChunkCount	= (elementCount >> 0x06) + 1;

	
	/* log creation */
	sprintf_s(__remarkBuffer, sizeof(__remarkBuffer), "NAME: %s\nINDEX: %d\nSIZE: %d\n"
		"LENGTH: %d\n", bhvPtr->name, _vcore->bufferHandler.behaviorCount,
		(vI32)elementSize, elementCount);
	vLogAction("Created Buffer Behavior", __remarkBuffer);

	_vcore->bufferHandler.behaviorCount++; /* INCREMENT BUFFERBHV COUNT */

	/* CRITICAL SECT LEAVE */ vbuffReleaseRWPermission();

	return _vcore->bufferHandler.behaviorCount - 1;
}

/* creates a buffer object to holds things within.			 */
VAPI vHNDL vCreateBuffer(const char* name, vHNDL behavior)
{
	/* CRITICAL SECT ENTER */ vbuffCaptureRWPermission();

	vPBufferBehavior bhv = _vcore->bufferHandler.behaviors + behavior;

	/* allocate buffer object to heap */
	vI32 buffIndex = vbuffFindFreeBufferObject();
	_vcore->bufferHandler.buffers[buffIndex] = vAllocZeroed(sizeof(vBufferObject));
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffIndex];

	/* copy name */
	__movsb(buff->name, name, strlen(name));

	/* allocate field and data members */
	buff->field = vAllocZeroed(sizeof(vUI64) * bhv->fieldChunkCount);
	buff->data  = vAllocZeroed((bhv->elementSizeBytes * bhv->elementCount) + 
		BUFFER_MEMORY_PADDING_BYTES);
	buff->behavior = bhv;

	/* initialize buffer rw mutex */
	InitializeCriticalSection(&buff->rwPermission);

	_vcore->bufferHandler.bufferCount++;

	/* log buffer creation */
	sprintf_s(__remarkBuffer, sizeof(__remarkBuffer),
		"NAME: %s\nBEHAVIOR:%s\nINDEX:%d\n", buff->name, bhv->name,
		buffIndex);
	vLogAction("Created Buffer", __remarkBuffer);

	/* CRITICAL SECT LEAVE */ vbuffReleaseRWPermission();

	return buffIndex;
}

/* destroys a buffer object									 */
VAPI vBOOL vDestroyBuffer(vHNDL bufferHndl)
{
	/* CRITICAL SECT ENTER */ vbuffCaptureRWPermission();

	vPBufferObject buff = _vcore->bufferHandler.buffers[bufferHndl];
	if (buff == NULL) return FALSE;

	/* log buffer deletion */
	sprintf_s(__remarkBuffer, sizeof(__remarkBuffer),
		"NAME: %s\nBEHAVIOR: %s\nINDEX: %d\n", buff->name, buff->behavior->name,
		bufferHndl);
	vLogAction("Destroying Buffer", __remarkBuffer);

	/* regain ownership of buff RW and call destructor on everything */
	EnterCriticalSection(&buff->rwPermission);
	if (buff->behavior->destructor)
	{
		for (int i = 0; i < buff->behavior->elementCount; i++)
		{
			vI32 chunk, bit;
			vbuffMapIndexToField(i, &chunk, &bit);

			/* if already destroyed, continue */
			if (_bittest64(&buff->field[chunk], bit) == ZERO) continue;

			buff->behavior->destructor(bufferHndl, i, 
				buff->data + (i * buff->behavior->elementSizeBytes));
		}
	}

	/* delete the critical section */
	DeleteCriticalSection(&buff->rwPermission);

	vFree(buff->field);
	vFree(buff->data);
	vFree(buff);

	_vcore->bufferHandler.bufferCount--;
	_vcore->bufferHandler.buffers[bufferHndl] = NULL;

	/* CRITICAL SECT LEAVE */ vbuffReleaseRWPermission();

	return TRUE;
}


/* ========== BUFFER ELEMENT OPERATIONS			==========	*/

/* finds an empty spot in the buffer and returns the		*/
/* pointer to that element for the user to store or modify  */
/* NOT TO BE USED WHEN MULTITHREADING!!						*/
VAPI vPTR vBufferAdd(vHNDL buffer)
{
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffer];
	if (buff == NULL) 
	{
		vLogWarning("Buffer Add Failed", "Tried to add to a buffer that doesn't exist.");
		return NULL;
	}

	/* CRITICAL SECT ENTER */ EnterCriticalSection(&buff->rwPermission);

	vI32 buffIndex = vbuffFindFreeBufferObjectSpot(buff);

	/* clear element */
	vPTR elemPtr = buff->data + (buffIndex * buff->behavior->elementSizeBytes);
	__stosb(elemPtr, ZERO, buff->behavior->elementSizeBytes);

	buff->usedElementCount++; /* increment used elem count */
	
	if (buff->behavior->initializer) /* invoke initializer */
		buff->behavior->initializer(buffer, buffIndex, elemPtr);

	/* CRITICAL SECT LEAVE */ LeaveCriticalSection(&buff->rwPermission);

	return elemPtr;
}

/* finds an empty spot in the buffer and then performs the	*/
/* operation specified (if any) to the element.				*/
VAPI void vBufferAddSafe(vHNDL buffer, vPFBUFFOPERATION operation)
{
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffer];
	if (buff == NULL)
	{
		vLogWarning("Buffer AddSafe Failed", "Tried to add to a buffer that doesn't exist.");
		return;
	}
	if (operation == NULL)
	{
		vLogWarning("Buffer AddSafe Failed", "No operation was specified.");
		return;
	}

	/* CRITICAL SECT ENTER */ EnterCriticalSection(&buff->rwPermission);

	vI32 buffIndex = vbuffFindFreeBufferObjectSpot(buff);

	/* clear element */
	vPTR elemPtr = buff->data + (buffIndex * buff->behavior->elementSizeBytes);
	__stosb(elemPtr, ZERO, buff->behavior->elementSizeBytes);

	buff->usedElementCount++; /* increment used elem count */

	if (buff->behavior->initializer) /* invoke initializer first */
		buff->behavior->initializer(buffer, buffIndex, elemPtr);

	operation(buffer, buffIndex, elemPtr);   /* apply operation next */

	/* CRITICAL SECT LEAVE */ LeaveCriticalSection(&buff->rwPermission);
}

/* returns a pointer to an element in the buffer specified  */
/* by an index. NOT TO BE USED WHEN MULTITHREADING!!		*/
VAPI vPTR vBufferGet(vHNDL buffer, vI32 index)
{
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffer];
	if (buff == NULL)
	{
		vLogWarning("Buffer Get Failed", "Tried to get from a buffer that doesn't exist.");
		return NULL;
	}

	return buff->data + (index * buff->behavior->elementSizeBytes);
}

/* finds the element in the buffer specified by an index    */
/* performs the function passed on it						*/
VAPI void vBufferOperate(vHNDL buffer, vI32 index, vPFBUFFOPERATION operation)
{
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffer];
	if (buff == NULL)
	{
		vLogWarning("Buffer Operate Failed", "Tried to get from a buffer that doesn't exist.");
		return;
	}
	if (operation == NULL)
	{
		vLogWarning("Buffer Operate Failed", "No operation specified.");
		return;
	}
	if (index > buff->behavior->elementCount)
	{
		vLogWarning("Buffer Operate Failed", "Invalid Index.");
		return;
	}

	/* check for not active */
	vI32 chunk, bit;
	vbuffMapIndexToField(index, &chunk, &bit);
	if (_bittest64(&buff->field[chunk], bit) == FALSE)
	{
		vLogWarning("Buffer Operate Failed", "Inactive element");
		return;
	}

	vPTR elemPtr = buff->data + (index * buff->behavior->elementSizeBytes);

	/* CRITICAL SECT ENTER */ EnterCriticalSection(&buff->rwPermission);
	operation(buffer, index, elemPtr);
	/* CRITICAL SECT LEAVE */ LeaveCriticalSection(&buff->rwPermission);
}

/* finds the element within the buffer by index and removes */
/* it, applying the destructor function if any.				*/
VAPI void vBufferRemoveIndex(vHNDL buffer, vI32 index)
{
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffer];
	if (buff == NULL)
	{
		vLogWarning("Buffer Remove Failed", "Tried to remove from a buffer that doesn't exist.");
		return;
	}
	if (index >= buff->behavior->elementCount)
	{
		vLogWarning("Buffer Remove Failed", "Tried to remove an invalid index.");
		return;
	}

	vI32 chunk, bit;
	vbuffMapIndexToField(index, &chunk, &bit);

	/* check for already removed */
	if (_bittest64(&buff->field[chunk], bit) == FALSE)
	{
		vLogWarning("Buffer Remove Failed", "Tried to remove an inactive element.");
		return;
	}

	/* CRITICAL SECT ENTER */ EnterCriticalSection(&buff->rwPermission);

	/* apply destructor if any */
	if (buff->behavior->destructor)
		buff->behavior->destructor(buffer, index, 
			buff->data + (index * buff->behavior->elementSizeBytes));

	_bittestandreset64(&buff->field[chunk], bit);
	buff->usedElementCount--;

	/* CRITICAL SECT LEAVE */ LeaveCriticalSection(&buff->rwPermission);
}

/* loops through every element and performs the specified	*/
/* operation on it. buffer may be synced depending on bhv	*/
VAPI void vBufferIterate(vHNDL buffer, vPFBUFFITERATOR operation)
{
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffer];
	if (buff == NULL)
	{
		vLogWarning("Buffer Iterate Failed", "Tried to iterate over a buffer that doesn't exist.");
		return;
	}
	if (operation == NULL)
	{
		vLogWarning("Buffer Iterate Failed", "Operation not specified.");
		return;
	}

	for (int i = 0; i < buff->behavior->elementCount; i++)
	{
		vI32 chunk, bit;
		vbuffMapIndexToField(i, &chunk, &bit);

		/* CRITICAL SECT ENTER */ EnterCriticalSection(&buff->rwPermission);

		if (_bittest64(&buff->field[chunk], bit) == ZERO) continue;

		operation(buffer, i, buff->data + (i * buff->behavior->elementSizeBytes));

		/* CRITICAL SECT LEAVE */ LeaveCriticalSection(&buff->rwPermission);
	}
}

/* returns whether a given index is marked as used.			*/
VAPI vBOOL vBufferIsIndexInUse(vHNDL buffer, vI32 index)
{
	vPBufferObject buff = _vcore->bufferHandler.buffers[buffer];
	if (buff == NULL)
	{
		vLogWarning("Buffer Is Index In Use Failed", "Tried to check a buffer that doesn't exist.");
		return;
	}

	/* CRITICAL SECT ENTER */ EnterCriticalSection(&buff->rwPermission);

	vI32 chunk, bit;
	vbuffMapIndexToField(index, &chunk, &bit);
	vBOOL result = _bittest64(&buff->field[chunk], bit);

	/* CRITICAL SECT LEAVE */ LeaveCriticalSection(&buff->rwPermission);

	return result;
}