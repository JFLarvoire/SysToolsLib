/*****************************************************************************\
*                                                                             *
*   File name:	    GETPMLIN.C						      *
*									      *
*   Description:    Get PM Linear address       			      *
*									      *
*   Notes:	    							      *
*									      *
*   History:								      *
*    1997/01/07 JFL Extracted this routine from dpmigate.c.		      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"
#include "utildef.h"
#include "pmode.h"

//+--------------------------------------------------------------------------
//+ Function   : GetPMLinearAddress
//+
//+ Purpose    : Switch to 32 bits priviledged mode, call a routine, switch back
//+
//+ Ouput      : 0 = Success
//+		 !0 = Failures
//+
//+ Notes:     : Must already have switched to PM using DPMI
//+
//+ Creation   : 1995/02/23 by Jean-Francois LARVOIRE
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+
//+--------------------------------------------------------------------------

DWORD _cdecl GetPMLinearAddress(void far *lpAddr)
    {
    WORD sel;
    WORD off;

    if (!lpAddr) return 0;	// ~~jfl 95/10/24

    sel = FP_SEG(lpAddr);
    off = FP_OFF(lpAddr);

    return GetSelectorBase(sel) + off;
    }
