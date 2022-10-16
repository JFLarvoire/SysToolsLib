/*****************************************************************************\
*                                                                             *
*   Filename:	    File.cpp						      *
*									      *
*   Description:    OS-independant file access routines			      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application, and  *
*		    yet give access to 64-bits files under OS capable of it.  *
*									      *
*		    OS-Independant routines are called FileXxxxx().	      *
*		    Offsets are referenced by their QWORD byte number.	      *
*									      *
*   History:								      *
*    2008/04/21 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#define _CRT_SECURE_NO_WARNINGS /* Prevent warnings about using fopen, etc */

#include "File.h"

#include "FileLibc.cpp"

#ifdef _WIN32
#include "FileW32.cpp"
#endif // _WIN32

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/
