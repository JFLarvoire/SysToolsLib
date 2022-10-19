/*****************************************************************************\
*                                                                             *
*   Filename:	    crc32.cpp						      *
*									      *
*   Description:    Compute and verify EFI CRCs				      *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2001/02/20 JFL Created this file.					      *
*    2001/02/26 JFL Changed crc32 param 2 from size_t to int.		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include <stdio.h>
#include <string.h>

#define NO_QWORD_CONSTRUCTORS	// Work around QWORD class in structure bug.

#include "crc32.h"

// Standard EFI definitions

#ifndef _EFI_INCLUDE_
#pragma warning(disable:4096) /* Avoid warnings "'cdecl' must be used with '...'" */
#include "efi.h"	// Main EFI definitions
#include "efilib.h"	// EFI library functions.
#endif // ifndef _EFI_INCLUDE_

#ifdef _DEBUG

extern "C" {
extern int iDebug;	// Defined in main module. If TRUE, display debug infos.
extern int iVerbose;	// Defined in main module. If TRUE, display progress infos.
extern int iReadOnly;	// Defined in main module. If TRUE, do not write to disk.
}

#endif // _DEBUG

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    crc32						      |
|									      |
|   Description:    Calculate the standard CRC32 of a buffer		      |
|									      |
|   Parameters:     void far *pBuf	Buffer				      |
|		    int iLen		Buffer size			      |
|									      |
|   Returns:	    The standard CRC-32					      |
|                                                                             |
|   Notes:          Short implementation, which is supposed to have bad	      |
|                   performance, but who cares with a 1 GHz processor?	      |
|                                                                             |
|   History:								      |
|									      |
|    2000/12/11 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#define POLY32  0x04C11DB7	// Ethernet CRC-32 polynom = (32,26,23,22,16,12,11,10,8,7,5,4,2,1,0)
#define POLY32X 0xEDB88320	// Bit reversal of 0x04C11DB7

DWORD crc32(void FAR *pBuf, int iLen)
    {
    DWORD dwCRC;
    unsigned char FAR *pb;
#ifdef _DEBUG
    int iLen0 = iLen;
#endif // _DEBUG

    for (dwCRC = 0xffffffff, pb = (unsigned char FAR *)pBuf; iLen; pb++, iLen--)
	{
	int iBit;
	
	dwCRC ^= *pb;
	for (iBit=0; iBit<8; iBit++)
	    {
	    DWORD dwCarry = (dwCRC & 1);
	    dwCRC >>= 1;
	    if (dwCarry) dwCRC ^= POLY32X;
            }
	}

#ifdef _DEBUG
    if (iDebug) printf("CRC of buffer @%Fp Length 0x%X = 0x%08lX\n", pBuf, iLen0, ~dwCRC);
#endif // _DEBUG

    return ~dwCRC;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SetCrc						      |
|									      |
|   Description:    Set the CRC32 of an EFI structure			      |
|									      |
|   Parameters:     EFI_TABLE_HEADER *Hdr	EFI header		      |
|									      |
|   Returns:	    Nothing						      |
|                                                                             |
|   Notes:          This routine is copied from EFI's Sample implementation.  |
|                   As such, its prototype is already in efi.h.               |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/05 JFL Copied from EFI Sample Implementation.		      |
*									      *
\*---------------------------------------------------------------------------*/

VOID
SetCrc (
    IN OUT EFI_TABLE_HEADER *Hdr
    )
/*++

Routine Description:

    Updates the CRC32 value in the table header

Arguments:

    Hdr     - The table to update

Returns:

    None

--*/
{
    SetCrcAltSize (Hdr->HeaderSize, Hdr);
}

VOID
SetCrcAltSize (
    IN UINTN                 Size,
    IN OUT EFI_TABLE_HEADER *Hdr
    )
/*++

Routine Description:

    Updates the CRC32 value in the table header

Arguments:

    Hdr     - The table to update

Returns:

    None

--*/
{
    Hdr->CRC32 = 0;
    Hdr->CRC32 = CalculateCrc((UINT8 *)Hdr, Size);
}


/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CheckCrc						      |
|									      |
|   Description:    Check the CRC32 of an EFI structure			      |
|									      |
|   Parameters:	    UINTN MaxSize               The maximum expected size     |
|		    EFI_TABLE_HEADER *Hdr	EFI header		      |
|									      |
|   Returns:	    TRUE/FALSE						      |
|                                                                             |
|   Notes:          This routine is copied from EFI's Sample implementation.  |
|                   As such, its prototype is already in efi.h.               |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/05 JFL Copied from EFI Sample Implementation.		      |
*									      *
\*---------------------------------------------------------------------------*/

BOOLEAN
CheckCrc (
    IN UINTN                 MaxSize,
    IN OUT EFI_TABLE_HEADER *Hdr
    )
/*++

Routine Description:

    Checks the CRC32 value in the table header

Arguments:

    Hdr     - The table to check

Returns:

    TRUE if the CRC is OK in the table

--*/
{
    return CheckCrcAltSize (MaxSize, Hdr->HeaderSize, Hdr);
}




BOOLEAN
CheckCrcAltSize (
    IN UINTN                 MaxSize,
    IN UINTN                 Size,
    IN OUT EFI_TABLE_HEADER *Hdr
    )
/*++

Routine Description:

    Checks the CRC32 value in the table header

Arguments:

    Hdr     - The table to check

Returns:

    TRUE if the CRC is OK in the table

--*/
{
    UINT32      Crc;
    UINT32      OrgCrc;
    BOOLEAN     f;

    if (Size == 0) {
        /* 
         *  If header size is 0 CRC will pass so return FALSE here
         */
        return FALSE;
    }
    if (MaxSize && Size > MaxSize) {
        DEBUG((D_ERROR, "CheckCrc32: Size > MaxSize\n"));
        return FALSE;
    }

    /*  clear old crc from header */
    OrgCrc = Hdr->CRC32;
    Hdr->CRC32 = 0;
    Crc = CalculateCrc((UINT8 *)Hdr, Size);

    /*  set restults */
    Hdr->CRC32 = OrgCrc;

    /*  return status */
    f = (BOOLEAN)(OrgCrc == Crc);
    if (!f) {
#ifdef _DEBUG
        if (iDebug) printf("But the CRC found in the header was %08lX.\n", OrgCrc);
#endif // _DEBUG
        DEBUG((D_ERROR, "CheckCrc32: Crc check failed\n"));
    }

    return f;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    CalculateCrc					      |
|									      |
|   Description:    Compute the CRC32 of a structure			      |
|									      |
|   Parameters:     UINT8 *pt		Structure address		      |
|		    UINTN Size		Structure size			      |
|									      |
|   Returns:	    The CRC32						      |
|                                                                             |
|   Notes:          This routine is adapted from EFI's Sample implementation. |
|                   As such, its prototype is already in efi.h.               |
|                                                                             |
|   History:								      |
|									      |
|    2001/01/05 JFL Adapted from EFI Sample Implementation, to use a short,   |
|		    table-less version of the standard CRC32 algorithm.	      |
*									      *
\*---------------------------------------------------------------------------*/

UINT32
CalculateCrc (
    UINT8 *pt,
    UINTN Size
    )
{
    return (UINT32)crc32(pt, (int)Size);
}

/*---------------------------------------------------------------------------*/ 
