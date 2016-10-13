/*****************************************************************************\
*                                                                             *
*   Filename:	    HardDisk.cpp					      *
*									      *
*   Description:    OS-independant hard disk access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    OS-Independant routines are called HardDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    For MS-DOS, all access are done through the BIOS.	      *
*		    16-bits-BIOS-dependant routines are called BiosXxxxx().   *
*									      *
*   History:								      *
*    2000/09/21 JFL Created this file.					      *
*    2001/01/08 JFL Added support for read-only mode.			      *
*		    Changed HardDiskXxxx buffer pointers to far pointers.     *
*    2001/01/10 JFL Changed GetBiosDiskXxxx buffer to a far pointer.   	      *
*    2001/01/22 JFL Added functions BiosDiskReadLba() and BiosDiskWriteLba(). *
*		    Use them first in HardDiskRead() and HardDiskWrite().     *
*    2001/02/20 JFL Added function HardDiskOpen().			      *
*    2001/02/26 JFL Removed function HardDiskGetNext().			      *
*		    Added WIN32 (NT only) version of the functions.	      *
*		    Fixed bug: Process flag iReadOnly in BiosDiskWriteLba too.*
*    2001/06/18 JFL Moved BIOS definitions to int13.h.			      *
*    2001/09/05 JFL Moved actual code to hDiskDos.cpp & HDiskW32.cpp.	      *
*    2010/10/07 JFL Added inclusion of a WIN64 version.                       *
*									      *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

/*****************************************************************************\
*                                                                             *
*                               MSDOS Routines                                *
*                                                                             *
\*****************************************************************************/

#if defined(_MSDOS)

#include "HDiskDos.cpp"

#endif // _MSDOS

/*****************************************************************************\
*                                                                             *
*                               WIN32 Routines                                *
*                                                                             *
\*****************************************************************************/

#if defined(_WIN64)

#include "HDiskW64.cpp"

#elif defined(_WIN32)

#include "HDiskW32.cpp"

#endif // _WIN32 or _WIN64

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/
