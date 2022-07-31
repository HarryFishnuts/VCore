
/* HEADER GUARD BEGIN */
#ifndef _VCORE_CORE_INCLUDE_
#define _VCORE_CORE_INCLUDE_ 

/* INCLUDES */
#include "internal.h";

/* CORE VERSION DEFINITIONS */
#define VCORE_VERSION_MAJOR 0
#define VCORE_VERSION_MINOR 1
#define VCORE_VERSION_PATCH 1

/* CORE VERSION STRUCTURE */
typedef struct vCOREVERSION
{
	vI32 major; /* incompatible API changes */
	vI32 minor; /* compatible API additions */
	vI32 patch; /* compatible bug fixes		*/
} vCOREVERSION, *vPCOREVERSION;

/* CORE STRUCTURE AND LAYOUT */
typedef struct vCORE
{
	/* large integer holding time in msc that core was init */
	ULONGLONG initializationTime;

	/* windows heap object */
	HANDLE heap;

	/* small memory buffer */
	vBYTE mBuffer[0x100];

	/* current state of the core							*/
	/* bit 0 -> has initialized?							*/
	LONG coreState;
} vCORE, *vPCORE;

/* CORE OBJECT */
vCORE _vCore;

/* Initializes _vCore object by first zeroing memory and then populating	*/
/* all of the members of the struct. Will terminate process if failes		*/
VAPI vBOOL vCoreInitialize(void);

/* Returns whether the _vCore object is initialized or not. Will return		*/
/* incorrect values if the coreState member is corrupted or tampered with	*/
VAPI vBOOL vCoreIsInitialized(void);

/* Checks if the first bit of the coreState member in the core object is	*/
/* set to 1. If not, will create an error and then terminate the process	*/
VAPI vVOID vCoreEnsureInitialized(void);

/* Creates and error message which specifies the attempted action which		 */
/* failed, any remarks from the dev, and whether to terminate the process	 */
/* once the messagebox is acknowleged. action and remarks params can be NULL */
VAPI vVOID vCoreCreateErrorMessage(const char* triedAction, const char* remarks,
	vBOOL terminateAfterMessageClose);

/* Takes a core version structure and populates it with relevant information */
VAPI vVOID vCoreGetVersion(vPCOREVERSION out);


/* HEADER GUARD END */
#endif