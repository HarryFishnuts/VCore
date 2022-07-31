
/* HEADER GUARD BEGIN */
#ifndef _VCORE_INTERNAL_INCLUDE_
#define _VCORE_INTERNAL_INCLUDE_

/* PREPROCESSOR DEFS */
#define _WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

/* INCLUDES */
#include <stdint.h>
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UNSIGNED TYPES */
typedef void		vVOID;
typedef uint8_t		vBYTE;
typedef uint8_t		vBOOL;
typedef uint8_t		vUI8;
typedef uint16_t	vUI16;
typedef uint32_t	vUI32;
typedef uint32_t	vHNDL;
typedef HANDLE		vWHNDL;
typedef uint64_t	vUI64;
typedef uint64_t	vTIME;

/* SIGNED TYPES */
typedef int8_t		vI8;
typedef int16_t		vI16;
typedef int			vINT;
typedef int32_t		vI32;
typedef int64_t		vI64;

/* FLOAT TYPES */
typedef float		vF32;
typedef double		vF64;

/* POINTER TYPES */
typedef void*		vPTR;
typedef vBYTE*		vPBYTE;
typedef vBOOL*		vPBOOL;
typedef vUI8*		vPUI8;
typedef vUI16*		vPUI16;
typedef vUI32*		vPUI32;
typedef vHNDL*		vPHNDL;
typedef vWHNDL*		vPWHNDL;
typedef vUI64*		vPUI64;
typedef vTIME*		vPTIME;
typedef vI8*		vPI8;
typedef vI16*		vPI16;
typedef vINT*		vPINT;
typedef vI32*		vPI32;
typedef vI64*		vPI64;

/* VECTOR TYPES */
typedef struct vIV16
{
	vI16 x;
	vI16 y;
} vIV16, *vPIV16;
typedef struct vIV32
{
	vI32 x;
	vI32 y;
} vIV32, *vPIV32;
typedef struct vIV64
{
	vI64 x;
	vI64 y;
} vIV64, *vPIV64;
typedef struct vFV32
{
	vF32 x;
	vF32 y;
} vFV32, *vPFV32;
typedef struct vFV64
{
	vF64 x;
	vF64 y;
} vFV64, *vPFV64;

/* API DEFINITION */
#ifdef VCORE_EXPORTS
#define VAPI __declspec(dllexport)
#else
#define VAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
}
#endif

/* HEADER GUARD END */
#endif