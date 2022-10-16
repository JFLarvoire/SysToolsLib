/*****************************************************************************\
*                                                                             *
*   Filename:	    HDiskW64.cpp					      *
*									      *
*   Description:    Windows 64-specific hard disk access routines	      *
*                                                                             *
*   Notes:	    This code implements the OS-independant hard disk I/O     *
*		    routines for the 64 bits version of Windows XP and later. *
*									      *
*		    OS-Independant routines are called HardDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    Physical disk accesses are done through file I/O to       *
*		    \\.\PhysicalDriveX device drivers.			      *
*									      *
*   History:								      *
*    2010/10/07 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#define HardDiskNTOpen HardDiskOpen
#define HardDiskNTClose HardDiskClose
#define HardDiskNTGetGeometry HardDiskGetGeometry
#define HardDiskNTRead HardDiskRead
#define HardDiskNTWrite HardDiskWrite

#include "HDiskNT.cpp"


