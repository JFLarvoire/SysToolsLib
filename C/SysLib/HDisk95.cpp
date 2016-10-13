/*****************************************************************************\
*                                                                             *
*   Filename:	    HDisk95.cpp						      *
*									      *
*   Description:    Windows 9X-specific hard disk access routines	      *
*                                                                             *
*   Notes:	    This code implements the OS-independant hard disk I/O     *
*		    routines for Windows 95/98/ME.			      *
*									      *
*		    OS-Independant routines are called HardDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    Physical disk accesses are done through Virtual Machine   *
*		    Manager's I/O subsystem.				      *
*									      *
*   History:								      *
*    2000/09/21 JFL Created file harddisk.cpp.				      *
*    2001/09/05 JFL Moved all Win9X-specific routines into file HDisk95.cpp.  *
*    2001/10/01 JFL Do not include C/CPP files, only their .H headers.        *
*    2002/04/15 JFL Added test of iReadOnly.				      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "HardDisk.h"

typedef struct tagDISK95
    {
    int iDrive;
    int iMode;
    HANDLE hVWin32;
    } DISK95, *HDISK95;

#include "ring0.h"	// General Ring0 access mechanism
#include "R0IOS.H"	// Win9X I/O Supervisor access routines.

/* Win9X VWIN32 API */
#define VWIN32_DIOC_DOS_IOCTL	  1	// Performs the specified MS-DOS device I/O control function (Interrupt 21h Function 4400h through 4411h)
#define VWIN32_DIOC_DOS_INT25	  2	// Performs the Absolute Disk Read command (Interrupt 25h)
#define VWIN32_DIOC_DOS_INT26	  3	// Performs the Absolute Disk Write command (Interrupt 26h)
#define VWIN32_DIOC_DOS_INT13 	  4	// Performs Interrupt 13h commands  
#define VWIN32_DIOC_DOS_DRIVEINFO 6	// Performs Interrupt 21h Function 730X commands. This value is supported in Windows 95 OEM Service Release 2 and later.

typedef struct _DIOC_REGISTERS {
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDisk95Open					      |
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
|    2001-02-26 JFL Created this routine				      |
|    2016-04-14 JFL Added debug code.					      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE HardDisk95Open(int iDrive, int iMode)
    {
    HANDLE h = NULL;

#ifdef _DEBUG
    if (iDebug)
        {
        printf("HardDisk95Open(iDrive=%d, iMode=%x)\n", iDrive, iMode);
	}
#endif // _DEBUG

    iDrive += 0x80; // Convert to int 13 drive number.

    if (R0IosFindInt13Drive(iDrive)) {
      h = (HANDLE)(DWORD)(iDrive + ((WORD)iMode << 15));
    } // Else return NULL = No such hard disk.

#ifdef _DEBUG
      if (iDebug) {
	printf("  return %04X\n", h);
      }
#endif // _DEBUG
    return h;
    }
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDisk95Close					      |
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
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4100) /* Avoid warnings "unreferenced formal parameter" */

void HardDisk95Close(HANDLE hDrive)
    {
    return;
    }
    
#pragma warning(default:4100)

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDisk95GetGeometry				      |
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
|		    Includes a workaround for an error in the total number of |
|		    sectors reported by IOS. This workaround takes about 3s.  |
|		    if pHdGeometry->dwXlatCyls == 0xFEEDBACC then the	      |
|		    workaround is skipped. This prevents the timeout, but at  |
|		    the cost of an underestimated number of sectors.	      |
|									      |
|   History:								      |
|									      |
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDisk95GetGeometry(HANDLE hDrive, HDGEOMETRY *pHdGeometry)
    {
    // DWORD dwReentrancyCount;
    _BlockDev_Device_Descriptor *pBDD;
    BYTE bDrive = (BYTE)hDrive;
    int iCheckSectors = TRUE;

    if (pHdGeometry->dwXlatCyls == 0xFEEDBACC) iCheckSectors = FALSE;

#ifdef _DEBUG
    if (iDebug)
        {
        printf("HardDisk95GetGeometry(hDrive=%lX, Buf@=%Fp)\n", 
    			hDrive, pHdGeometry);
	}
#endif // _DEBUG

    pBDD = R0IosFindInt13Drive(bDrive);
    if (!pBDD) return BDS_No_Device;

    pHdGeometry->qwSectors = *(QWORD *)(pBDD->BDD_Max_Sector) + 1;
    pHdGeometry->wSectorSize = (WORD)(pBDD->BDD_Sector_Size);
    pHdGeometry->dwCyls = pBDD->BDD_Num_Cylinders;
    pHdGeometry->dwHeads = pBDD->BDD_Num_Heads;
    pHdGeometry->dwSects = pBDD->BDD_Num_Sec_Per_Track;
    // Assume no translation.
    pHdGeometry->dwXlatCyls = pBDD->BDD_Num_Cylinders;
    pHdGeometry->dwXlatHeads = pBDD->BDD_Num_Heads;
    pHdGeometry->dwXlatSects = pBDD->BDD_Num_Sec_Per_Track;

    /*
        Work around a problem in Windows 98 SE: 
        The BDD_Max_Sector field contains the Cyl*Head*Sect product, which is
	limited to 8GB.
    */
#if 0
    /*
    	First workaround found: Trial and error.
	Guarantied to work in every case, but takes 3 seconds due to head movements.
    */
	{
	void *pBuf;
	QWORD qwMin;
	QWORD qwMax;

	if (!iCheckSectors) return 0;   // Avoid timeout if magic value passed-in.

	pBuf = malloc(pHdGeometry->wSectorSize);
	if (!pBuf) return 0;

	// Phase 1: Find an upper limit for the actual number
	qwMin = pHdGeometry->qwSectors; // Number of sectors
	for (qwMax = qwMin; !HardDisk95Read(hDrive, qwMax, 1, pBuf); qwMax *= 2) ; // Read further and further away.
	qwMin -= 1;	    // Index of the last surely readable sector.
	// Phase 2: Do a dichotomic search for the actual end
	while (TRUE)
	    {
	    QWORD qw;
	    qw = qwMin;
	    qw += qwMax;
	    qw /= 2;		// qw = middle of segment
	    if (qw == qwMin) break; // if max == min+1, we're finished.
	    if (!HardDisk95Read(hDrive, qw, 1, pBuf))   // If no error
		qwMin = qw;		// Then this sector is readable. Move up lower end.
	    else					    // Else error
		qwMax = qw;		// This sector is not readble. Move down upper end.
	    }
	pHdGeometry->qwSectors = qwMax; // Update the actual number of sectors.

	free(pBuf);
	}
#else
    /* 
	Second workaround found: Get the IDE ID sector in IOS DCB for the drive.
	Works only for IDE drives.
    */

	{
	WORD *pw;
	// The clean method would be to scan all DCBs using R0IosService(). 
	// But the initialization of R0IosService is not so clean.
	// Also experience shows our DCB includes the BDD we already know of in the end.
	// So simply use pointer arithmetics to get the address of the DCB inside which the BDD is embedded.
	PIOSDCB pDCB = (PIOSDCB)((char *)pBDD - (sizeof(IOSDCB) - sizeof(DCB_BLOCKDEV)));

	if (pDCB->DCB_bus_type == DCB_BUS_ESDI)	// For IDE and ESDI only...
	    {
	    // Get the physical geometry from the DCB while we have it.
	    // ~~JFL 2001/10/01 These fields contain 0 for SCSI devices under WinME. 
	    //			This is why I added a test of the bus type above.
	    pHdGeometry->dwCyls = pDCB->DCB_actual_cyl_cnt;
	    pHdGeometry->dwHeads = pDCB->DCB_actual_head_cnt;
	    pHdGeometry->dwSects = pDCB->DCB_actual_spt;

	    // Now decode the IDE ID sector contents.
	    // To do: Add support for 48-bit sector addressing (ATA 4 or ATA 5 I think)
	    pw = (WORD *)(pDCB->DCB_cmn.DCB_pEid);
	    if (pw && (pw[53] & 1) && (pw[54] == pHdGeometry->dwCyls) && (pw[55] == pHdGeometry->dwHeads) && (pw[56] == pHdGeometry->dwSects))
		{
		pHdGeometry->qwSectors = *(DWORD *)(pw+60);
		}
	    }
	}
#endif
    

    /* End of problem workaround */

    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    HardDisk95Read					      |
|									      |
|   Description:    Read N sectors from the physical disk (Win9X version).    |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the hard disk.      |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    Uses Win32 Ring 0 access routines.			      |
|									      |
|   History:								      |
|									      |
|    2001/05/30 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDisk95Read(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    // DWORD dwReentrancyCount;
    _BlockDev_Device_Descriptor *pBDD;
    _BlockDev_Command_Block *pBCB;
    char *pR0Buf;
    int iErr;
    BYTE bDrive = (BYTE)hDrive;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("HardDisk95Read(hDrive=%lX, LBA=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
#endif // _DEBUG

    // Protect VMM call section.
    // ~~jfl 2001/06/01 Experiments without this seem to work just as well.
    // dwReentrancyCount = R0BeginReentrantExecution();

    pBDD = R0IosFindInt13Drive(bDrive);
    if (!pBDD) return BDS_No_Device;

    // Allocate space for N sectors in the non-pageable system heap.
    pR0Buf = (char *)R0HeapAllocate(wNum * pBDD->BDD_Sector_Size, 0);
    if (!pR0Buf) return BDS_Memory_Error;

    // Create the I/O command request. Must be in the non-pageable system heap.
    pBCB = (_BlockDev_Command_Block *)R0HeapAllocate(sizeof(_BlockDev_Command_Block), HEAPZEROINIT);
    if (!pBCB)
        {
        R0HeapFree(pR0Buf, 0);
        return BDS_Memory_Error;
        }
    pBCB->BD_CB_Command = BDC_Read;
    pBCB->BD_CB_Flags = BDCF_Bypass_Voltrk | BDCF_High_Priority;
    *(QWORD *)&(pBCB->BD_CB_Sector[0]) = qwSector;
    pBCB->BD_CB_Count = wNum;
    pBCB->BD_CB_Buffer_Ptr = (DWORD)pR0Buf;
    pBCB->BD_CD_SGD_Lin_Phys = (DWORD)(pBCB+1);
    pBCB->BD_CB_Num_SGDs = 0;
    pBCB->BD_CB_Vol_Designtr = pBDD->BDD_Int_13h_Number;

    // Do the I/O.
    iErr = R0IosSendCommand(pBCB, pBDD);

    // Get the result back in Win32 buffer.
    memcpy(pBuf, pR0Buf, wNum * pBDD->BDD_Sector_Size);
    
    // Cleanup allocated blocks.
    R0HeapFree(pR0Buf, 0);
    R0HeapFree(pBCB, 0);

    // End protected section.
    // R0EndReentrantExecution(dwReentrancyCount);

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
|    2001/02/26 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int HardDisk95Write(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    // DWORD dwReentrancyCount;
    _BlockDev_Device_Descriptor *pBDD;
    _BlockDev_Command_Block *pBCB;
    char *pR0Buf;
    int iErr;
    BYTE bDrive = (BYTE)hDrive;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("HardDisk95Write(hDrive=%lX, LBA=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
    if (iReadOnly) printf("Read-only! Write canceled.\n");
#endif // _DEBUG
    if (iReadOnly) return 0;

    if ((WORD)hDrive & 0x8000) return BDS_Write_Protect; // Write protect error.

    // Protect VMM call section.
    // ~~jfl 2001/06/01 Experiments without this seem to work just as well.
    // dwReentrancyCount = R0BeginReentrantExecution();

    pBDD = R0IosFindInt13Drive(bDrive);
    if (!pBDD) return BDS_No_Device;

    // Allocate space for N sectors in the non-pageable system heap.
    pR0Buf = (char *)R0HeapAllocate(wNum * pBDD->BDD_Sector_Size, 0);
    if (!pR0Buf) return BDS_Memory_Error;

    // Copy the data from the Win32 buffer to the Ring 0 buffer.
    memcpy(pR0Buf, pBuf, wNum * pBDD->BDD_Sector_Size);
    
    // Create the I/O command request. Must be in the non-pageable system heap.
    pBCB = (_BlockDev_Command_Block *)R0HeapAllocate(sizeof(_BlockDev_Command_Block), HEAPZEROINIT);
    if (!pBCB)
        {
        R0HeapFree(pR0Buf, 0);
        return BDS_Memory_Error;
        }
    pBCB->BD_CB_Command = BDC_Write;
    pBCB->BD_CB_Flags = BDCF_Bypass_Voltrk | BDCF_High_Priority;
    *(QWORD *)&(pBCB->BD_CB_Sector[0]) = qwSector;
    pBCB->BD_CB_Count = wNum;
    pBCB->BD_CB_Buffer_Ptr = (DWORD)pR0Buf;
    pBCB->BD_CD_SGD_Lin_Phys = (DWORD)(pBCB+1);
    pBCB->BD_CB_Num_SGDs = 0;
    pBCB->BD_CB_Vol_Designtr = pBDD->BDD_Int_13h_Number;

    // Do the I/O.
    iErr = R0IosSendCommand(pBCB, pBDD);

    // Cleanup allocated blocks.
    R0HeapFree(pR0Buf, 0);
    R0HeapFree(pBCB, 0);

    // End protected section.
    // R0EndReentrantExecution(dwReentrancyCount);

    return iErr;
    }

