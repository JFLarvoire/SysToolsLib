/*****************************************************************************\
*									      *
*   File name:	    ioctldr.c						      *
*									      *
*   Description:    IOCtl disk read routine				      *
*									      *
*   Notes:	    Important: Accessing tracks to disks with more than 36    *
*		    sectors per track using IOCtl calls fails, unless the     *
*		    device parameters for the drives are updated with an      *
*		    extended device parameters structure (DEVICEPARAMSEX).    *
*									      *
*		    This structure must be filled with a trivial track layout,*
*		    that is consecutive entries with numbers from 1 to max,   *
*		    and size always 512.				      *
*									      *
*   History:								      *
*    1995/08/25 JFL Created this file					      *
*									      *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "msdos.h"
#include "lodos.h"

#pragma warning(disable:4704)   // Ignore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IoctlDiskRead					      |
|									      |
|   Description:    Read physical disk sectors, using a DOS IOCtl call.       |
|									      |
|   Parameters:     See below						      |
|									      |
|   Returns:	    The DOS error code. 0 = success.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1994/07/28 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl IoctlDiskRead(
WORD wDrive,		/* 1=A, 2=B, 3=C, etc... */
WORD wCyl, 					/* Cylinder */
WORD wHead, 				/* Head */
WORD wSect, 				/* Sector */
WORD n, 						/* Number of sectors to read */
char far *lpcBuffer)		/* Buffer for the data read */
    {
    int iErrCode;
    RWBLOCK rw;

    rw.rwSpecFunc = 0;
    rw.rwHead = wHead;
    rw.rwCylinder = wCyl;
    rw.rwFirstSector = wSect;
    rw.rwSectors = n;
    rw.rwBuffer = (DWORD)lpcBuffer;

    _asm
	{
	push ds
	mov  bx, wDrive
	lea  dx, word ptr rw
	push ss
	pop  ds 		; rw is on stack, so copy ss -> ds
	mov  ax, 440DH		; Function IOCTL for block devices
	mov  cx, 0861H		; Category 08, subfunction 61: Read track
	int  21H
	pop  ds
	jc   error_abs_dr	; Jump if error
	xor  ax, ax
    error_abs_dr:
	mov  iErrCode, ax	; Save the error code
	}

    return iErrCode;
    }
