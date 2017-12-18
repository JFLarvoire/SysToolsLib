/*****************************************************************************\
*									      *
*   File name:	    diskread.c						      *
*									      *
*   Description:    BIOS disk read routine				      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1995/08/25 JFL Created this file					      *
*    2000/07/25 JFL Use constant ISECT0 as the sector index origin.	      *
*		    This allows to reuse the source as is, in programs        *
*		     with different origin conventions.			      *
*									      *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"

#pragma warning(disable:4704)   // Ignore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BiosDiskRead					      |
|									      |
|   Description:    Read physical disk sectors, using a BIOS int 13 call.     |
|									      |
|   Parameters:     See below						      |
|									      |
|   Returns:	    The BIOS error code. 0 = success.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1994/07/28 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl BiosDiskRead(
WORD wDrive,			/* 0=A, 1=B, 0x80=C, 0x81=D, etc... */
WORD wCyl,			/* Cylinder */
WORD wHead,			/* Head */
WORD wSect,			/* Sector */
WORD n, 			/* Number of sectors to read */
char far *lpcBuffer)		/* Buffer for the data read */
    {
    int iErrCode;

#if (ISECT0 == 0)
    wSect += 1;			// BIOS indexes sectors starting from #1
#endif

    _asm     // Use int 13 fct 2
	{
	push es
	mov  ah, 2
	mov  al, byte ptr n
	mov  cx, wCyl
	xchg ch, cl		  ; Move bits <7:0> of cylinder to CH
	shl  cl, 1		  ; Move bits <9:8> of cylinder to bits <7:6> of CL
	shl  cl, 1
	shl  cl, 1
	shl  cl, 1
	shl  cl, 1
	shl  cl, 1
	or   cl, byte ptr wSect   ; Sector in CL<5:0>
	mov  dx, wDrive
	mov  dh, byte ptr wHead
	les  bx, lpcBuffer	  ; Buffer address
	int  13h		  ; Call BIOS
	mov  al, ah		  ; BIOS returns error code in AH
	xor  ah, ah		  ; AX = BIOS error code
	mov  iErrCode, ax	  ; Save the error code
	pop  es
	}

    return iErrCode;
    }
