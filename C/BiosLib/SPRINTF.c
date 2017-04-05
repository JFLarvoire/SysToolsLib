/*****************************************************************************\
*                                                                             *
*   File name:      sprintf.c                                                 *
*                                                                             *
*   Description:    Subset of the standard C library routine.                 *
*                                                                             *
*   Notes:          For use where the standard C library can't be used.       *
*		    Reentrant.                                                *
*                   Supports only simple integer types.                       *
*                   The goal is to use as little ROM space as possible.       *
*                   Should you want to improve it, remember to keep it small! *
*                                                                             *
*   History:                                                                  *
*    1993/10/06 JFL Separated this file from CLIBC.C.                         *
*    1997/08/25 JFL Added routines sctli_d() and stcli_h().                   *
*                   Support formats %ld and %lx.                              *
*    1997/10/06 JFL Moved stcx_x routines into new stc.c.		      *
*    1999/09/06 JFL Added standard C library's vsprintf().                    *
*    2002/07/15 JFL Added support for %Fs format (far strings).               *
*                                                                             *
*      (c) Copyright 1987-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"            /* Make sure our implementation matches the
                                     definition there */
/* Forward references */

static int strcpyform(char *, char far *, int, int, char);

//+--------------------------------------------------------------------------
//+ Function   : sprintf
//+
//+ Purpose    : Redefinition of a C library routine
//+
//+ Parameters : See a C library reference for the list of arguments
//+
//+ Return     : See a C library reference for the return value.
//+
//+ Notes:     : The list of supported formats is rather limited.
//+
//+              The goal is to have compact code, even if at the cost of
//+               being slightly incompatible.
//+
//+              This routine is usable even in interrupt routines.
//+
//+ Creation   : 1987 by Jean-François LARVOIRE, in CLIBC.C.
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+ 04-Feb-1988  JFL     Ported from Lattice C to Microsoft C 5.0
//+ 24-Mar-1988  JFL     Optimized, added a local version of sprintf.
//+ 18-Apr-1990  JFL     Adapted to ROM setup demo.
//+ 03-May-1990  JFL     Added %u format and stcu_d support routine.
//+ 02-Nov-1990  JFL     Changed Tabs to 8 for the Microsoft Editor.
//+ 15-Nov-1990  JFL     Added _cdecl keywords to printf & sprintf.
//+ 07-Dec-1990  JFL     Moved prototypes of public routines to clibdef.h.
//+                      Made all other routines static.
//+ 03-Sep-1993  JFL     Allow 0 left-padding with the %s format.
//+ 06-Sep-1993  JFL     Support %% format to output a '%'.
//+                      Support variable length formats %*.
//+ 09-Mar-1994  JFL     Split routine sprintf1 to removed the limit on the
//+                      number of arguments to printf.
//+ 31-Aug-1995  JFL     Added a test to ignore the 'l' format specification.
//+
//+--------------------------------------------------------------------------

int _cdecl sprintf(char *pszOutput, const char *pszFormat, ...)
    {
    return sprintf1(pszOutput, &pszFormat);  // Common entry point to printf & sprintf
    }

// sprintf1() was defined for Nodoslib before I learned about Standard C library's vsprintf().
// I kept it as sprintf1() is more simple, and was used in several programs.
int sprintf1(char *pszOutput, const char **ppVarList)
    {
    //              Output     Format        Address of next argument
    return vsprintf(pszOutput, ppVarList[0], (va_list)(ppVarList+1));
    }

extern int vsprintf(char *pszOutput, const char *pszFormat, va_list pArgs)
    {
    return _vsnprintf(pszOutput, 65535, pszFormat, pArgs);
    }

extern int _cdecl _snprintf(char *pszOutput, size_t uSize, const char *pszFormat, ...)
    {
    return _vsnprintf(pszOutput, uSize, pszFormat, (va_list)((&pszFormat)+1));
    }

extern int _vsnprintf(char *pszOutput, size_t uSize, const char *pszFormat, va_list pArgs)
    {
    int *pparms;	    // Pointer to the optional arguments following the format
    char c;
    char cFill;
    char szTempBuf[80];
    char far *lpszTemp;
    char *pszOutput0 = pszOutput;
    int iRequested;	    // Requested width for an output field
    int iAvailable;	    // Number of characters available for output
    int iLong;		    // TRUE if the %l modifier has been provided

    pparms = (int *)pArgs;

    while (c = *(pszFormat++))
	{
	if (c == '%')	  // If c is the format escape character.
	    {
	    cFill = ' ';   // Default filler for the left side of output fields
	    if (*pszFormat == '0')
		{
		cFill = '0';		// Optional filler
		pszFormat += 1; 	// Skip the 0
		}

	    if (*pszFormat == '*')	// If variable length format
		{
		iRequested = *(pparms++); // Get the size from the argument list
		pszFormat += 1; 	  // Skip the *
		}
	    else			// Else fixed length format
		{
		// Convert the field size to an int, and advance past the size in format.
		pszFormat += stcd_i(pszFormat, &iRequested);
		}

	    iLong = FALSE;
test_format:
	    switch (*(pszFormat++))    // The next char is the format ID.
		{
		case 'd':
		    if (!iLong)
			iAvailable = stci_d(szTempBuf, *(pparms++)); // Convert int to string
		    else
			iAvailable = stcli_d(szTempBuf, *(((long *)pparms)++)); // Convert int to string
		    pszOutput += strcpyform(pszOutput, szTempBuf, iRequested, iAvailable, cFill);
		    break;
		case 'u':
		    iAvailable = stcu_d(szTempBuf, *(pparms++)); // Convert uint to string
		    pszOutput += strcpyform(pszOutput, szTempBuf, iRequested, iAvailable, cFill);
		    break;
		case 'p':
		    if (iLong)
			pszOutput += sprintf(pszOutput, "%04X:", pparms[1]);
		    pszOutput += sprintf(pszOutput, "%04X", pparms[0]);
		    pparms += iLong+1;
		    break;
		case 'x':   // Incompatible! Processed as %X to save space.
		case 'X':
		    if (!iLong)
			iAvailable = stci_h(szTempBuf, *(pparms++)); // Convert uint to hex. string
		    else
			iAvailable = stcli_h(szTempBuf, *(((long *)pparms)++)); // Convert uint to hex. string
		    pszOutput += strcpyform(pszOutput, szTempBuf, iRequested, iAvailable, cFill);
		    break;
		case 's':
		    if (!iLong)
			lpszTemp = *(((char **)pparms)++);		 // Pointer on the given string
		    else
			lpszTemp = *(((char far **)pparms)++);		 // Pointer on the given string
		    pszOutput += strcpyform(pszOutput, lpszTemp, iRequested, fstrlen(lpszTemp), cFill);
		    break;
		case 'c':
		    *pparms &= 0x00FF;	 // Make sure the char is followed by a NUL.
		    pszOutput += strcpyform(pszOutput, (char *)(pparms++), iRequested, 1, ' ');
		    break;
		case '%':
		    *(pszOutput++) = '%';
		    break;
		case 'l':
		case 'F':
		    iLong = TRUE;
		    goto test_format;	 // Ignore long integer specifications
		default:    // Unsupported format. Just output question marks
		    pszOutput += strcpyform(pszOutput, "", iRequested, 0, '?');
		    break;
		}
	    }
	else		  // Else c is a normal character
	    {
	    *(pszOutput++) = c;  // Just copy it to the output
	    }
	}
    *pszOutput = '\0';
    return pszOutput - pszOutput0;   // Number of characters written
    }

//+--------------------------------------------------------------------------
//+ Function   : strcpyform
//+
//+ Purpose    : Formatted copy of a string, used internally by sprintf.
//+
//+ Parameters : See comments in function definition below
//+
//+ Return     : Number of characters output
//+
//+ Notes:     : This code is tried and tested. It is not expected to change.
//+              This is why the variable names have not been updated to Pike
//+               coding standards.
//+
//+ Creation   : 1987 by Jean-François LARVOIRE
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+ 04-Feb-1988  JFL     Ported from Lattice C to Microsoft C 5.0
//+ 07-Dev-1990  JFL     Made routines static.
//+ 09-Mar-1993  JFL     Reformatted according to Pike coding standards
//+
//+--------------------------------------------------------------------------

static int strcpyform(
char *to,       /* Where to copy the from string */
char far *from, /* String to copy */
int size,       /* Number of characters to copy or 0 = copy actual size */
int actual,     /* Actual size of the string to copy */
char cFill)     /* Character to use to fill the left of the field */
    {
    int right = TRUE;		  /* TRUE if right justified */
    int fill = 0;

    if (actual < 0) return 0;	  /* Error */
    if (size == 0) size = actual; /* Free form copy */
    if (size < 0)		  /* Left justify */
	{
	right = FALSE;
	size = -size;
	}
    if (size < actual)		  /* Copy only part of the string */
	{
	if (right) from += actual - size;
	actual = size;
	}
    fill = size - actual;	  /* Number of spaces to add */

    if (right)			  /* If right justified ... */
	{
	while (fill--) *to++ = cFill; /* ... fill left with fill character */
	}
    while (actual--) *to++ = *from++; /* Actual copy */
    if (!right) 		  /* If left justified ... */
	{
	while (fill--) *to++ = ' ';   /* ... fill right with spaces */
	}
    *to = '\0';

    return size;
    }
