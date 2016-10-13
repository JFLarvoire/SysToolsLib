/*****************************************************************************\
*                                                                             *
*   Filename:	    Int13.h						      *
*									      *
*   Description:    Data structures used by BIOS int 13H service routines     *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2001-06-06 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _INT13_H_
#define _INT13_H_

#include "qword.h"	/* Defines BYTE, WORD, DWORD, QWORD, etc */

#pragma pack(1)

/* Hard disk parameters (Returned by int 13H function 8) */
typedef struct
    {
    WORD wCyls;		/* 00h    WORD    number of cylinders */
    BYTE bHeads;	/* 02h    BYTE    number of heads */
    BYTE bSignature;	/* 03h    BYTE    A0h (signature indicating translated table) */
    BYTE bPSects;	/* 04h    BYTE    number of physical sectors per track */
    WORD wPrecomp;	/* 05h    WORD    starting write precompensation cylinder number */
    BYTE bReserved;	/* 07h    BYTE    reserved */
    BYTE bFlags;	/* 08h    BYTE    control byte (see #03198 at INT 41"DISK 0") */
    WORD wPCyls;	/* 09h    WORD    number of physical cylinders */
    BYTE bPHeads;	/* 0Bh    BYTE    number of physical heads */
    WORD wLZone;	/* 0Ch    WORD    cylinder number of landing zone */
    BYTE bSects;	/* 0Eh    BYTE    number of logical sectors per track */
    BYTE bChecksum;	/* 0Fh    BYTE    checksum */
    } HDD_PARMS;

/* EDD address packet (Used by int 13H functions 42H to 47H) */
typedef struct
    {
    BYTE bSize;	    /* Packet size, in bytes. >=16. */
    BYTE bReserved1;/* Reserved. Must be 0. */
    BYTE bNumber;   /* Number of sectors to transfer */
    BYTE bReserved2;/* Reserved. Must be 0. */
    DWORD dwBuffer; /* Buffer address */
    QWORD qwLBA;    /* Logical Block Address */
/*  QWORD qwBuf64;  // Optional 64-bit buffer address */
    } EDDPACKET;

/* Hard disk parameters (Returned by int 13H function 48H) */
typedef struct
    {
    WORD wIoBase;   /* physical I/O port base address */
    WORD wAddress;  /* disk-drive control port address */
    BYTE bFlags;    /* drive flags (see #0187) */
    BYTE bProp;     /* proprietary information */
		    /* bits 7-4 reserved (0) */
		    /* bits 3-0: Phoenix proprietary (used by BIOS) */
    BYTE bIrq;	    /* IRQ (bits 3-0; bits 7-4 reserved and must be 0) */
    BYTE bSectors;  /* sector count for multi-sector transfers */
    BYTE bDma;	    /* DMA control */
		    /* bits 7-4: DMA type (0-2) as per ATA-2 specification */
		    /* bits 3-0: DMA channel */
    BYTE bPio;	    /* programmed I/O control */
		    /* bits 7-4: reserved (0) */
		    /* bits 3-0: PIO type (1-4) as per ATA-2 specification */
    WORD wOptions;  /* drive options (see #0188) */
    WORD wReserved; /* reserved (0) */
    BYTE bRevision; /* extension revision level (high nybble=major, low nybble=minor) */
		    /* (currently 10h for v1.0 and 11h for v1.1) */
    BYTE bChecksum; /* 2's complement checksum of bytes 00h-0Eh */
		    /* 8-bit sum of all bytes 00h-0Fh should equal 00h */
    } EDDPARMS;

typedef struct
    {
    WORD wSize;     /* (call) size of buffer (001Ah for v1.x, 001Eh for v2.x) */
		    /* (ret) size of returned data */
    WORD wInfo;     /* information flags (see #0184) */
    DWORD dwCyls;   /* number of physical cylinders on drive */
    DWORD dwHeads;  /* number of physical heads on drive */
    DWORD dwSects;  /* number of physical sectors per track */
    QWORD qwTotal;  /* total number of sectors on drive */
    WORD wBpS;	    /* bytes per sector */
    EDDPARMS FAR *lpedd;    /* Pointer to an EDDPARMS table */
    } HDPARMS;

#pragma pack()

#endif /* _INT13_H_ */
