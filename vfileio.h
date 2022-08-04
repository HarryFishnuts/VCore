
/* ========== <vfileio.h>						==========	*/
/* Bailey Jia-Tao Brown							2022		*/
/* Fast win32 file manipulation								*/

#ifndef _VCORE_FILEIO_INCLUDE_
#define _VCORE_FILEIO_INCLUDE_

/* ========== INCLUDES							==========	*/
#include "vcore.h"


/* ========== CREATION AND DESTRUCTION			==========	*/
VAPI HANDLE vFileCreate(const char* fileName);
VAPI HANDLE vFileOpen(const char* fileName);
VAPI void   vFileDelete(const char* fileName);
VAPI void   vFileClose(HANDLE fHndl);


/* ========== READ AND WRITE					==========	*/
VAPI vBOOL vFileRead(HANDLE fHndl, vUI32 readOffset, vUI32 readAmount,
	vPTR outBuffer);
VAPI vBOOL vFileWrite(HANDLE fHndl, vUI32 writeOffset, vUI32 writeAmount,
	vPTR inBuffer);
VAPI vBOOL vFileReadLocked(HANDLE fHndl, vUI32 readOffset, vUI32 readAmount,
	vPTR outBuffer);
VAPI vBOOL vFileWriteLocked(HANDLE fHndl, vUI32 writeOffset, vUI32 writeAmount,
	vPTR inBuffer);


/* ========== FILE INFORMATION					==========	*/
VAPI vBOOL vFileExists(const char* fileName);
VAPI vUI64 vFileSize(HANDLE fHndl);

#endif 
