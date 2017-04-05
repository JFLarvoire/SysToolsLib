/*****************************************************************************\
*									      *
*   File name:	    setdevpa.c						      *
*									      *
*   Description:    Set device parameters routine			      *
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
|   Function:	    SetDeviceParams					      |
|									      |
|   Description:    Set a drive parameters				      |
|									      |
|   Parameters:     See below						      |
|									      |
|   Returns:	    The DOS error code. 0 = success.			      |
|									      |
|   Notes:	    Don't forget to set the pdp->dpSpecFunc field, which      |
|		     tells if the function is to process a DEVICEPARAMS or    |
|		     DEVICEPARAMSEX structure.				      |
|		    Bit 0   0=Use dpBytesPerSec to dpHugeSectors. 1=Ignore.   |
|		    Bit 1   0=Use DEVICEPARAMS. 1=Use DEVICEPARAMSEX	      |
|		    Bit 2   Must always be 1.				      |
|									      |
|   History:								      |
|									      |
|    1994/07/28 JFL Created this routine				      |
|    1995/09/08 JFL Removed the initialization of dpSpecFunc. It's the caller |
|		     that should do it, depending on the structure type.      |
*                                                                             *
\*---------------------------------------------------------------------------*/

int _cdecl SetDeviceParams(
int iDrive,		      /* 0=A, 1=B, 2=C, etc... */
DEVICEPARAMS *pdp)
{
   int iErrCode;

   _asm
   {
      mov   bx, iDrive
      inc   bx		      ; 1=A, 2=B, 3=C, etc...
      mov   dx, pdp           ; Structure is malloc-ed, so DS is already OK.
      mov   ax, 440DH         ; Function IOCTL for block devices
      mov   cx, 0840H         ; Category 08, subfunction 40: Set device params
      int   21H
      jc    GDP_error	      ; Jump if error
      xor   ax, ax
GDP_error:
      mov   iErrCode, ax      ; Save the error code
   }

   return iErrCode;
}
