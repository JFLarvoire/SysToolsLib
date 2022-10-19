/*****************************************************************************\
*                                                                             *
*   Filename:	    HDiskDos.cpp					      *
*									      *
*   Description:    MS-DOS hard disk access routines			      *
*                                                                             *
*   Notes:	    This code implements the OS-independant hard disk I/O     *
*		    routines for MS-DOS.				      *
*									      *
*		    OS-Independant routines are called HardDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    For MS-DOS, all access are done through the BIOS.	      *
*		    16-bits-BIOS-dependant routines are called BiosXxxxx().   *
*									      *
*		    TO DO: Implement Read-Only mode in HardDiskOpen().	      *
*									      *
*   History:								      *
*    2000-09-21 JFL Created harddisk.cpp.				      *
*    2001-01-10 JFL Changed GetBiosDiskXxxx buffer to a far pointer.   	      *
*    2001-01-22 JFL Added functions BiosDiskReadLba() and BiosDiskWriteLba(). *
*		    Use them first in HardDiskRead() and HardDiskWrite().     *
*    2001-02-26 JFL Fixed bug: Process flag iReadOnly in BiosDiskWriteLba too.*
*    2001-06-18 JFL Moved BIOS definitions to int13.h.			      *
*    2001-09-05 JFL Moved all MS-DOS routines into file HDiskDos.cpp.	      *
*    2002-04-15 JFL Added write protection flag in iDrive bit 15.	      *
*    2016-04-27 JFL Check the buffer size in GetBiosDiskChsParameters().      *
*    2017-07-28 JFL GetBiosDiskChsParameters() gets the actual sector size    *
*		    for floppys.					      *
*		    Fixed all HardDiskXxxx() routines, to clear the R/O flag  *
*		    in iDisk.						      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "HardDisk.h"	// Public definitions for this module.

#include <stdint.h>	// For intptr_t

#ifndef MINICOMS
#include <memory.h>	// For memset().
#endif

#include "int13.h"	// BIOS interrupt 13H definitions.

#define ISECT0 1	// Index of the first sector in a C/H/S track.

// Important: All MS_DOS data structures must be packed on a
// one-byte boundary.

#ifndef __MSDOS_H__
#pragma pack(1)
typedef struct _MID {
    WORD  midInfoLevel;
    DWORD midSerialNum;
    char  midVolLabel[11];
    char  midFileSysType[8];
} MID, *PMID;
#pragma pack()
#endif // !defined(__MSDOS_H__)

/*===========================================================================*\
*                                                                             *
"                               BIOS Routines                                 "
*                                                                             *
\*===========================================================================*/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BiosDiskReadChs					      |
|									      |
|   Description:    Read physical disk sectors, using a BIOS int 13 call.     |
|									      |
|   Parameters:     See below						      |
|									      |
|   Returns:	    The BIOS error code. 0 = success.			      |
|									      |
|   Notes:								      |
|									      |
|   Updates:								      |
|									      |
|    1994/07/28 JFL Created this routine				      |
*                                                                             *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)   // Ignore the inline assembler etc... warning

int _cdecl BiosDiskReadChs(
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

#ifdef _DEBUG
    if (iDebug) printf("BiosDiskReadChs(wDrive=%X, wCyl=%d, wHead=%d, wSect=%d, n=%d)\n",
    			wDrive, wCyl, wHead, wSect, n);
#endif // _DEBUG

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

#pragma warning(default:4704)   // Restore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BiosDiskWriteChs					      |
|									      |
|   Description:    Write physical disk sectors, using a BIOS int 13 call.    |
|									      |
|   Parameters:     See below						      |
|									      |
|   Returns:	    The BIOS error code. 0 = success.			      |
|									      |
|   Notes:								      |
|									      |
|   Updates:								      |
|									      |
|    1995/09/18 JFL Created this routine				      |
|    2002/04/15 JFL Added write protection flag in wDrive bit 15.	      |
*                                                                             *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)   // Ignore the inline assembler etc... warning

int _cdecl BiosDiskWriteChs(
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

#ifdef _DEBUG
    if (iDebug) printf("BiosDiskWriteChs(wDrive=%X, wCyl=%d, wHead=%d, wSect=%d, n=%d)\n",
    			wDrive, wCyl, wHead, wSect, n);

    if (iReadOnly) printf("Read-only mode! Write canceled.\n");
#endif // _DEBUG
    if (iReadOnly) return 0;
    if (wDrive & 0x8000) return 3;	// Write protection error

    _asm     // Use int 13 fct 3
	{
	push es
	mov  ah, 3
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

#pragma warning(default:4704)   // Restore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetBiosDiskChsParameters				      |
|									      |
|   Description:    Get a Hard Disk ISA-compatible CHS parameters.	      |
|									      |
|   Parameters:     int iDrive          Driver number (0x80-0xFF)	      |
|		    DDPARMS far *lpBuf  Output buffer			      |
|		    int iSize	        Buffer size 			      |
|									      |
|   Returns:	    int iError	        BIOS error. 0 = Success.	      |
|									      |
|   Notes:	    Uses the INT 13H BIOS function 8.			      |
|									      |
|		    Stores the result in a DDPARMS structure, to be able      |
|		     to compare results with that of function 48H.	      |
|									      |
|   History:								      |
|    2000-06-09 JFL Created this routine				      |
|    2001-01-10 JFL Changed the buffer parameter to a far pointer.     	      |
|    2017-07-28 JFL Get the actual sector size for floppys.		      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

int _cdecl GetBiosDiskChsParameters(int iDisk, DDPARMS far *lpBuf, int iSize)
    {
    int iResult;

#ifdef _DEBUG
    if (iDebug) printf("GetBiosDiskChsParameters(iDisk=0x%X, lpBuf=%Fp, iSize=0x%X)\n",
    			iDisk, lpBuf, iSize);
#endif // _DEBUG

    if (iSize < sizeof(DDPARMS)) return -1;

    _asm
	{
	push	es
	push	di

	mov	dx, iDisk
	mov	ah, 08H
	xor	di, di
	mov	es, di		; Clear ES:DI to see if the BIOS sets it
	int	13h
	jc	int13done	; Failed. BIOS error in AH
	; But the function can succeed even if the iDisk number is larger than the # of drives
	mov	al, BYTE PTR iDisk
	and	al, 7FH		; Clear the HD bit (bit #7)
	cmp	al, dl		; Was this a valid disk number?
	mov	ah, 1		; Invalid parameter error
	jae	int13done

	; Compute the sector size. The BIOS supports sizes != 512 for floppys only.
	mov	bx, 200H	; Default sector size
	mov	ax, es
	or	ax, di		; Did the BIOS set a Drive Parameter table?
	jz	no_DPT
	mov	bx, 80H		; Minimum sector size = 128
	mov	al, cl		; Save CL
	mov	cl, es:[di+3]	; Bytes per sector (00h = 128, 01h = 256, 02h = 512, 03h = 1024)
	shl	bx, cl		; Bytes per sector
	mov	cl, al		; Restore CL
no_DPT:
	push	bx		; Sector size

	cld
	les	di, lpBuf

	; Output a function-48H-compatible parameter table
	mov	ax, 1AH		; sizeof(DDPARMS)
	stosw          		; 00 wSize

	mov	ax, 2		; Cyl/Head/Sect info is valid
	stosw         		; 02 wInfo

	mov	al, cl
	shl	ax, 1
	shl	ax, 1
	mov	al, ch
	inc	ax		; Number = Max + 1
	stosw     		; 04 dwCyls
	push	ax
	xor	ax, ax
	stosw

	mov	al, dh
	inc	ax		; Number = Max + 1
	stosw     		; 08 dwHeads
	push	ax
	xor	ax, ax
	stosw

	mov	al, cl
	and	al, 3FH		; Number = Max
	stosw     		; 0C dwSects
	push	ax
	xor	ax, ax
	stosw

	pop	ax		; AL = Sectors
	pop	bx		; BL = Heads
	mul	bl		; AX = product
	pop	bx		; BX = Cylinders
	mul	bx		; DX:AX = Product
	stosw			; 10 qwTotal
	mov	ax, dx
	stosw
	xor	ax, ax
	stosw
	stosw

	pop	ax		; Sector size
	stosw			; 18 wBpS

	xor	ax, ax

int13done:
	mov	al, ah		; Get the return code into AX
	xor	ah, ah
	mov	iResult, ax

	pop	di
	pop	es
	}

    return iResult;
    }

#pragma warning(default:4704)   // Restore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetBiosDiskParameterTable				      |
|									      |
|   Description:    Get a Hard Disk parameter table.			      |
|									      |
|   Parameters:     int iDrive          Drive number (0x80-0xFF)	      |
|		    DDPARMS far *lpBuf  Output buffer			      |
|		    int iSize	        Buffer size 			      |
|									      |
|   Returns:	    int iError	        BIOS error. 0 = Success.	      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1998/06/22 JFL Created this routine				      |
|    2000/06/07 JFL Added call to function 8 in case function 48 fails.	      |
|		    This allows to support floppys under plain old DOS.	      |
|    2001/01/10 JFL Changed the buffer parameter to a far pointer.     	      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

int _cdecl GetBiosDiskParameterTable(int iDisk, DDPARMS far *lpBuf, int iSize)
    {
    int iErr;

    // First try function 48H, which supports very large disks.
    _asm
	{
	push	ds
	push	si
	lds	si, lpBuf
	mov	ax, iSize
	mov	word ptr [si], ax   ; Set the buffer size
	mov	dx, iDisk
	mov	ah, 48H
	int	13h
	jc	int13done
	xor	ax, ax
int13done:
	mov	al, ah		    ; Get the return code into AX
	xor	ah, ah
	mov	iErr, ax
	pop	si
	pop	ds
	}

    // If unsupported, try the old ISA function 8.
    if (iErr == 1) iErr = GetBiosDiskChsParameters(iDisk, lpBuf, iSize);

    return iErr;
    }

#pragma warning(default:4704)   // Restore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BiosDiskWriteLba					      |
|									      |
|   Description:    Write N sectors from the hard disk, using LBA BIOS calls. |
|									      |
|   Parameters:     int iDrive          	Drive number (0x80-0xFF)      |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void far *lpBuf             Input buffer.                 |
|									      |
|   Returns:	    0 = Success; 1=Invalid function; Else disk error.	      |
|									      |
|   Notes:	    Uses int 13H function 43H for LBA access.		      |
|									      |
|   History:								      |
|									      |
|    2001/01/22 JFL Created this routine				      |
|    2002/04/15 JFL Added write protection flag in iDrive bit 15.	      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

int BiosDiskWriteLba(int iDrive, QWORD qwSector, WORD wNum, void far *lpBuf)
    {
    int iErr;
    EDDPACKET eddPacket;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("BiosDiskWriteLba(iDrive=%X, LBA=%s, N=%X, Buf@=%Fp)\n",
    			iDrive, qwtox(qwSector, szqw), wNum, lpBuf);
	}
    if (iReadOnly) printf("Read-only mode! Write canceled.\n");
#endif // _DEBUG
    if (iReadOnly) return 0;
    if (iDrive & 0x8000) return 3;	// Write protected error

    memset(&eddPacket, 0, sizeof(EDDPACKET));
    eddPacket.bSize = sizeof(EDDPACKET);
    eddPacket.bNumber = (BYTE)wNum;
    eddPacket.dwBuffer = (DWORD)lpBuf;
    eddPacket.qwLBA = qwSector;

    _asm
    	{
    	push	si
	lea	si, WORD PTR eddPacket
	mov	dx, iDrive
	mov	ax, 4300H
	int	13H
	mov	al, ah		  ; BIOS returns error code in AH
	xor	ah, ah		  ; AX = BIOS error code
	mov	iErr, ax
	pop	si
    	}

    return iErr;
    }

#pragma warning(default:4704)   // Restore the inline assembler etc... warning

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    BiosDiskReadLba					      |
|									      |
|   Description:    Read N sectors from the hard disk, using LBA BIOS calls.  |
|									      |
|   Parameters:     int iDrive          	Drive number (0x80-0xFF)      |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void far *lpBuf             Output buffer.                |
|									      |
|   Returns:	    0 = Success; 1=Invalid function; Else disk error.	      |
|									      |
|   Notes:	    Uses int 13H function 42H for LBA access.		      |
|									      |
|   History:								      |
|									      |
|    2001/01/22 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)	// Ignore the inline assembler etc... warning

int BiosDiskReadLba(int iDrive, QWORD qwSector, WORD wNum, void far *lpBuf)
    {
    int iErr;
    EDDPACKET eddPacket;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("BiosDiskReadLba(iDrive=%X, LBA=%s, N=%X, Buf@=%Fp)\n",
    			iDrive, qwtox(qwSector, szqw), wNum, lpBuf);
	}
#endif // _DEBUG

    memset(&eddPacket, 0, sizeof(EDDPACKET));
    eddPacket.bSize = sizeof(EDDPACKET);
    eddPacket.bNumber = (BYTE)wNum;
    eddPacket.dwBuffer = (DWORD)lpBuf;
    eddPacket.qwLBA = qwSector;

    _asm
    	{
    	push	si
	lea	si, WORD PTR eddPacket
	mov	dx, iDrive
	mov	ah, 42H
	int	13H
	mov	al, ah		  ; BIOS returns error code in AH
	xor	ah, ah		  ; AX = BIOS error code
	mov	iErr, ax
	pop	si
    	}

    return iErr;
    }

#pragma warning(default:4704)   // Restore the inline assembler etc... warning

/*===========================================================================*\
*                                                                             *
"                     OS-independant family, for MS-DOS                       "
*                                                                             *
\*===========================================================================*/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskOpen					      |
|									      |
|   Description:    Get a handle for a given hard disk.			      |
|									      |
|   Parameters:     int iDisk	    Hard Disk number. 0=1st hard disk, etc.   |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns:	    The hard disk handle, or NULL if no such hard disk.	      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2001/02/20 JFL Created this routine				      |
|    2002/04/15 JFL Added write protection flag in handle bit 15.	      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE HardDiskOpen(int iDrive, int iMode)
    {
    int iErr;
    DDPARMS hdParms;

#ifdef _DEBUG
    if (iDebug)
        {
        printf("HardDiskOpen(iDrive=%d)\n", iDrive);
	}
#endif // _DEBUG

    // Get the BIOS drive number
    iDrive += 0x80;

    // Check if it exists
    iErr = GetBiosDiskParameterTable(iDrive, &hdParms, sizeof(hdParms));
    if (iErr) return NULL;

    if (iMode) iDrive += 0x8000;	// Report R/O flag
    return (HANDLE)(intptr_t)iDrive;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskClose					      |
|									      |
|   Description:    Release resources reserved by HardDiskOpen.		      |
|									      |
|   Parameters:     HANDLE hDisk    Hard Disk handle.			      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2001/02/20 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4100) /* Avoid warnings "unreferenced formal parameter" */

void HardDiskClose(HANDLE hDrive)
    {
    return;
    }

#pragma warning(default:4100)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskGetGeometry					      |
|									      |
|   Description:    Get the geometry of the hard disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    HDGEOMETRY *pHdGeometry	Buffer for the results.	      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2000/09/11 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskGetGeometry(HANDLE hDrive, HDGEOMETRY *pHdGeometry)
    {
    int iDisk = (int)(DWORD)(LPVOID)hDrive & 0xFF;
    int iErr;
    DDPARMS hdParms;

    // Get the physical parameters for the drive.
    hdParms.lpedd = (EDDPARMS far *)-1L;
    iErr = GetBiosDiskParameterTable(iDisk, &hdParms, sizeof(hdParms));
    if (iErr) return iErr;

    pHdGeometry->qwSectors = hdParms.qwTotal;
    pHdGeometry->wSectorSize = hdParms.wBpS;
    pHdGeometry->dwCyls = hdParms.dwCyls;
    pHdGeometry->dwHeads = hdParms.dwHeads;
    pHdGeometry->dwSects = hdParms.dwSects;
    // Assume no translation.
    pHdGeometry->dwXlatCyls = hdParms.dwCyls;
    pHdGeometry->dwXlatHeads = hdParms.dwHeads;
    pHdGeometry->dwXlatSects = hdParms.dwSects;

#ifdef _DEBUG
	// Display extended parameters for the curious.
	if (iVerbose && (hdParms.lpedd != (EDDPARMS far *)-1L))
	    {
	    int i;
	    WORD w;

	    i = hdParms.lpedd->bRevision;
	    printf("Extended parameter table revision %d.%d:\n", i>>4, i&0x0F);
	    if (iVerbose) printf("At address %lp\n", hdParms.lpedd);
	    if (hdParms.lpedd->bFlags & 0x40)
		printf("LBA enabled.\n");
	    else
		printf("LBA disabled.\n");
	    printf("IRQ%d\n", hdParms.lpedd->bIrq);
	    printf("DMA type %d, ", hdParms.lpedd->bDma >> 4);
	    printf("on DMA channel %d\n", hdParms.lpedd->bDma & 0x0F);
	    printf("PIO type %d\n", hdParms.lpedd->bPio & 0x0F);
	    w = hdParms.lpedd->wOptions;
	    if (w & 0x0001) printf("Fast PIO enabled.\n");
	    if (w & 0x0002) printf("Fast DMA enabled.\n");
	    if (w & 0x0004) printf("Block PIO enabled.\n");
	    if (w & 0x0008) printf("CHS translation enabled.\n");
	    if (w & 0x0010) printf("LBA translation enabled.\n");
	    if (w & 0x0020) printf("Removable media.\n");
	    if (w & 0x0040) printf("ATAPI device (CDROM).\n");
	    if (w & 0x0080) printf("32-bits transfer mode.\n");
	    if (w & 0x0100) printf("ATAPI uses DRQ.\n");
	    switch ((w >> 9) & 3)
		{
		case 0: printf("Phoenix bit-shifting translation.\n"); break;
		case 1: printf("LBA-assisted translation.\n"); break;
		case 3: printf("proprietary translation.\n"); break;
		default: printf("reserved translation.\n"); break;
		}
	    }
#endif // _DEBUG

    // Get the BIOS-translated parameters
    iErr = GetBiosDiskChsParameters(iDisk, &hdParms, sizeof(hdParms));
    if (iErr) return 0;	// Use the no-translation assumption.

    pHdGeometry->dwXlatCyls = hdParms.dwCyls;
    pHdGeometry->dwXlatHeads = hdParms.dwHeads;
    pHdGeometry->dwXlatSects = hdParms.dwSects;

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskLBA2CHS					      |
|									      |
|   Description:    Convert LBA coordinates to Cylinder/Head/Sector.	      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    QWORD qwSector              LBA Sector number.            |
|		    WORD *pCyl                  Cylinder	              |
|		    WORD *pHead                 Head    	              |
|		    WORD *pSect                 Sector  	              |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2000/09/15 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskLBA2CHS(HANDLE hDrive, QWORD qwSector, WORD *pwCyl, WORD *pwHead, WORD *pwSect)
    {
    int iDisk = (int)(DWORD)(LPVOID)hDrive & 0xFF;
    int iErr;
    DDPARMS hdParms;
    DWORD dw;

    /* If the BIOS does not support LBA access, use CHS parameters. */
#ifdef __cplusplus
    if (qwSector > 0xFFFFFFFF) return 256;	// Overflow.
    dw = (DWORD)qwSector;
#else
    if (qwSector.dw1) return 256;		// Overflow.
    dw = qwSector.dw0;
#endif

    // Get the BIOS-translated parameters
    iErr = GetBiosDiskChsParameters(iDisk, &hdParms, sizeof(hdParms));
    if (iErr) return iErr;

    *pwSect = (WORD)(dw % hdParms.dwSects) + 1;
    dw /= hdParms.dwSects;
    *pwHead = (WORD)(dw % hdParms.dwHeads);
    dw /= hdParms.dwHeads;
    *pwCyl = (WORD)(dw % hdParms.dwCyls);
    dw /= hdParms.dwCyls;

    return (dw != 0) ? 256 : 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskRead					      |
|									      |
|   Description:    Read N sectors from the hard disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2000/09/12 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskRead(HANDLE hDrive, QWORD qwSector, WORD wNum, void far *pBuf)
    {
    int iDisk = (int)(DWORD)(LPVOID)hDrive & 0xFF;
    int iErr;
    WORD wCyl;			/* Cylinder */
    WORD wHead;			/* Head */
    WORD wSect;			/* Sector */

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("HardDiskRead(hDrive=%lX, LBA=%s, N=%X, Buf@=%Fp)\n",
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
#endif // _DEBUG

    // First use int 13H function 42H for LBA access.
    iErr = BiosDiskReadLba(iDisk, qwSector, wNum, pBuf);
    if (iErr != 1) return iErr;	// Success, or disk I/O error.

    /* If the BIOS does not support LBA access, use CHS parameters. */
    iErr = HardDiskLBA2CHS(hDrive, qwSector, &wCyl, &wHead, &wSect);
    if (iErr) return iErr;

    iErr = BiosDiskReadChs((WORD)iDisk, wCyl, wHead, wSect, wNum, (char far *)pBuf);
    return iErr;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskWrite					      |
|									      |
|   Description:    Write N sectors to the hard disk.			      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2000/09/12 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskWrite(HANDLE hDrive, QWORD qwSector, WORD wNum, void far *pBuf)
    {
    int iDisk = (int)(DWORD)(LPVOID)hDrive & 0xFF;
    int iErr;
    WORD wCyl;			/* Cylinder */
    WORD wHead;			/* Head */
    WORD wSect;			/* Sector */

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("HardDiskWrite(hDrive=%lX, LBA=%s, N=%X, Buf@=%Fp)\n",
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
    // if (iReadOnly) { printf("Read-only mode! Write canceled.\n"); return 0; }
    // Unnecessary as the Write security is actually done in BiosXxxx() routines.
#endif // _DEBUG

    // First use int 13H function 43H for LBA access.
    iErr = BiosDiskWriteLba(iDisk, qwSector, wNum, pBuf);
    if (iErr != 1) return iErr;	// Success, or disk I/O error.

    /* If the BIOS does not support LBA access, use CHS parameters. */
    iErr = HardDiskLBA2CHS(hDrive, qwSector, &wCyl, &wHead, &wSect);
    if (iErr) return iErr;

    iErr = BiosDiskWriteChs((WORD)iDisk, wCyl, wHead, wSect, wNum, (char far *)pBuf);
    return iErr;
    }

/*===========================================================================*\
*                                                                             *
"                          Spare MS-DOS Routines                              "
*                                                                             *
\*===========================================================================*/

#if 0

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LockLogicalVolume					      |
|									      |
|   Description:    Perform a logical volume lock			      |
|									      |
|   Parameters:     int iDrive	    DOS drive number. 0=A; 1=B; 2=C; etc...   |
|		    WORD wLockLevel 0, 1, 2, or 3.			      |
|		    WORD wPermissions bit 0 = Allow writes		      |
|				      bit 1 = Prevent file mapping (?)	      |
|				      bit 2 = Allow formatting		      |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|									      |
|   Notes:	    See the Win32 SDK, Guides, Programmer's Guide to Win 95,  |
|		     Using MS-DOS extensions, Exclusive Volume Locking.       |
|                                                                             |
|   Updates:								      |
|									      |
|    1994/09/04 JFL Created this routine.				      |
|    1996/08/08 JFL Added MS-DOS version testing.			      |
*									      *
\*---------------------------------------------------------------------------*/

int LockLogicalVolume(int iDrive, WORD wLockLevel, WORD wPermissions)
    {
    if (_dos_version() < 0x700) return 1;   // Unsupported before Win95.
    return DevIoCtl(0x4A, wPermissions, (wLockLevel << 8) | (BYTE)(iDrive+1));
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    UnlockLogicalVolume 				      |
|									      |
|   Description:    Perform a logical volume unlock			      |
|									      |
|   Parameters:     int iDrive	    DOS drive number. 0=A; 1=B; 2=C; etc...   |
|									      |
|   Returns:	    The DOS error code. 0=Success.			      |
|									      |
|   Notes:	    See the Win32 SDK, Guides, Programmer's Guide to Win 95,  |
|		     Using MS-DOS extensions, Exclusive Volume Locking.       |
|                                                                             |
|   Updates:								      |
|									      |
|    1994/09/04 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

int UnlockLogicalVolume(int iDrive)
    {
    return DevIoCtl(0x6A, 0, iDrive+1);
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    LockDosVolume					      |
|									      |
|   Description:    Lock an MS-DOS volume, for direct sector access.	      |
|									      |
|   Parameters:     int iDosDrive   MS-DOS drive number 		      |
|									      |
|   Output:	    int number of unlocks to call. -1=Failure.		      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1996/08/21 JFL initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

int LockDosVolume(int iDosDrive)
    {
    int iErr;
    int i;

    // Useless up to MS-DOS 6.22
    if (_dos_version() < 0x700) return 0;

    // First attempt a level 0 lock
    iErr = LockLogicalVolume(iDosDrive, 0, 1);
    if (!iErr) return 1;	// Success
    if (iErr != 5) return -1;	// Failure

    // If lock denied, then it's necessary to get locks 1 to 3
    for (i=1; i<=3; i++)
	{
	iErr = LockLogicalVolume(iDosDrive, i, 1);
	if (iErr) break;
	}
    if (i>3) return 3;		// We got all three locks. Success.
    // Else undo partial locks
    UnlockDosVolume(iDosDrive, i-1);
    return -1;			// Failure
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    UnlockDosVolume					      |
|									      |
|   Description:    Unlock an MS-DOS volume, for direct sector access.	      |
|									      |
|   Parameters:     int iDosDrive   MS-DOS drive number 		      |
|		    int iUndo	    Number of locks to undo		      |
|									      |
|   Output:	    None						      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1996/08/21 JFL initial implementation.				      |
|    2000/06/06 JFL Added safety against negative iUndo parameters.	      |
*									      *
\*---------------------------------------------------------------------------*/

void UnlockDosVolume(int iDosDrive, int iUndo)
    {
    while (iUndo-- > 0) UnlockLogicalVolume(iDosDrive);
    return;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskLock					      |
|									      |
|   Description:    Lock the hard disk, to allow direct sector access.	      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2000/09/18 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskLock(HANDLE hDrive, HANDLE *phLock)
    {
    int iDisk = (int)(DWORD)(LPVOID)hDrive & 0xFF;
    int iErr;
    int iUndoLock = 0;		// Number of locks obtained on the drive

    // To do: Change BIOS drive # into MS-DOS drives #.

    // Lock the drive. This is necessary for writing a sector.
    if ((iDosDrive >= 0) && (iOrder < 0))
	{
	iUndoLock = LockDosVolume(iDosDrive);
	if (iUndoLock == -1)
	    {
	    // printf("Cannot lock volume %c:\n", cDrive);
	    return -1;
	    }
#ifdef _DEBUG
	if (iVerbose) printf("Obtained %d locks on drive %c.\n", iUndoLock, cDrive);
#endif // _DEBUG
	// The drive is now locked. It must be unlocked before exiting.
	}

    *phLock = (HANDLE)iUndoLock;
    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDiskUnlock					      |
|									      |
|   Description:    Unlock the hard disk, to disallow direct sector access.   |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    int iUndoLock					      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the HardDiskXxxx family. |
|									      |
|   History:								      |
|									      |
|    2000/09/18 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDiskUnlock(HANDLE hDrive, HANDLE hLock)
    {
    int iDisk = (int)(DWORD)(LPVOID)hDrive & 0xFF;
    int iUndoLock = (int)(DWORD)hLock;

    if (iUndoLock > 0) UnlockDosVolume(iDosDrive, iUndoLock);

    return 0;
    }

#endif // Keep sample lock/unlock code hidden.

