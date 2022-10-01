
/* ========== <vdbuffers.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/

/* ========== INCLUDES							==========	*/
#include "vdbuffers.h"
#include <intrin.h>


/* ========== HELPER							==========	*/
static __forceinline vUI32 vhFindFreeDBufferIndex(void)
{
	/* loop and check each's in use flag. on free, return	*/
	for (vUI32 i = 0; i < MAX_BUFFERS; i++)
	{
		if (_vcore.dbuffers[i].inUse) continue;
		return i;
	}

	/* on reach here, all buffers are used. log and exit	*/
	vLogError(__func__, "Trying to create new dynamic buffer, but "
		"maximum amount of dynamic buffers have been created. "
		"Error is unrecoverable and process will be terminated.");
	vCoreFatalError(__func__, "Could not create more dynamic buffers.");
}

static __forceinline vPDBufferNode vhCreateBufferNode(vPDBuffer parent)
{
	EnterCriticalSection(&parent->rwPermission);

	vUI64 nodeSize = sizeof(vDBufferNode);
	vUI64 useFieldSize = (((vUI64)parent->nodeSize >> 0x06) + 1) * sizeof(vUI64);
	vUI64 blockSize = parent->nodeSize * parent->elementSizeBytes;

	/* alloc node to heap */
	vPDBufferNode node = vAllocZeroed(nodeSize + useFieldSize + blockSize);

	node->parent   = parent;
	node->useField = (vPBYTE)(node) + nodeSize;
	node->block =    (vPBYTE)(node->useField) + useFieldSize;

	LeaveCriticalSection(&parent->rwPermission);

	return node;
}

static __forceinline void vhDestroyBufferNode(vPDBufferNode node)
{
	EnterCriticalSection(&node->parent->rwPermission);

	vFree(node);

	LeaveCriticalSection(&node->parent->rwPermission);
}

static __forceinline void vhMapIndexToUseField(vUI64 index, vPUI64 chunk, vPUI64 bit)
{
	*chunk = (index >> 0x06);
	*bit = (index & 0b111111);
}

static __forceinline vUI64 vhMapUseFieldToIndex(vUI64 chunk, vUI64 bit)
{
	return (chunk << 0x06) + bit;
}

static __forceinline vUI32 vhFindFreeBufferNodeIndex(vPDBufferNode node)
{
	/* if node is full, return */
	if (node->elementCount >= node->parent->nodeSize) return ~0;

	/* find unused */
	for (vUI64 i = 0; i < node->parent->nodeSize; i++)
	{
		vUI64 chunk, bit;
		vhMapIndexToUseField(i, &chunk, &bit);
		if (_bittest64(node->useField + chunk, bit) == FALSE)
		{
			/* set used and return index */
			_bittestandset64(node->useField + chunk, bit);

			node->elementCount++; /* increment element count */

			return i;
		}
			
	}

	/* on nothing found, return -1 */
	return ~0;
}


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vHNDL vCreateDBuffer(const char* dBufferName, vUI16 elementSize,
	vUI32 nodeSize, vPFDBUFFERINITIALIZEELEMENT initializeFunc,
	vPFDBUFFERDESTROYELEMENT destroyFunc)
{
	vCoreLock(); /* SYNC */

	/* get buffer */
	vUI32 index = vhFindFreeDBufferIndex();
	vPDBuffer dBuffer = _vcore.dbuffers + index;
	vZeroMemory(dBuffer, sizeof(vDBuffer));

	/* initialize name */
	if (dBufferName == NULL)
	{
		vLogWarning(__func__, "Created dynamic buffer with no name.");
	}
	else
	{
		vMemCopy(dBuffer->name, dBufferName, strlen(dBufferName));
	}

	/* setup fields */
	dBuffer->inUse = 1;
	dBuffer->elementSizeBytes = elementSize;
	dBuffer->nodeSize = nodeSize;
	vCoreTime(&dBuffer->timeCreated);
	InitializeCriticalSection(&dBuffer->rwPermission);

	/* setup callbacks */
	dBuffer->initializeFunc = initializeFunc;
	dBuffer->destroyFunc    = destroyFunc;

	/* setup first node */
	dBuffer->head = vhCreateBufferNode(dBuffer);
	dBuffer->tail = dBuffer->head;

	vCoreUnlock(); /* UNSYNC */

	vLogInfoFormatted(__func__,
		"Dynamic buffer '%s' created with "
		"element size %d.",
		dBuffer->name, dBuffer->elementSizeBytes);

	return index;
}

VAPI vBOOL vDestroyDBuffer(vHNDL dBuffer)
{
	if (dBuffer < 0 || dBuffer > MAX_DBUFFERS) return FALSE;

	vCoreLock();

	vPDBuffer buffer = _vcore.dbuffers + dBuffer;

	/* check if already destroyed */
	if (buffer->inUse == FALSE) 
	{
		vLogError(__func__, "Tried to destroy dynamic buffer which doesn't exist.");
		return FALSE;
	}

	/* lock buffer */
	vDBufferLock(dBuffer);

	/* clear buffer */
	vDBufferClear(dBuffer);
		
	/* mark as destroyed */
	buffer->inUse = FALSE;

	/* destroy all nodes */
	vPDBufferNode node = buffer->head;
	while (node != NULL)
	{
		/* toDestroy now points to current node */
		vPDBufferNode toDestroy = node;

		/* node now points to next node */
		node = toDestroy->next;

		/* destroy current node */
		vhDestroyBufferNode(toDestroy);
	}

	vLogInfoFormatted(__func__, "Destroyed dynamic buffer '%s'.", buffer->name);

	vCoreUnlock();

	return TRUE;
}


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vDBufferLock(vHNDL dBuffer)
{
	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];
	EnterCriticalSection(&buffer->rwPermission);
}

VAPI void vDBufferUnlock(vHNDL dBuffer)
{
	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];
	LeaveCriticalSection(&buffer->rwPermission);
}


/* ========== ELEMENT MANIPULATION				==========	*/
VAPI vPTR vDBufferAdd(vHNDL dBuffer, vPTR input)
{
	vPDBuffer buffer = _vcore.dbuffers + dBuffer;

	vDBufferLock(dBuffer); /* SYNC */

	/* try add to each node */
	vPDBufferNode currentNode = buffer->head;

	while(TRUE)
	{
		/* try to add to current Node */
		vUI32 freeIndex = vhFindFreeBufferNodeIndex(currentNode);

		/* on no free, go to next */
		if (freeIndex == ~0)
		{
			vPDBufferNode nextNode = currentNode->next;

			/* create node if missing */
			if (nextNode == NULL)
			{
				nextNode = vhCreateBufferNode(buffer);
				currentNode->next = nextNode;
				buffer->tail = nextNode;
			}

			currentNode = nextNode;

			continue;
		}

		/* on valid index, return PTR */
		vPBYTE element = (vPBYTE)(currentNode->block) + ((buffer->elementSizeBytes) * freeIndex);
		vZeroMemory(element, buffer->elementSizeBytes);
		buffer->elementCount++;

		/* call initialization func (if exists) */
		if (buffer->initializeFunc)
			buffer->initializeFunc(dBuffer, element, input);

		vDBufferUnlock(dBuffer); /* UNSYNC */

		return element;
	}

	/* SHOULD NEVER REACH HERE! */
	vLogError(__func__, "Unexpected error while trying to add to dynamic buffer.");
	vCoreFatalError(__func__, "Unexpected error while trying to add to dynamic buffer.");
}

VAPI void vDBufferRemove(vHNDL dBuffer, vPTR element)
{
	vDBufferLock(dBuffer);

	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];

	vPDBufferNode node = buffer->head;

	/* iterate all nodes */
	while (node != NULL)
	{
		if ((vPBYTE)element >= (vPBYTE)node->block && 
			(vPBYTE)element <= (vPBYTE)node->block + 
			(buffer->elementSizeBytes * buffer->nodeSize))
		{
			/* call destruction func (if exists) */
			if (buffer->destroyFunc)
				buffer->destroyFunc(dBuffer, element);

			/* get index using ptr math */
			vUI32 nodeIndex = ((vPBYTE)element - (vPBYTE)node->block) / buffer->elementSizeBytes;

			/* get bitfield chunk & bit using index */
			vUI64 chunk, bit;
			vhMapIndexToUseField(nodeIndex, &chunk, &bit);

			/* set bit to be unused */
			_bittestandreset64(node->useField + chunk, bit);

			/* decrement element count */
			node->elementCount -= 1;

			vDBufferUnlock(dBuffer);

			return; /* end */
		}

		/* move to next block */
		node = node->next;
	}
}

VAPI void vDBufferIterate(vHNDL dBuffer, vPFDBUFFERITERATEFUNC function, vPTR input)
{
	vDBufferLock(dBuffer);
	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];

	/* walk every node */
	vPDBufferNode node = buffer->head;
	while (node != NULL)
	{
		/* check every element */
		for (vUI64 i = 0; i < buffer->nodeSize; i++)
		{
			vUI64 chunk, index;
			vhMapIndexToUseField(i, &chunk, &index);

			/* if unused, skip */
			if (_bittest64(&node->useField[chunk], index) == FALSE) continue;

			function(dBuffer, (vPBYTE)node->block + (i * buffer->elementSizeBytes), input);
		}

		node = node->next;
	}

	vDBufferUnlock(dBuffer);
}

VAPI void vDBufferClear(vHNDL dBuffer) 
{
	vDBufferLock(dBuffer);
	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];

	vPDBufferNode node = buffer->head;

	/* walk all nodes and set their fields to NULL */
	while (node != NULL)
	{
		for (vUI64 i = 0; i < buffer->nodeSize; i++)
		{
			vUI64 chunk, bit;
			vhMapIndexToUseField(i, &chunk, &bit);

			/* if index is empty, skip */
			if (_bittest64(node->useField + chunk, bit) == FALSE) continue;

			/* get element pointer and destroy (if possible) */
			vPBYTE element = (vPBYTE)node->block + (i * buffer->elementSizeBytes);
			if (buffer->destroyFunc)
				buffer->destroyFunc(dBuffer, element);

			node->elementCount--;
			buffer->elementCount--;
			
			/* set flag to unused */
			_bittestandreset64(node->useField + chunk, bit);
		}

		node = node->next;
	}

	vDBufferUnlock(dBuffer);
}


/* ========== BUFFER INFORMATION				==========	*/
VAPI vUI32 vDBufferGetElementCount(vHNDL dBuffer)
{
	vPDBuffer buffer = _vcore.dbuffers + dBuffer;
	return buffer->elementCount;
}