
/* ========== <ventriesformatted.c>				==========	*/
/* Bailey Jia-Tao Brown							2022		*/


/* ========== INCLUDES							==========	*/
#include "ventriesformatted.h"
#include <stdio.h>


/* ========== FORMATTED EVENT LOGGING			==========	*/
VAPI void vLogEventFormatted(const char* funcName, const char* remarks, ...)
{
	va_list arguments;

	char strBuff[BUFF_MEDIUM];
	vZeroMemory(strBuff, sizeof(strBuff));

	va_start(arguments, remarks);
	vsprintf_s(strBuff, sizeof(strBuff), remarks, arguments);
	vLogEvent(funcName, strBuff);
	va_end(arguments);
}

VAPI void vLogWarningFormatted(const char* funcName, const char* remarks, ...)
{
	va_list arguments;

	char strBuff[BUFF_MEDIUM];
	vZeroMemory(strBuff, sizeof(strBuff));

	va_start(arguments, remarks);
	vsprintf_s(strBuff, sizeof(strBuff), remarks, arguments);
	vLogWarning(funcName, strBuff);
	va_end(arguments);
}

VAPI void vLogErrorFormatted(const char* funcName, const char* remarks, ...)
{
	va_list arguments;

	char strBuff[BUFF_MEDIUM];
	vZeroMemory(strBuff, sizeof(strBuff));

	va_start(arguments, remarks);
	vsprintf_s(strBuff, sizeof(strBuff), remarks, arguments);
	vLogError(funcName, strBuff);
	va_end(arguments);
}