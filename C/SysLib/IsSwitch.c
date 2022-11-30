/*****************************************************************************\
*									      *
*   File name	    IsSwitch.c						      *
*									      *
*   Description     Test if a command line argument is a switch.	      *
*									      *
*   Notes								      *
*									      *
*   History								      *
*    1998-05-25 JFL Created this file in BiosLib.			      *
*    2022-10-19 JFL Moved to SysLib, with support for non-Microsoft OSs.      *
*    2022-11-29 JFL Use the CDECL calling convention, to get the same API     *
*		    as the old version in BiosLib.			      *
*		    Note that using _fastcall in SysLib causes a warning      *
*		    C4124: __fastcall with stack checking is inefficient      *
*		    							      *
*      (c) Copyright 1997-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "mainutil.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    IsSwitch						      |
|									      |
|   Description     Test if a command line argument is a switch.	      |
|									      |
|   Parameters      char *pszArg					      |
|									      |
|   Returns	    TRUE or FALSE					      |
|									      |
|   Notes								      |
|									      |
|   History								      |
|    1997-03-04 JFL Created this routine				      |
|    2016-08-25 JFL "-" alone is NOT a switch.				      |
*									      *
\*---------------------------------------------------------------------------*/

int CDECL IsSwitch(char *pszArg) {
  switch (*pszArg) {
    case '-': /* All operating systems, including Unix, MacOS, FreeBSD, ... */
#if defined(_WIN32) || defined(_MSDOS)
    case '/': /* Microsoft operating systems only */
#endif
      return (*(short*)pszArg != (short)'-'); /* "-" is NOT a switch */
    default:
      return FALSE;
  }
}

