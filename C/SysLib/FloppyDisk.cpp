/*****************************************************************************\
*                                                                             *
*   Filename:	    HardDisk.cpp					      *
*									      *
*   Description:    OS-independant floppy disk access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    OS-Independant routines are called FloppyDiskXxxxx().     *
*		    Sectors are referenced by their DWORD LBA number.	      *
*									      *
*		    For MS-DOS, all access are done through the BIOS.	      *
*		    16-bits-BIOS-dependant routines are called BiosXxxxx().   *
*									      *
*   History:								      *
*    2017-07-17 JFL Created this file.					      *
*									      *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

/*****************************************************************************\
*                                                                             *
*                               MSDOS Routines                                *
*                                                                             *
\*****************************************************************************/

#if defined(_MSDOS)

#include "FDiskDos.cpp"

#endif // _MSDOS

/*****************************************************************************\
*                                                                             *
*                               WIN32 Routines                                *
*                                                                             *
\*****************************************************************************/

#if defined(_WIN32)

#include "FDiskW32.cpp"

#endif // _WIN32 or _WIN64

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/
