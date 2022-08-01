
/* ========== <buffer.h>						==========	*/
/* Bailey Jia-Tao Brown			2022						*/
/* Lightweight threadsafe buffering system.					*/


#ifndef _VCORE_BUFFER_INCLUDE_
#define _VCORE_BUFFER_INCLUDE_ 

/* ========== INCLUDES							==========	*/
#include "core.h"


/* ========== INITIALIZATION FUNCTIONS			==========	*/

VAPI void _vBufferInit(void);
VAPI void _vBufferTerminate(void);


/* ========== BUFFER CREATION AND DESTRUCTION	==========	*/

/* creates a buffer behavior for buffer objects to adhere	*/
/* to. buffer behaviors cannot be destroyed.				*/
VAPI vHNDL vCreateBufferBehavior(const char* name, SIZE_T elementSize,
	vI32 elementCount, vBOOL threadSafe, vBOOL lockPerElement,
	vBOOL zeroElements, vPFBUFFINITIALIZER elementInitCallback,
	vPFBUFFDESTRUCTOR  elementDestroyCallback);

/* creates a buffer object to holds things within.			 */
VAPI vHNDL vCreateBuffer(const char* name, vHNDL behavior);

/* destroys a buffer object									 */
VAPI vBOOL vDestroyBuffer(vHNDL bufferHndl);


/* ========== BUFFER ELEMENT OPERATIONS			==========	*/

/* finds an empty spot in the buffer and returns the		*/
/* pointer to that element for the user to store or modify  */
/* NOT TO BE USED WHEN MULTITHREADING!!						*/
VAPI vPTR vBufferAdd(vHNDL buffer);

/* finds an empty spot in the buffer and then performs the	*/
/* operation specified (if any) to the element.				*/
VAPI void vBufferAddSafe(vHNDL buffer, vPFBUFFOPERATION operation);

/* returns a pointer to an element in the buffer specified  */
/* by an index. NOT TO BE USED WHEN MULTITHREADING!!		*/
VAPI vPTR vBufferGet(vHNDL buffer, vI32 index);

/* finds the element in the buffer specified by an index    */
/* performs the function passed on it						*/
VAPI void vBufferOperate(vHNDL buffer, vI32 index, vPFBUFFOPERATION operation);

/* finds the element within the buffer by index and removes */
/* it, applying the destructor function if any.				*/
VAPI void vBufferRemoveIndex(vHNDL buffer, vI32 index);

/* loops through every element and performs the specified	*/
/* operation on it. buffer may be synced depending on bhv	*/
VAPI void vBufferIterate(vHNDL buffer, vPFBUFFITERATOR operation);


#endif
