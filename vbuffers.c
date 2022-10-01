
/* ========== <vbuffers.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "vbuffers.h"
#include <intrin.h>


/* ========== HELPER							==========	*/
static __forceinline vUI32 vhFindFreeBufferIndex(void)
{
	/* loop and check each's in use flag. on free, return	*/
	for (vUI32 i = 0; i < MAX_BUFFERS; i++)
	{
		if (_vcore.buffers[i].inUse) continue;
		return i;
	}

	/* on reach here, all buffers are used. log and exit	*/
	vLogError(__func__, "Trying to create new buffer, but "
		"maximum amount of buffers have been created. "
		"Error is unrecoverable and process will be terminated.");
	vCoreFatalError(__func__, "Could not create more buffers.");
}

static __forceinline void vhMapIndexToUseField(vUI64 index, vPUI64 chunk, vPUI64 bit)
{
	*chunk	= (index >> 0x06    );
	*bit	= (index &  0b111111);
}

static __forceinline vUI64 vhMapUseFieldToIndex(vUI64 chunk, vUI64 bit)
{
	return (chunk << 0x06) + bit;
}

static __forceinline vUI16 vhMapPtrToBufferIndex(vPBuffer buffer, vPTR ptr)
{
	return ((vPBYTE)ptr - buffer->data) / buffer->elementSizeBytes;
}

static __forceinline vPBuffer vhGetBufferLocked(vHNDL bufHndl)
{
	/* SYNC		*/ vCoreLock();
	vPBuffer buff = _vcore.buffers + bufHndl;
	/* UNSYNC	*/ vCoreUnlock();
	return buff;
}

/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vHNDL CreateBuffer(const char* bufferName, vUI16 elementSize,
	vUI16 capacity, vPFBUFFERINITIALIZEELEMENT initializeFunc,
	vPFBUFFERDESTROYELEMENT destroyFunc)
{
	/* SYNC		*/ vCoreLock();

	/* find new buffer */
	vUI32 bufferIndex = vhFindFreeBufferIndex();
	vPBuffer buffer = _vcore.buffers + bufferIndex;
	vZeroMemory(buffer, sizeof(vBuffer));

	/* initialize name */
	if (bufferName == NULL)
	{
		vLogWarning(__func__, "Created buffer with no name.");
	}
	else
	{
		vMemCopy(buffer->name, bufferName, min(BUFF_SMALL - 1, strlen(bufferName)));
	}

	/* initialize element related data */
	InitializeCriticalSection(&buffer->rwPermission);
	vCoreTime(&buffer->timeCreated);
	buffer->elementSizeBytes = elementSize;
	buffer->capacity	= capacity;
	buffer->sizeBytes   = buffer->elementSizeBytes * buffer->capacity;
	buffer->inUse		= TRUE;

	/* allocate memory for field and data */
	buffer->useFieldLength  = (buffer->sizeBytes >> 0x03) + 1;
	buffer->useField		= vAllocZeroed(sizeof(vUI64) * buffer->useFieldLength);
	buffer->data			= vAllocZeroed(buffer->sizeBytes);

	/* setup callbacks */
	buffer->initializeFunc = initializeFunc;
	buffer->destroyFunc    = destroyFunc;

	/* log buffer creation */
	vLogInfoFormatted(__func__,
		"Buffer '%s' created with "
		"element size %d and capacity %d.",
		buffer->name, buffer->elementSizeBytes, buffer->capacity);

	/* UNSYNC	*/ vCoreUnlock();

	return bufferIndex;
}

VAPI vBOOL vDestroyBuffer(vHNDL buffHndl)
{
	/* SYNC		*/ vCoreLock();

	/* get buffer and check if valid */
	vPBuffer buffer = vhGetBufferLocked(buffHndl);
	if (buffer->inUse == FALSE)
	{
		vLogWarning(__func__, "Tried to destroy buffer which does not exist.");
		return FALSE;
	}

	/* wait for buffer to finish */
	vBufferLock(buffHndl);

	/* destroy all elements */
	for (int i = 0; i < buffer->capacity; i++)
	{
		vBufferRemoveIndex(buffHndl, i);
	}

	/* free all memory */
	vFree(buffer->data);
	vFree(buffer->useField);
	
	/* delete buffer sync object */
	DeleteCriticalSection(&buffer->rwPermission);
	buffer->inUse = FALSE;

	/* log buffer deletion */
	vLogInfoFormatted(__func__, "Destroyed buffer '%s'.",
		buffer->name);

	/* UNSYNC	*/ vCoreUnlock();

	return TRUE;
}


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vBufferLock(vHNDL buffer)
{
	EnterCriticalSection(&_vcore.buffers[buffer].rwPermission);
}

VAPI void vBufferUnlock(vHNDL buffer)
{
	LeaveCriticalSection(&_vcore.buffers[buffer].rwPermission);
}


/* ========== ELEMENT MANIPULATION				==========	*/
VAPI vPTR  vBufferAdd(vHNDL buffHndl, vPTR input)
{
	/* get buffer */
	vPBuffer buff = vhGetBufferLocked(buffHndl);

	/* SYNC		*/ vBufferLock(buffHndl);

	/* search for free index */
	vUI16 startIndex = buff->elementsUsed >> 0x1;
	for (vUI64 i = 0; i < buff->capacity; i++)
	{
		/* start at offset */
		vUI64 indexActual = (i + startIndex) % buff->capacity;

		vUI64 chunk, bit = 0;
		vhMapIndexToUseField(indexActual, &chunk, &bit);

		/* bittest to see if element is free */
		BOOLEAN result = _bittest64(&buff->useField[chunk], bit);
		if (result == TRUE) continue;

		/* on element free, set bit and increment use count */
		_bittestandset64(&buff->useField[chunk], bit);
		buff->elementsUsed++;

		/* get ptr and zero memory */
		vPTR elemPtr = buff->data + (indexActual * buff->elementSizeBytes);
		vZeroMemory(elemPtr, buff->elementSizeBytes);

		/* call initialization callback if it exists */
		if (buff->initializeFunc)
			buff->initializeFunc(buffHndl, indexActual, elemPtr, input);

		/* UNSYNC	*/ vBufferUnlock(buffHndl);
		
		return elemPtr;
	}

	/* on reach here, buffer is full		*/
	/* log and fatal err					*/
	vLogWarning(__func__, "Buffer has no more free elements.");
	vCoreFatalError(__func__, "Buffer has run out of free elements.");
}

VAPI void  vBufferRemove(vHNDL buffHndl, vPTR element)
{
	/* get buffer and element index */
	vPBuffer buff = vhGetBufferLocked(buffHndl);
	vUI16 elementIndex = vhMapPtrToBufferIndex(buff, element);

	/* SYNC		*/ vBufferLock(buffHndl);

	/* check if spot is already removed */
	vUI64 chunk, bit = 0;
	vhMapIndexToUseField(elementIndex, &chunk, &bit);
	if (_bittest64(&buff->useField[chunk], bit) == FALSE)
	{
		vLogWarning(__func__, "Tried to remove element that doesn't exist.");
	}
	else
	{
		/* call destruction function (if exists) */
		if (buff->destroyFunc)
			buff->destroyFunc(buffHndl, elementIndex, element);

		/* reset bit and decrement use count */
		_bittestandreset64(&buff->useField[chunk], bit);
		buff->elementsUsed--;
	}

	/* UNSYNC	*/ vBufferUnlock(buffHndl);
}

VAPI void  vBufferRemoveIndex(vHNDL buffHndl, vUI16 index)
{
	/* get buffer and element index */
	vPBuffer buff = vhGetBufferLocked(buffHndl);

	/* SYNC		*/ vBufferLock(buffHndl);

	/* check if spot is already removed */
	vUI64 chunk, bit = 0;
	vhMapIndexToUseField(index, &chunk, &bit);
	if (_bittest64(&buff->useField[chunk], bit) == FALSE)
	{
		vLogWarning(__func__, "Tried to remove element that doesn't exist.");
	}
	else
	{
		/* call destruction function (if exists) */
		if (buff->destroyFunc)
			buff->destroyFunc(buffHndl, index, 
				(vPBYTE)buff->data + (index * buff->elementSizeBytes));

		/* reset bit and decrement use count */
		_bittestandreset64(&buff->useField[chunk], bit);
		buff->elementsUsed--;
	}

	/* UNSYNC	*/ vBufferUnlock(buffHndl);
}

VAPI vUI16 vBufferGetElementIndex(vHNDL buffer, vPTR element)
{
	vPBuffer buff = vhGetBufferLocked(buffer);

	return vhMapPtrToBufferIndex(buff, element);
}

VAPI vPTR  vBufferGetIndex(vHNDL buffer, vUI16 index)
{
	vPBuffer buff = vhGetBufferLocked(buffer);

	if (buff->inUse == FALSE)
	{
		vLogError(__func__, "Tried to get index from buffer that doesn't exist");
		return NULL;
	}
	if (vBufferIndexUsed(buffer, index) == FALSE)
	{
		vLogError(__func__, "Tried to get index that was unused.");
		return NULL;
	}

	return buff->data + (index * buff->elementSizeBytes);
}

VAPI void  vBufferIterate(vHNDL buffHndl, vPFBUFFERITERATEFUNC function, vPTR input)
{
	if (function == NULL)
	{
		vLogWarning(__func__, "Tried to iterate over buffer with NULL function.");
		return;
	}

	vPBuffer buff = vhGetBufferLocked(buffHndl);

	/* SYNC		*/ vBufferLock(buffHndl);

	/* loop over all ACTIVE elements	*/
	vUI32 runCount = 0;
	for (vUI16 i = 0; i < buff->capacity; i++)
	{
		vUI64 chunk, bit = 0;
		vhMapIndexToUseField(i, &chunk, &bit);

		if (_bittest64(&buff->useField[chunk], bit) == FALSE) continue;

		function(buffHndl, i, buff->data + (i * buff->elementSizeBytes), input);
		
		/* once all active elements are processed, break */
		if (runCount++ >= buff->elementsUsed)	break;
	}

	/* UNSYNC	*/ vBufferUnlock(buffHndl);
}

VAPI vPTR  vBufferGetData(vHNDL buffer, PSIZE_T dataSize)
{
	vPBuffer buff = vhGetBufferLocked(buffer);

	if (dataSize != NULL)
		*dataSize = buff->sizeBytes;

	return buff->data;
}

VAPI vUI64 vBufferGetField(vHNDL buffer, PSIZE_T fieldSize)
{
	vPBuffer buff = vhGetBufferLocked(buffer);

	if (fieldSize != NULL)
		*fieldSize = buff->useFieldLength * sizeof(vUI64);

	return buff->useField;
}


/* ========== BUFFER INFORMATION				==========	*/
VAPI void vBufferGetInfo(vHNDL buffer, vPBufferInfo infoOut)
{
	vPBuffer buff = &_vcore.buffers[buffer];

	infoOut->capacity    = buff->capacity;
	infoOut->elementSize = buff->elementSizeBytes;
	infoOut->elementsUsed = buff->elementsUsed;
	infoOut->sizeBytes   = buff->sizeBytes;
	infoOut->timeCreated = buff->timeCreated;
	infoOut->name		 = buff->name;

	float usePercentNormalized = (float)buff->elementsUsed / (float)buff->capacity;
	infoOut->usePercentage = usePercentNormalized * 100.0f;
}

VAPI vBOOL vBufferExists(vHNDL buffer)
{
	vPBuffer buff = vhGetBufferLocked(buffer);
	return buff->inUse;
}

VAPI vBOOL vBufferIndexUsed(vHNDL buffer, vUI16 index)
{
	vPBuffer buff = vhGetBufferLocked(buffer);
	vUI64 chunk, bit;
	vhMapIndexToUseField(index, &chunk, &bit);
	return _bittest64(&buff->useField[chunk], bit);
}

