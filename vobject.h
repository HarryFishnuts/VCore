
/* ========== <vobject.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Object classification and component system				*/

#ifndef _VCORE_OBJECT_INCLUDE_
#define _VCORE_OBJECT_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI vPosition  vCreatePosition(float x, float y);
VAPI vTransform vCreateTransform(vPosition pos, float r, float s);
VAPI vPObject   vCreateObject(vTransform transform, vPObject parent);
VAPI void       vDestroyObject(vPObject object);


/* ========== COMPONENT CREATION				==========	*/
VAPI vUI16 vCreateComponent(vPCHAR name, vUI64 staticSize, vUI64 objectSize,
	vPFCOMPONENTINITIALIZATIONSTATIC staticInitialization,
	vPFCOMPONENTINITIALIZATION initialization, vPFCOMPONENTDESTRUCTION destruction);
VAPI vUI16 vComponentGetHandleByName(vPCHAR name);
VAPI vBOOL vComponentGetNameByHandle(vUI16 handle, vPCHAR nameBuffer, vUI32 bufferLength);
VAPI vPTR  vComponentGetStaticPtr(vUI16 component);
VAPI vPComponentDescriptor vComponentGetDescriptor(vUI16 component);


/* ========== OBJECT SYNCHRONIZATION			==========	*/
VAPI void vObjectLockAll(void);
VAPI void vObjectUnlockAll(void);
VAPI void vObjectLock(vPObject object);
VAPI void vObjectUnlock(vPObject object);


/* ========== OBJECT COMPONENT MANIPULATION		==========	*/
VAPI vBOOL vObjectAddComponent(vPObject object, vUI16 component);
VAPI vBOOL vObjectRemoveComponent(vPObject object, vUI16 component);
VAPI vBOOL vObjectHasComponent(vPObject object, vUI16 component);
VAPI vPComponent vObjectGetComponent(vPObject object, vUI16 component);


#endif
