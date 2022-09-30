
/* ========== <vobject.c>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/

/* ========== INCLUDES							==========	*/
#include "vobject.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
vPosition  vCreatePosition(float x, float y)
{
	vPosition rPos;
	rPos.x = x;
	rPos.y = y;
	return rPos;
}

vTransform vCreateTransform(vPosition pos, float r, float s)
{
	vTransform transform;
	transform.position = pos;
	transform.rotation = r;
	transform.scale = s;
	return transform;
}

vPObject   vCreateObject(vTransform transform, vPObject parent)
{
	vDBufferLock(_vcore.objects);

	vPObject object = vDBufferAdd(_vcore.objects);
	InitializeCriticalSection(&object->componentLock);
	object->transform = transform;
	object->parent = parent;

	vDBufferUnlock(_vcore.objects);
}

void       vDestroyObject(vPObject object)
{
	vDBufferLock(_vcore.objects);

	vDBufferRemove(_vcore.objects, object);

	vDBufferUnlock(_vcore.objects);
}


/* ========== COMPONENT CREATION				==========	*/
vUI16 vCreateComponent(vPCHAR name, vUI64 staticSize, vUI64 objectSize,
	vPFCOMPONENTINITIALIZATIONSTATIC staticInitialization,
	vPFCOMPONENTINITIALIZATION initialization, vPFCOMPONENTDESTRUCTION destruction)
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
		vMemCopy(compD->componentName, name, strlen(name));
		compD->staticAttributeSize = staticSize;
		compD->objectAttributeSize = objectSize;
		compD->staticInitFunc = staticInitialization;
		compD->objectInitFunc = initialization;
		compD->objectDestroyFunc = destruction;
		
		/* allocate static block, do callback (if exists) */
		compD->staticAttribute = vAllocZeroed(compD->staticAttributeSize);
		if (compD->staticInitFunc)
			compD->staticInitFunc(compD, compD->staticAttribute);

		/* mark component descriptor as used */
		compD->inUse = TRUE;

		vCoreUnlock();

		/* RETURN HANDLE */
		return i;
	}

	/* fail if reached, components are all used up */
	vLogError(__func__, "Could not create more components. Max components have been created.");
	vDumpEntryBuffer();
	vCoreFatalError(__func__, "Could not create more components. Max components have been created.");
}

vUI16 vComponentGetHandleByName(vPCHAR name)
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

vBOOL vComponentGetNameByHandle(vUI16 handle, vPCHAR nameBuffer, vUI32 bufferLength)
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

vPTR  vComponentGetStaticPtr(vUI16 component)
{
	return _vcore.components[component].staticAttribute;
}

vPComponentDescriptor vComponentGetDescriptor(vUI16 component)
{
	return _vcore.components + component;
}


/* ========== OBJECT COMPONENT MANIPULATION		==========	*/
vBOOL vObjectAddComponent(vPObject object, vUI16 component)
{
	/* don't add if already existing */
	if (vObjectHasComponent(object, component)) return FALSE;

	vPComponentDescriptor desc = _vcore.components + component;

	EnterCriticalSection(&object->componentLock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if used, skip */
		if (comp->objectAttribute != NULL) continue;

		comp->componentDescriptorHandle = component;
		comp->staticAttribute = desc->staticAttribute;
		comp->objectAttribute = vAllocZeroed(desc->objectAttributeSize);

		/* call init callback if possible */
		if (desc->objectInitFunc)
			desc->objectInitFunc(object, comp);
		
		LeaveCriticalSection(&object->componentLock);
		return TRUE;
	}

	vLogWarningFormatted(__func__, "Cannot add any more components to object '%p'.",
		object);
	LeaveCriticalSection(&object->componentLock);
	return FALSE;
}

vBOOL vObjectRemoveComponent(vPObject object, vUI16 component)
{
	vPComponentDescriptor desc = _vcore.components + component;

	EnterCriticalSection(&object->componentLock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if used, skip */
		if (comp->objectAttribute != NULL) continue;

		/* if of not same type, skip */
		if (comp->componentDescriptorHandle != component) continue;

		/* call destruction callback if possible */
		if (desc->objectDestroyFunc)
			desc->objectDestroyFunc(object, comp);

		/* free object attribute memory */
		vFree(comp->objectAttribute);
		
		/* zero component memory */
		vZeroMemory(comp, sizeof(vComponent));

		LeaveCriticalSection(&object->componentLock);
		return TRUE;
	}

	vLogWarningFormatted(__func__, "Could not remove component '%d' from object '%p'.",
		component, object);
	LeaveCriticalSection(&object->componentLock);
	return FALSE;
}

vBOOL vObjectHasComponent(vPObject object, vUI16 component)
{
	EnterCriticalSection(&object->componentLock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if used, skip */
		if (comp->objectAttribute != NULL) continue;

		/* if not matching skip */
		if (comp->componentDescriptorHandle != component) continue;
		
		/* on match, return true */
		LeaveCriticalSection(&object->componentLock);
		return TRUE;
	}

	LeaveCriticalSection(&object->componentLock);
	return FALSE;
}

vPComponent vObjectGetComponent(vPObject object, vUI16 component)
{
	EnterCriticalSection(&object->componentLock);

	for (int i = 0; i < VOBJECT_MAX_COMPONENTS; i++)
	{
		vPComponent comp = object->components + i;

		/* if used, skip */
		if (comp->objectAttribute != NULL) continue;

		/* if unmatching, skip */
		if (comp->componentDescriptorHandle != component) continue;

		/* on match, return ptr */
		LeaveCriticalSection(&object->componentLock);
		return comp;
	}

	LeaveCriticalSection(&object->componentLock);
	return NULL;
}
