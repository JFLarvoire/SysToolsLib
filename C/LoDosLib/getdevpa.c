/*****************************************************************************\
*									      *
*   File name:	    getdevpa.c						      *
*									      *
*   Description:    Get device parameters routine			      *
*									      *
*   Notes:								      *
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
|   Function:	    GetDeviceParams					      |
|									      |
|   Description:    Get a drive parameters				      |
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

int _cdecl GetDeviceParams(
int iDrive,		      /* 0=A, 1=B, 2=C, etc... */
DEVICEPARAMS *pdp)
{
   int iErrCode;

   pdp->dpSpecFunc = 1;       // Get infos about the current medium in the drive
   _asm
   {
      mov   bx, iDrive
      inc   bx		      ; 1=A, 2=B, 3=C, etc...
      mov   dx, pdp           ; Structure is malloc-ed, so DS is already OK.
      mov   ax, 440DH         ; Function IOCTL for block devices
      mov   cx, 0860H         ; Category 08, subfunction 60: Get device params
      int   21H
      jc    GDP_error	      ; Jump if error
      xor   ax, ax
GDP_error:
      mov   iErrCode, ax      ; Save the error code
   }

   return iErrCode;
}
