/*****************************************************************************\
*                                                                             *
*   Filename:	    LogDisk.cpp						      *
*									      *
*   Description:    OS-independant logical disk access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    OS-Independant routines are called LogDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    For MS-DOS, all access are done through DOS int 21H.      *
*									      *
*   History:								      *
*    2002/02/07 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

/*****************************************************************************\
*                                                                             *
*                               MSDOS Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _MSDOS

#include "LDiskDos.cpp"

#endif // _MSDOS

/*****************************************************************************\
*                                                                             *
*                               WIN32 Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _WIN32

#include "LDiskW32.cpp"

#endif // _WIN32

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/
