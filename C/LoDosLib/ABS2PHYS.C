/*****************************************************************************\
*									      *
*   File name:	    abs2phys.c						      *
*									      *
*   Description:    Convert an absolute sector number to its physical coords  *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1995/09/05 JFL Created this file					      *
*    2000/07/25 JFL Use constant ISECT0 as the sector index origin.	      *
*		    This allows to reuse the source as is, in programs        *
*		     with different origin conventions.			      *
*									      *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"
#include "msdos.h"
#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    Abs2PhysSector					      |
|									      |
|   Description:    Convert a logical sector number into cylinder/head/sector |
|									      |
|   Parameters:     See below						      |
|									      |
|   Returns:	    Error code. 0 = Success.				      |
|                                                                             |
|   History:								      |
|									      |
|    1994/07/28 JFL Created for the OmniShare makepar utility		      |
|    1995/08/30 JFL Modified to avoid 32 bits math.			      |
|    1995/09/05 JFL Changed the 1st argument to DEVICEPARAMS * to prevent     |
|		     unnecessary calls to GetDeviceParameters.		      |
|    2000/07/25 JFL Use constant ISECT0 as the sector index origin.	      |
|		    This allows to reuse the source as is, in programs        |
|		     with different origin conventions.			      |
*									      *
\*---------------------------------------------------------------------------*/

void _cdecl Abs2PhysSector(
DEVICEPARAMS *pdp,	/* Parameters for the drive */
DWORD dwAbsSect,	/* Absolute sector number, ie. index in the partition */
WORD *pwCyl,            /* Cylinder */
WORD *pwHead,           /* Head */
WORD *pwSect)           /* Sector (0-based) */
    {
    WORD wSecPerCyl;
    WORD wSector;

    wSecPerCyl = pdp->dpSecPerTrack * pdp->dpHeads;

    dwAbsSect += pdp->dpHiddenSecs;

    *pwCyl = div32x16to16(dwAbsSect, wSecPerCyl);
    dwAbsSect -= mul16x16to32(*pwCyl, wSecPerCyl); // dwAbsSect %= wSecPerCyl
    wSector = (WORD)dwAbsSect;
    *pwHead = wSector / pdp->dpSecPerTrack;
    *pwSect = (wSector % pdp->dpSecPerTrack) + ISECT0;
    return;
    }

#if 0	// This is the initial implementation of the routine

int Log2PhysSector(
int iDrive,             /* 1=A, 2=B, 3=C, etc... */
DWORD dwLogSect,        /* Logical sector number, ie. index in the partition */
WORD *pwCyl,            /* Cylinder */
WORD *pwHead,           /* Head */
WORD *pwSect)           /* Sector (0-based) */
{
   DEVICEPARAMS *pdp;

   pdp = GetCachedDeviceParams(iDrive);
   if (!pdp) return 0xFFFF;

   dwLogSect += pdp->dpHiddenSecs;
   *pwSect = (WORD)(dwLogSect % pdp->dpSecPerTrack) + ISECT0;
   dwLogSect /= pdp->dpSecPerTrack;
   *pwHead = (WORD)(dwLogSect % pdp->dpHeads);
   dwLogSect /= pdp->dpHeads;
   *pwCyl = (WORD)dwLogSect;
   return 0;
}

#endif
