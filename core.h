
/* HEADER GUARD BEGIN */
#ifndef _VCORE_CORE_INCLUDE_
#define _VCORE_CORE_INCLUDE_ 

/* INCLUDES */
#include "internal.h";

/* CORE VERSION DEFINITIONS */
#define VCORE_VERSION_MAJOR 0
#define VCORE_VERSION_MINOR 1
#define VCORE_VERSION_PATCH 0

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
	vBYTE mBuffer[0x200];

	/* current state of the core							*/
	/* bit 0 -> has initialized?							*/
	LONG coreState;
} vCORE, *vPCORE;

/* CORE OBJECT */
vCORE _vCore;

/* INITIALIZATION FUNCTIONS */
VAPI vBOOL vCoreInitialize(void);
VAPI vBOOL vCoreIsInitialized(void);
VAPI vVOID vCoreEnsureInitialized(void);

/* ERROR MESSAGE FUNCTION */
VAPI vVOID vCoreCreateErrorMessage(const char* action, const char* remarks,
	vBOOL terminateAfterMessageClose);

/* CORE VERSION RELATED FUNCTIONS */
VAPI vVOID vCoreGetVersion(vPCOREVERSION out);


/* HEADER GUARD END */
#endif