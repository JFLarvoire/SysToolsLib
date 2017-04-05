/*****************************************************************************\
*                                                                             *
*   File name:	    DPMIGATE.C						      *
*									      *
*   Description:    Use DPMI to switch to 32 bits priviledged mode	      *
*									      *
*   Notes:	    This works under Windows, but not under NT or OS/2.       *
*									      *
*   History:								      *
*    1995/02/23 JFL Created this file					      *
*    1997/01/07 JFL Moved GetPMLinearAddress() to a separate file.            *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"
#include "utildef.h"
#include "pmode.h"

//+--------------------------------------------------------------------------
//+ Function   : PM2Ring0AndCallBack
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

DWORD PM2Ring0AndCallBack(PRING0CALLBACK pRing0CB, DWORD dwParam)
    {
    int err;
    int i;
    WORD wLdtSel;
    WORD wCode32Sel;
    WORD wGate32Sel;
    DWORD dwPriv32LinearAddress;
    LPDESCRIPTOR lpLDT;
    LPCALLGATEDESCRIPTOR lpCG;
    LPRING0CALLBACK lpGate32;
    WORD *pW;
    DWORD dwRet;

    /* Allocate a Selector into our LDT */
    wLdtSel = GetLDTSelfDesc(); 	    // Get a selector into the LDT
    if (!wLdtSel) return (DWORD)-1L;
    lpLDT = (LPDESCRIPTOR)((DWORD)wLdtSel << 16);  // C pointer to the LDT

    /* Build a Ring 0 flat 32 bits code selector */
    wCode32Sel = AllocSelector(0);	    // Allocate a new selector
    if (!wCode32Sel) return (DWORD)-1L;
    i = (int)(wCode32Sel >> 3); 	    // Index into the LDT
    lpLDT[i].Access_Rights = Code_Type;     // Define it as a code segment
    lpLDT[i].Extra_Rights = D_PAGE32;	    // 32 bits default
    err = !SetSelectorBase(wCode32Sel, 0);
    err |= SetSelectorLimit(wCode32Sel, 0xFFFFFFFFL);
    if (err) return (DWORD)-1L;

    /* Get the linear address of our Ring 0 callback */
    lpGate32 = (LPRING0CALLBACK)pRing0CB;
#if (defined(M_I86TM) || defined(M_I86SM) || defined(M_I86CM))
    // For small code memory models, replace the selector with our CS
    pW = (WORD *)(& lpGate32);
    pW[1] = GetCS();
#endif
    dwPriv32LinearAddress = GetPMLinearAddress(lpGate32);

    /* Build a Call Gate to our Ring 0 routine */
    wGate32Sel = AllocSelector(0);	    // Allocate a new selector
    i = (int)(wGate32Sel >> 3); 	    // Index into the LDT
    lpCG = (LPCALLGATEDESCRIPTOR)(lpLDT+i);
    lpCG->Offset_0_15 = (WORD)dwPriv32LinearAddress;
    lpCG->Selector = wCode32Sel & RPL_CLR;
    lpCG->DWord_Count = 1;
    lpCG->Access_Rights = GATE32_RING3;
    lpCG->Offset_16_31 = (WORD)(dwPriv32LinearAddress >> 16);

    lpGate32 = (LPRING0CALLBACK)((DWORD)wGate32Sel << 16);
    (*lpGate32)(dwParam);   /* Returns a result in EAX */
    dwRet = ReturnEAX();

    FreeSelector(wGate32Sel);
    FreeSelector(wCode32Sel);

    return dwRet;
    }
