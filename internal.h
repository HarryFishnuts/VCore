
/* ========== <internal.h>						========== */
/* Bailey Jia-Tao Brown			2022					   */
/* Holds all types, structs and definitions used in VCore  */


#ifndef _VCORE_INTERNAL_INCLUDE_
#define _VCORE_INTERNAL_INCLUDE_

/* ========== PREPROCESSOR DEFINITIONS			========== */
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN


/* ========== INCLUDES							========== */
#include <stdint.h>
#include <Windows.h>


/* ========== DEFINITIONS						========== */
#define ZERO		0
#define NO_FLAGS	(DWORD)0
#define NO_HANDLE	(HANDLE)0
#define NO_WINDOW	(HWND)0

#define BUFF_TINY	0x40
#define BUFF_SMALL	0x80
#define BUFF_MEDIUM 0x100
#define BUFF_LARGE	0x200

#define MAX_MEMORY_ALLOCATEABLE			0x100000L
#define MAX_ACTIONS_SAVED_IN_MEMORY		0x80
#define ACTION_LOG_DUMP_INTERVAL_SEC	0x20

#define MAX_BUFFER_BEHAVIORS	0x40
#define MAX_BUFFER_OBJECTS		0x200

#ifdef VCORE_EXPORTS
#define VAPI __declspec(dllexport)
#else
#define VAPI __declspec(dllimport)
#endif


/* ========== TYPE DEFINITIONS					========== */

/* unsigned integers									   */
typedef uint8_t		vUI8;
typedef uint16_t	vUI16;
typedef uint32_t	vUI32;
typedef uint64_t	vUI64;

/* signed integers										   */
typedef int8_t		vI8;
typedef int16_t		vI16;
typedef int32_t		vI32;
typedef int64_t		vI64;

/* unsigned integer pointers							   */
typedef vUI8*		vPUI8;
typedef vUI16*		vPUI16;
typedef vUI32*		vPUI32;
typedef vUI64*		vPUI64;

/* signed integer pointers								   */
typedef vI8*		vPI8;
typedef vI16*		vPI16;
typedef vI32*		vPI32;
typedef vI64*		vPI64;

/* misc types											   */
typedef void*		vPTR;
typedef uint8_t		vBYTE;
typedef uint8_t		vBOOL;
typedef vUI32		vHNDL;
typedef vUI64		vTIME;

/* buffer callbacks										   */
typedef void (*vPFBUFFITERATOR)   (vI32 index, vPTR element);
typedef void (*vPFBUFFINITIALIZER)(vI32 index, vPTR element);
typedef void (*vPFBUFFDESTRUCTOR) (vI32 index, vPTR element);
typedef void (*vPFBUFFOPERATION)  (vI32 index, vPTR element);


/* ========== ENUMERATIONS						========== */

/* action type enumeration								   */
typedef enum vEnumActionType
{
	vActionType_UNUSED		= 0,
	vActionType_ACTION		= 1,
	vActionType_WARNING		= 2,
	vActionType_ERROR		= 3
} vEnumActionType;

/* buffering status enumeration							   */
typedef enum vEnumBufferStatus
{
	vBufferStatus_SUCESS		= 0,
	vBufferStatus_OUTOFSPACE	= 1,
} vEnumBufferStatus;



/* ========== ACTION AND LOGGING STRUCTURES		========== */

typedef struct vActionLog
{
	vEnumActionType type;
	char	action[BUFF_TINY];
	char	remark[BUFF_MEDIUM];
	vTIME	timeCreated;
} vActionLog, *vPActionLog;

typedef struct vActionLogBuffer
{
	CRITICAL_SECTION rwPermission;

	vTIME		lastDump;
	char		actionWriteFileName[BUFF_TINY];
	vI16		actionIndex;
	vActionLog	actionLog[MAX_ACTIONS_SAVED_IN_MEMORY];
} vActionLogBuffer, *vPActionLogBuffer;


/* ========== BUFFERING SYSTEM STRUCTURES		========== */

typedef struct vBufferBehavior
{
	char name[BUFF_TINY];

	vPFBUFFINITIALIZER	initializer;
	vPFBUFFDESTRUCTOR   destructor;
	SIZE_T				elementSizeBytes;
	vI32				elementCount;
	vI32				fieldChunkCount;

	/* each bit corresponds to a behavior		*/
	/* bit 0 -> is buffer threadsafe?			*/
	/* bit 1 ->	0 =	lock per whole iteration	*/
	/*			1 = lock per element			*/
	/* bit 2 -> zero on allocate element?		*/
	vBYTE				flags;

} vBufferBehavior, *vPBufferBehavior;

typedef struct vBufferObject
{
	vPBufferBehavior behavior;
	CRITICAL_SECTION rwPermission;

	char name[BUFF_TINY];
	vI32 usedElementCount;

	vPUI64 field;
	vPUI8  data;
} vBufferObject, *vPBufferObject;

typedef struct vBufferHandler
{
	CRITICAL_SECTION rwPermission;

	vI32 behaviorCount;
	vI32 bufferCount;

	vBufferBehavior behaviors[MAX_BUFFER_BEHAVIORS];
	vPBufferObject	buffers[MAX_BUFFER_OBJECTS];
} vBufferHandler, *vPBufferHandler;

/* ========== CORE LIBRARY STRUCTURE			========== */

typedef struct vCoreLibrary
{
	vTIME initializeTime;
	vActionLogBuffer actionLog;
	vBufferHandler	 bufferHandler;
} vCoreLibrary, *vPCoreLibrary;

#endif