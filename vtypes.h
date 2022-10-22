
/* ========== <vtypes.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* All type definitions for VCore							*/

#ifndef _VCORE_TYPES_INCLUDE_
#define _VCORE_TYPES_INCLUDE_

/* ========== INCLUDES							==========	*/
#include <stdint.h>


/* ========== SIGNED INTEGER TYPES				==========	*/
typedef int8_t	vI8;
typedef int16_t vI16;
typedef int32_t vI32;
typedef int64_t vI64;


/* ========== UNSIGNED INTEGER TYPES			==========	*/
typedef uint8_t	 vUI8;
typedef uint16_t vUI16;
typedef uint32_t vUI32;
typedef uint64_t vUI64;


/* ========== SIGNED INTEGER POINTER TYPES		==========	*/
typedef vI8*  vPI8;
typedef vI16* vPI16;
typedef vI32* vPI32;
typedef vI64* vPI64;


/* ========== UNISGNED INTEGER POINTER TYPES	==========	*/
typedef vUI8*  vPUI8;
typedef vUI16* vPUI16;
typedef vUI32* vPUI32;
typedef vUI64* vPUI64;


/* ========== MISCELLANEOUS TYPES				==========	*/
typedef vUI8   vBOOL;
typedef char   vCHAR;
typedef vCHAR* vPCHAR;
typedef vUI8   vBYTE;
typedef vBYTE* vPBYTE;
typedef void*  vPTR;
typedef vUI32  vHNDL;
typedef vUI64  vTIME;
typedef vTIME* vPTIME;


/* ========== CALLBACK TYPES					==========	*/
typedef void (*vPFBUFFERITERATEFUNC )(vHNDL buffer, vUI16 index, vPTR element, vPTR input);
typedef void (*vPFDBUFFERITERATEFUNC)(vHNDL dbuffer, vPTR element, vPTR input);

typedef void (*vPFBUFFERINITIALIZEELEMENT )(vHNDL buffer, vUI16 index, vPTR element
	, vPTR input);
typedef void (*vPFDBUFFERINITIALIZEELEMENT)(vHNDL dbuffer, vPTR element, vPTR input);

typedef void (*vPFBUFFERDESTROYELEMENT )(vHNDL buffer, vUI16 index, vPTR element);
typedef void (*vPFDBUFFERDESTROYELEMENT)(vHNDL dbuffer, vPTR element);

typedef void (*vPFCOMPONENTINITIALIZATIONSTATIC)(struct vComponentDescriptor* descriptor,
	vPTR staticData);
typedef void (*vPFCOMPONENTINITIALIZATION)(struct vObject* object, 
	struct vComponent* component, vPTR input);
typedef void (*vPFCOMPONENTCYCLE)(struct vWorker* worker, vPTR persistentData,
	struct vComponent* component);
typedef void (*vPFCOMPONENTDESTRUCTION)(struct vObject* object, 
	struct vComponent* component);

typedef void (*vPFWORKERINIT )(struct vWorker* worker, vPTR persistentData, 
	vPTR input);
typedef void (*vPFWORKEREXIT )(struct vWorker* worker, vPTR persistentData);
typedef void (*vPFWORKERCYCLE)(struct vWorker* worker, vPTR persistentData);
typedef void (*vPFWORKERTASK) (struct vWorker* worker, vPTR persistentData, 
	vPTR input);


#endif
