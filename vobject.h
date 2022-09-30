
/* ========== <vobject.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Object classification and component system				*/

#ifndef _VCORE_OBJECT_INCLUDE_
#define _VCORE_OBJECT_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
vPosition  vCreatePosition(float x, float y);
vTransform vCreateTransform(vPosition pos, float r, float s);
vPObject   vCreateObject(vTransform transform, vPObject parent);
void       vDestroyObject(vPObject object);


/* ========== COMPONENT CREATION				==========	*/
vUI16 vCreateComponent(vPCHAR name, vUI64 staticSize, vUI64 objectSize,
	vPFCOMPONENTINITIALIZATIONSTATIC staticInitialization,
	vPFCOMPONENTINITIALIZATION initialization, vPFCOMPONENTDESTRUCTION destruction);
vUI16 vComponentGetHandleByName(vPCHAR name);
vBOOL vComponentGetNameByHandle(vUI16 handle, vPCHAR nameBuffer, vUI32 bufferLength);
vPTR  vComponentGetStaticPtr(vUI16 component);
vPComponentDescriptor vComponentGetDescriptor(vUI16 component);


/* ========== OBJECT COMPONENT MANIPULATION		==========	*/
vBOOL vObjectAddComponent(vPObject object, vUI16 component);
vBOOL vObjectRemoveComponent(vPObject object, vUI16 component);
vBOOL vObjectHasComponent(vPObject object, vUI16 component);
vPComponent vObjectGetComponent(vPObject object, vUI16 component);


#endif
