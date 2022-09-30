
/* ========== <vdbuffers.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Object classification system								*/

#ifndef _VCORE_OBJECT_INCLUDE_
#define _VCORE_OBJECT_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
vPosition  vCreatePosition(float x, float y);
vTransform vCreateTransform(vPosition pos, float r, float s);
vPObject   vCreateObject(vTransform transform);
void       vDestroyObject(vPObject object);


/* ========== COMPONENT CREATION				==========	*/
vUI16 vCreateComponent(vPCHAR name, vUI64 staticSize, vUI64 objectSize,
	vPFDCOMPONENTINITIALIZATIONSTATIC staticInitialization,
	vPFDCOMPONENTINITIALIZATION initialization, vPFDCOMPONENTDESTRUCTION destruction);
vUI16 vComponentGetHandleByName(vPCHAR name);
void  vComponentGetNameByHandle(vUI16 handle, vPCHAR nameBuffer, vUI32 bufferLength);
vPTR  vComponentGetStaticPtr(vUI16 component);
vPComponentDescriptor vComponentGetDescriptor(vUI16 component);


/* ========== OBJECT COMPONENT MANIPULATION		==========	*/
vBOOL vObjectAddComponent(vPObject object, vUI16 component);
vBOOL vObjectRemoveComponent(vPObject object, vUI16 component);
vBOOL vObjectHasComponent(vPObject object, vUI16 component);

vBOOL vObjectLockComponent(vPObject object, vUI16 component);
vBOOL vObjectUnlockComponent(vPObject object, vUI16 component);

vPComponent vObjectGetComponent(vPObject object, vUI16 component);


#endif
