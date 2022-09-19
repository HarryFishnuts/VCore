
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
	/* alloc node to heap */
	vPDBufferNode node = vAllocZeroed(parent->elementSizeBytes);

	/* setup node members */
	node->parent   = parent;
	node->useField = vAllocZeroed((DBUFFER_NODE_CAPACITY >> 0x03) + 1);
	node->block    = vAllocZeroed(DBUFFER_NODE_CAPACITY * parent->elementSizeBytes);

	return node;
}

static __forceinline void vhDestroyBufferNode(vPDBufferNode node)
{
	vFree(node->useField);
	vFree(node->block);
	vFree(node);
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

static __forceinline vUI32 vhFindBufferNodeFreeIndex(vPDBufferNode node)
{
	/* find unused */
	for (int i = 0; i < DBUFFER_NODE_CAPACITY; i++)
	{
		vUI64 chunk, bit;
		vhMapIndexToUseField(i, &chunk, &bit);
		if (_bittest64(&node->useField[chunk], bit) == FALSE)
		{
			/* set used and return index */
			_bittestandset64(&node->useField[chunk], bit);
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

	return index;
}

VAPI vBOOL vDestroyDBuffer(vHNDL dBuffer)
{
	if (dBuffer < 0 || dBuffer > MAX_DBUFFERS) return FALSE;

	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];
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

	return TRUE;
}


/* ========== SYNCHRONIZATION					==========	*/
VAPI void vDBufferLock(vHNDL dBuffer)
{
	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];
	EnterCriticalSection(&buffer->rwPermission);
}
VAPI void vDBufferUnlcok(vHNDL dBuffer)
{
	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];
	LeaveCriticalSection(&buffer->rwPermission);
}


/* ========== ELEMENT MANIPULATION				==========	*/
VAPI vPTR vDBufferAdd(vHNDL dBuffer)
{
	vPDBuffer buffer = &_vcore.dbuffers[dBuffer];

	/* try add to each node */
	vPDBufferNode currentNode = buffer->head;

	while(TRUE)
	{
		/* try to add to current Node */
		vUI32 freeIndex = vhFindBufferNodeFreeIndex(currentNode);

		printf("node index: %d\n", freeIndex);

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
		return element;
	}

	/* SHOULD NEVER REACH HERE! */
	vLogError(__func__, "Unexpected error while trying to add to dynamic buffer.");
	vCoreCrash();
}

VAPI void vDBufferRemove(vHNDL dBuffer, vPTR element)
{
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
			_bittestandreset64(&node->useField[chunk], bit);
		}

		/* move to next block */
		node = node->next;
	}
}

VAPI void vDBufferIterate(vHNDL buffer, vPFBUFFERITERATEFUNC function);

VAPI void vDBufferClear(vHNDL buffer);