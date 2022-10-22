
/* ========== <vobject.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/

/* ========== INCLUDES							==========	*/
#include "vobject.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vPosition  vCreatePosition(float x, float y)
{
	vPosition rPos;
	rPos.x = x;
	rPos.y = y;
	return rPos;
}

VAPI vTransform vCreateTransform(vPosition pos, float r, float s)
{
	vTransform transform;
	transform.position = pos;
	transform.rotation = r;
	transform.scale = s;
	return transform;
}

VAPI vTransform vCreateTransformF(float x, float y, float r, float s)
{
	vTransform transform;
	transform.position.x = x;
	transform.position.y = y;
	transform.rotation = r;
	transform.scale = s;
	return transform;
}

VAPI vPObject   vCreateObject(vTransform transform, vPObject parent)
{
	vDBufferLock(_vcore.objects);

	vPObject object = vDBufferAdd(_vcore.objects, NULL);
	InitializeCriticalSection(&object->lock);
	object->transform = transform;
	object->parent = parent;

	vDBufferUnlock(_vcore.objects);

	return object;
}

VAPI void       vDestroyObject(vPObject object)
{
	vDBufferLock(_vcore.objects);
	
	EnterCriticalSection(&object->lock);
	DeleteCriticalSection(&object->lock);

	/* destroy all components */
	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;
		if (comp->objectAttribute == NULL) continue;
		
		vPComponentDescriptor cDesc = _vcore.components + comp->componentDescriptorHandle;
		if (cDesc->objectDestroyFunc)
			cDesc->objectDestroyFunc(object, comp);
	}

	vDBufferRemove(_vcore.objects, object);

	vDBufferUnlock(_vcore.objects);
}


/* ========== COMPONENT CREATION				==========	*/
VAPI vUI16 vCreateComponent(vPCHAR name, vUI64 staticSize, vUI64 objectSize,
	vPFCOMPONENTINITIALIZATIONSTATIC staticInitialization,
	vPFCOMPONENTINITIALIZATION initialization, vPFCOMPONENTDESTRUCTION destruction,
	vPFCOMPONENTCYCLE cycle, vPWorker cycleWorker)
{
	vCoreLock();

	/* send warning if component name is already taken */
	if (vComponentGetHandleByName(name) != ZERO)
	{
		vLogWarningFormatted(__func__, "Creating Component '%s' with name that is already taken!\n",
			name);
	}

	/* find free component descriptor */
	for (int i = 0; i < COMPONENTS_MAX; i++)
	{
		vPComponentDescriptor compD = _vcore.components + i;

		/* skip if used */
		if (compD->inUse == TRUE) continue;

		/* initialize descriptor members */
		vMemCopy(compD->componentName, name, min(BUFF_SMALL, strlen(name)));
		compD->staticAttributeSize = staticSize;
		compD->objectAttributeSize = objectSize;
		compD->staticInitFunc = staticInitialization;
		compD->objectInitFunc = initialization;
		compD->objectDestroyFunc = destruction;
		compD->objectCycleWorker = cycleWorker;
		compD->objectCycleFunc   = cycle;
		
		/* allocate static block, do callback (if exists) */
		compD->staticAttribute = vAllocZeroed(max(4, compD->staticAttributeSize));
		if (compD->staticInitFunc)
			compD->staticInitFunc(compD, compD->staticAttribute);

		/* mark component descriptor as used */
		compD->inUse = TRUE;

		vLogInfoFormatted(__func__, "Created Component '%s' "
			"with static size %d and object size %d.", compD->componentName,
			compD->staticAttributeSize, compD->objectAttributeSize);

		vCoreUnlock();

		/* RETURN HANDLE */
		return i;
	}

	/* fail if reached, components are all used up */
	vLogError(__func__, "Could not create more components. Max components have been created.");
	vDumpEntryBuffer();
	vCoreFatalError(__func__, "Could not create more components. Max components have been created.");
}

VAPI vUI16 vComponentGetHandleByName(vPCHAR name)
{
	vCoreLock();

	for (int i = 0; i < COMPONENTS_MAX; i++)
	{
		vPComponentDescriptor compD = _vcore.components + i;
		if (compD->inUse == FALSE) continue;

		if (strcmp(name, compD->componentName) != 0) continue;

		vCoreUnlock();
		return i;
	}

	/* return 0 on fail */
	vCoreUnlock();
	return 0;
}

VAPI vBOOL vComponentGetNameByHandle(vUI16 handle, vPCHAR nameBuffer, vUI32 bufferLength)
{
	vCoreLock();

	for (int i = 0; i < COMPONENTS_MAX; i++)
	{
		vPComponentDescriptor compD = _vcore.components + i;
		if (compD->inUse == FALSE) continue;

		/* on matching, copy and return copied amount for sucess */
		if (handle == i)
		{
			int copyAmountBytes = min(strlen(compD->componentName), bufferLength);
			vMemCopy(nameBuffer, compD->componentName, copyAmountBytes);

			vCoreUnlock();
			return copyAmountBytes;
		}
	}

	/* return 0 on fail */
	vCoreUnlock();
	return 0;
}

VAPI vPTR  vComponentGetStaticPtr(vUI16 component)
{
	return _vcore.components[component].staticAttribute;
}

VAPI vPComponentDescriptor vComponentGetDescriptor(vUI16 component)
{
	return _vcore.components + component;
}


/* ========== OBJECT SYNCHRONIZATION			==========	*/
VAPI void vObjectGlobalLock(void)
{
	vDBufferLock(_vcore.objects);
}

VAPI void vObjectGlobalUnlock(void)
{
	vDBufferUnlock(_vcore.objects);
}

VAPI void vObjectLock(vPObject object)
{
	EnterCriticalSection(&object->lock);
}

VAPI void vObjectUnlock(vPObject object)
{
	LeaveCriticalSection(&object->lock);
}


/* ========== OBJECT COMPONENT MANIPULATION		==========	*/
VAPI vPComponent vObjectAddComponent(vPObject object, vUI16 component, vPTR input)
{
	/* don't add if already existing */
	if (vObjectHasComponent(object, component)) return FALSE;

	vPComponentDescriptor desc = _vcore.components + component;

	EnterCriticalSection(&object->lock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if used, skip */
		if (comp->objectAttribute != NULL) continue;

		comp->componentDescriptorHandle = component;
		comp->staticAttribute = desc->staticAttribute;
		comp->objectAttribute = vAllocZeroed(max(4, desc->objectAttributeSize));

		/* if object has a worker to do it's cycle, attach to list */
		if (desc->objectCycleWorker != NULL)
		{
			vWorkerComponentCycleData cycleData;
			cycleData.component = comp;
			cycleData.cycleFunc = desc->objectCycleFunc;

			/* grab cycle data pointer */
			comp->cycleDataPtr = 
				vDBufferAdd(desc->objectCycleWorker->componentCycleList, &cycleData);
		}

		/* call init callback if possible */
		if (desc->objectInitFunc)
			desc->objectInitFunc(object, comp, input);
		
		LeaveCriticalSection(&object->lock);
		return comp;
	}

	vLogWarningFormatted(__func__, "Cannot add any more components to object '%p'.",
		object);
	LeaveCriticalSection(&object->lock);
	return NULL;
}

VAPI vBOOL vObjectRemoveComponent(vPObject object, vUI16 component)
{
	vPComponentDescriptor desc = _vcore.components + component;

	EnterCriticalSection(&object->lock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if unused, skip */
		if (comp->objectAttribute == NULL) continue;

		/* if of not same type, skip */
		if (comp->componentDescriptorHandle != component) continue;

		/* call destruction callback if possible */
		if (desc->objectDestroyFunc)
			desc->objectDestroyFunc(object, comp);

		/* remove cycle data from worker */
		if (desc->objectCycleWorker != NULL)
			vDBufferRemove(desc->objectCycleWorker->componentCycleList, comp->cycleDataPtr);

		/* free object attribute memory */
		vFree(comp->objectAttribute);
		
		/* zero component memory */
		vZeroMemory(comp, sizeof(vComponent));

		LeaveCriticalSection(&object->lock);
		return TRUE;
	}

	vLogWarningFormatted(__func__, "Could not remove component '%d' from object '%p'.",
		component, object);
	LeaveCriticalSection(&object->lock);
	return FALSE;
}

VAPI vBOOL vObjectHasComponent(vPObject object, vUI16 component)
{
	EnterCriticalSection(&object->lock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if unused, skip */
		if (comp->objectAttribute == NULL) continue;

		/* if not matching skip */
		if (comp->componentDescriptorHandle != component) continue;
		
		/* on match, return true */
		LeaveCriticalSection(&object->lock);
		return TRUE;
	}

	LeaveCriticalSection(&object->lock);
	return FALSE;
}

VAPI vPComponent vObjectGetComponent(vPObject object, vUI16 component)
{
	EnterCriticalSection(&object->lock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if used, skip */
		if (comp->objectAttribute != NULL) continue;

		/* if unmatching, skip */
		if (comp->componentDescriptorHandle != component) continue;

		/* on match, return ptr */
		LeaveCriticalSection(&object->lock);
		return comp;
	}

	LeaveCriticalSection(&object->lock);
	return NULL;
}

VAPI vUI32 vObjectGetComponentCount(vPObject object)
{
	EnterCriticalSection(&object->lock);

	vUI32 counter = 0;
	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if used, increment counter */
		if (comp->objectAttribute != NULL) counter++;
	}

	LeaveCriticalSection(&object->lock);
	return counter;
}