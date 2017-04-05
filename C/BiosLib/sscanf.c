/*****************************************************************************\
*									      *
*   File name:	    sscanf.c						      *
*									      *
*   Description:    Subset of the standard C library sscanf() routine.	      *
*									      *
*   Notes:	    Standard C library routine. 			      *
*									      *
*   History:								      *
*    1998/05/25 JFL Created this file					      *
*									      *
*      (c) Copyright 1998-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    sscanf						      |
|									      |
|   Purpose:	    Redefinition of a Standard C Library routine	      |
|									      |
|   Parameters:     char *pszString	String to scan			      |
|		    char *pszFormat	Format of the above string	      |
|		     ...		List of pointers to scanned data      |
|									      |
|   Return:	    The number of fields assigned			      |
|									      |
|   History:								      |
|    1998/05/25 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl sscanf(const char *pszString, const char *pszFormat, ...)
    {
    return vscanf(pszString, &pszFormat);  // Common entry point to scanf & sscanf
    }

int _cdecl vscanf(const char *pszString, const char **ppVarList)
    {
    const char *pszFormat;  // Pointer to the format string
    int *pparms;	    // Pointer to the optional arguments following it
    char c;
    int iWidth;
    int nFields = 0;
    int iLong;
    int iUsed;

    pszFormat = *ppVarList;	     // format = First in the variable list
    pparms = (int *)(ppVarList + 1); // First optional parameter following it

    while (c = *(pszFormat++))
	{
	if (c == '%')	  // If c is the format escape character.
	    {
	    pszFormat += stcd_i(pszFormat, &iWidth);
	    iLong = FALSE;
test_format:
	    switch (*(pszFormat++))    // The next char is the format ID.
		{
		case 'd':
		    if (!iLong)
			{
			iUsed = stcd_i(pszString, (int *)*(pparms++)); // Convert string to int
			pszString += iUsed;
			if (iUsed) nFields += 1;
			}
		    // else
		    //	   pszString += stcld_i(pszString, *(((long *)pparms)++)); // Convert string to long int
		    break;
		case 'x':
		case 'X':
		    if (!iLong)
			{
			iUsed = stch_i(pszString, (int *)*(pparms++)); // Convert hex. string to int
			pszString += iUsed;
			if (iUsed) nFields += 1;
			}
		    // else
		    //	  pszString += stclh_i(pszString, *(((long *)pparms)++)); // Convert uint to hex. string
		    break;
		case 's':
		    // Incompatible! To be improved.
		    if (!iWidth)
			{
			strcpy((char *)*(pparms++), pszString);
			pszString += strlen(pszString);
			}
		    else
			{
			strncpy((char *)*(pparms++), pszString, iWidth);
			pszString += iWidth;
			}
		    nFields += 1;
		    break;
		case 'c':
		    c = *(pszString++);
		    if (c)
			{
			*(char *)*(pparms++) = c;
			nFields += 1;
			}
		    break;
		case 'l':
		    iLong = TRUE;
		    goto test_format;	 // Ignore long integer specifications
		default:    // Unsupported format. Just output question marks
		    break;
		}
	    }
	else		  // Else c is a normal character
	    {
	    pszString += 1; // Skip it in the input string
	    }
	}

    return nFields;
    }
