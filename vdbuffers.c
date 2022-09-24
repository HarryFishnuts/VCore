
/* ========== <vdbuffers.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/

/* ========== INCLUDES							==========	*/
#include "vdbuffers.h"
#include <stdio.h>
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

	/* alloc node to heap */
	vPDBufferNode node = vAllocZeroed(sizeof(vDBufferNode));

	printf("CREATED NEW: %p\n", node);

	/* setup node members */
	node->parent   = parent;
	int useFieldChunkCount = (DBUFFER_NODE_CAPACITY >> 0x03) + 1;
	node->useField = vAllocZeroed(useFieldChunkCount * sizeof(vPUI64));
	node->block    = vAllocZeroed(DBUFFER_NODE_CAPACITY * parent->elementSizeBytes);

	LeaveCriticalSection(&parent->rwPermission);

	return node;
}

static __forceinline void vhDestroyBufferNode(vPDBufferNode node)
{
	EnterCriticalSection(&node->parent->rwPermission);

	printf("node: %p\n", (vPBYTE)node - 8);
	printf("block: %p\n", (vPBYTE)(node->block) - 8);
	vFree(node->block);
	vFree(node->useField); puts("freed field");
	vFree(node); puts("freed node");

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
	if (node->elementCount >= DBUFFER_NODE_CAPACITY) return ~0;

	/* find unused */
	for (vUI64 i = 0; i < DBUFFER_NODE_CAPACITY; i++)
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
VAPI vHNDL vCreateDBuffer(const char* dBufferName, vUI16 elementSize)
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
	vCoreTime(&dBuffer->timeCreated);
	InitializeCriticalSection(&dBuffer->rwPermission);

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
VAPI vPTR vDBufferAdd(vHNDL dBuffer)
{
	vPDBuffer buffer = _vcore.dbuffers + dBuffer;

	vDBufferLock(dBuffer); /* SYNC */

	/* first, try last node */
	vUI32 freeIndex = vhFindFreeBufferNodeIndex(buffer->tail);
	if (freeIndex != ~0)
	{
		/* on valid index, return PTR */
		vPBYTE element = (vPBYTE)(buffer->tail->block) + ((buffer->elementSizeBytes) * freeIndex);
		vZeroMemory(element, buffer->elementSizeBytes);
		buffer->elementCount++;

		vDBufferUnlock(dBuffer); /* UNSYNC */

		return element;
	}

	/* IF THE LAST NODE IS FULL, TRY ALL PREVIOUS NODES.	*/
	/* IF ALL ARE FULL, THEN CREATE A NEW NODE				*/

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

		vDBufferUnlock(dBuffer); /* UNSYNC */

		return element;
	}

	/* SHOULD NEVER REACH HERE! */
	vLogError(__func__, "Unexpected error while trying to add to dynamic buffer.");
	vCoreCrash();
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
			(buffer->elementSizeBytes * DBUFFER_NODE_CAPACITY))
		{
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

VAPI void vDBufferIterate(vHNDL buffer, vPFBUFFERITERATEFUNC function);

VAPI void vDBufferClear(vHNDL buffer);


/* ========== BUFFER INFORMATION				==========	*/
VAPI vUI32 vDBufferGetElementCount(vHNDL dBuffer)
{
	vPDBuffer buffer = _vcore.dbuffers + dBuffer;
	return buffer->elementCount;
}