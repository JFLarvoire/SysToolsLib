/*****************************************************************************\
*                                                                             *
*   Filename:	    IsMBR.c						      *
*									      *
*   Description:    Check if a buffer contains a (master) boot sector.	      *
*                                                                             *
*   Notes:	    The heuristics used for this test are good but do not     *
*		    guaranty a correct answers in 100% of cases.              *
*									      *
*		    To do: add improvements as mentioned inline.	      *
*									      *
*   History:								      *
*    2001/12/19 JFL Created this file from routines in in13emu.cpp.	      *
*    2002/07/01 JFL Fixed a bug in IsBS(). Added a second parameter.	      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define FALSE 0
#define TRUE 1

#include "IsMBR.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsBS						      |
|									      |
|   Description:    Test if a buffer contains a boot sector parameter block.  |
|									      |
|   Parameters:     char *pBuf						      |
|									      |
|   Returns:	    TRUE or FALSE					      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    2001/06/19 JFL Created this routine				      |
|    2002/07/01 JFL Workaround a bug that causes a GPF in WIN32 version:      |
|		    If the bsBytesPerSec field is invalid, it may cause an    |
|		    access beyond the allocated buffer limit. So make sure    |
|		    that the field is plausible, by ensuring it's a multiple  |
|		    of 128, between 128 and the buffer size.		      |
|		    This forces to modify the function definition by adding   |
|		    another argument with the buffer size.	              |
*									      *
\*---------------------------------------------------------------------------*/

int IsBS(char *pBuf, unsigned int uBufSize)
    {
    BOOTSECTOR *pBS = (BOOTSECTOR *)pBuf;

    /* Eliminate all invalid cases */
    
    /* Check if the BytesPerSec field is valid. */
    if (   (pBS->bsBytesPerSec & 0x7F)		/* It must be a multiple of 128 */
        || (pBS->bsBytesPerSec < 128)		/* between 128 */
        || (pBS->bsBytesPerSec > uBufSize)	/* and the buffer size */
       ) return FALSE;
    
    /* First there must be a valid 55AA signature in the end. */
    if (*(WORD *)(pBuf + pBS->bsBytesPerSec - 2) != 0xAA55) return FALSE;
    
    /* There must be a jump around the BPB. */
    switch (pBS->bsJump[0])
        {
        case 0xE8:	/* Near call */
        case 0xE9:	/* Near jump */
        case 0xEB:	/* Short jump */
            break;		/* Valid instruction */
        default:
            return FALSE;	/* Invalid instruction */
        }
    
    /* Sector size and cluster size must be a power of 2 */
    if (pBS->bsBytesPerSec & (pBS->bsBytesPerSec - 1)) return FALSE;
    if (pBS->bsSecPerClust & (pBS->bsSecPerClust - 1)) return FALSE;
    
    /* There cannot be more than 2 FATs */
    if (pBS->bsFATs > 2) return FALSE;	

    /* # of Heads and # of sectors/track must be between 1 and 256. */
    if (pBS->bsSecPerTrack < 1) return FALSE;
    if (pBS->bsSecPerTrack > 256) return FALSE;
    if (pBS->bsHeads < 1) return FALSE;
    if (pBS->bsHeads > 256) return FALSE;

    /* Media ID must be greater than F0 */
    if (pBS->bsMedia < 0xF0) return FALSE;
    
    /* OK, this looks like a valid BPB */
    return TRUE;    
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsMBR						      |
|									      |
|   Description:    Test if a buffer contains a Master boot Record.	      |
|									      |
|   Parameters:     char *pBuf						      |
|									      |
|   Returns:	    TRUE or FALSE					      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    2001/06/19 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int IsMBR(char *pBuf)
    {
    int i;
    MASTERBOOTRECORD *pMBR = (MASTERBOOTRECORD *)pBuf;
    int nBootable = 0;

    /* First there must be a valid 55AA signature in the end. */
    if (pMBR->mbrSignature != 0xAA55) return FALSE;
    
    /* Eliminate all invalid cases */
    for (i=0; i<4; i++)	/* Scan the 4 partition table entries... */
        {
        int iBegCyl, iEndCyl;
        int iBegCHS, iEndCHS;

	/* Is this an empty entry... */
	if (pMBR->mbrPart[i].type == 0)
	    {
	    int j;
	    /* then it must be completely zeroed-out. */
	    char *pc = (char *)&(pMBR->mbrPart[i]);
	    for (j=0; j<sizeof(PARTITION); j++) if (pc[j]) return FALSE;
	    /* The rest of the tests are meaning-less. */
	    continue;
	    }

        /* Reconstruct cylinder numbers from their pieces. */
        iBegCyl = (pMBR->mbrPart[i].beg_hcyl << 8) + pMBR->mbrPart[i].beg_lcyl;
        iEndCyl = (pMBR->mbrPart[i].end_hcyl << 8) + pMBR->mbrPart[i].end_lcyl;
       
	/* The C/H/S fields must be both present or both absent. */
	iBegCHS = FALSE;
	if (iBegCyl || pMBR->mbrPart[i].beg_sect || pMBR->mbrPart[i].beg_head) iBegCHS = TRUE;
	iEndCHS = FALSE;
	if (iEndCyl || pMBR->mbrPart[i].end_sect || pMBR->mbrPart[i].end_head) iEndCHS = TRUE;
	if (iBegCHS != iEndCHS) return FALSE;
	
        /* The end cylinder cannot be before the beginning cylinder. */
	if (iEndCyl < iBegCyl) return FALSE;

	/* The sector number (if specified) cannot be 0. */
	if (iEndCHS)
	    {
	    if (pMBR->mbrPart[i].beg_sect == 0) return FALSE;
	    if (pMBR->mbrPart[i].end_sect == 0) return FALSE;
	    }

	/* The LBA/Size fields must be both present. */
	if ((!pMBR->mbrPart[i].first_sector) || (!pMBR->mbrPart[i].n_sectors)) return FALSE;

	/* The LBA/Size fields must be consistent with the C/H/S fields */
	/* TO BE DONE */

	/* The partitions may not overlap */
	/* TO BE DONE */
	
	/* Valid boot flag values are 0 and 0x80. 0x81 to 83 may also be accepted? */
	if (pMBR->mbrPart[i].boot & 0x7C) return FALSE;
	if (pMBR->mbrPart[i].boot != 0) nBootable += 1;
        }

    /* There must be only 1 active partition */
    if (nBootable > 1) return FALSE;

    /* OK, this looks like a valid MBR */
    return TRUE;    
    }

#ifdef __cplusplus
}
#endif

