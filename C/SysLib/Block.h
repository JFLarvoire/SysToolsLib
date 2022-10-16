/*****************************************************************************\
*                                                                             *
*   Filename:	    Block.h						      *
*									      *
*   Description:    OS-independant block device access routines		      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2001-02-20 JFL Created this file.					      *
*    2002-02-07 JFL Added logical volumes.				      *
*    2002-07-05 JFL Restore the default packing size in the end.	      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*    2017-07-15 JFL Added the floppy and CD disk types.   		      *
*		    Removed BLOCK_TYPE_* constants. Use BT_* enums instead.   *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "SysLib.h"		/* SysLib Library core definitions */

/*****************************************************************************\
*                                                                             *
*                          Basic type definitions                             *
*                                                                             *
\*****************************************************************************/

/* Redefine these basic types. Compatible with Windows and EFI. */
#include "qword.h"		/* Defines BYTE, WORD, DWORD, QWORD, etc */

typedef void * HANDLE;

/******************** Legacy partition table structures **********************/

#pragma pack(1)   /* IMPORTANT: All following structures are packed! */

#define SECTORSIZE 512		/* Standard disk sector size */

/* Processor independant dereference of handle type */
#define HandleToPtr(h) ((void *)(h))

/* Block device access */
typedef struct {
  int iType;		/* 0=File; 1=Hard Disk; 2=Logical Volume; 3=Floppy; 4=CD. */
  int iBlockSize;	/* Block size, in bytes. */
  QWORD qwBlocks;	/* Total number of blocks in device */
  HANDLE h;
} SBLOCKDEVICE;

HANDLE BlockOpen(char *pszName, char *pszMode);
void BlockClose(HANDLE hDevice);
#define BlockPtr(hDevice) ((SBLOCKDEVICE *)HandleToPtr(hDevice))
QWORD BlockCount(HANDLE hDevice);
int BlockSize(HANDLE hDevice);
int BlockType(HANDLE hDevice);
enum {
  BT_FILE,		/* 0 = File */
  BT_HARDDISK,		/* 1 = Hard disk */
  BT_LOGICALVOLUME,	/* 2 = Logical volume */
  BT_FLOPPYDISK,	/* 3 = Floppy disk */
  BT_COMPACTDISK,	/* 4 = CD/DVD/BlueRay */
};
int BlockRead(HANDLE hDevice, QWORD qwSector, WORD wNum, void FAR *pBuf);
int BlockWrite(HANDLE hDevice, QWORD qwSector, WORD wNum, void FAR *pBuf);
char *BlockIndexName(HANDLE hBlock);

#pragma pack()	/* Restore the default packing size */

#endif /* ifndef _BLOCK_H_ */
