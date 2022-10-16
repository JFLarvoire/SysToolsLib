/*****************************************************************************\
*                                                                             *
*   Filename:	    LDisk95.cpp						      *
*									      *
*   Description:    Windows 9X-specific logical disk access routines	      *
*                                                                             *
*   Notes:	    This code implements the OS-independant logical disk I/O  *
*		    routines for Windows 95/98/ME.			      *
*									      *
*		    OS-Independant routines are called LogDiskXxxxx().	      *
*		    Sectors are referenced by their QWORD LBA number.	      *
*									      *
*		    Physical disk accesses are done through Virtual Machine   *
*		    Manager's I/O subsystem.				      *
*									      *
*		    TO DO: IMPLEMENT THESE ROUTINES!!!			      *
*									      *
*   History:								      *
*    2002/02/07 JFL Created this file.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "LogDisk.h"

/*---------------------------------------------------------------------------*\
|   Start Definitions for the VWIN32 VxD Not defined in a Microsoft header    |
\*---------------------------------------------------------------------------*/

#define VWIN32_DIOC_DOS_IOCTL	    1
#define VWIN32_DIOC_DOS_INT25	    2
#define VWIN32_DIOC_DOS_INT26	    3
#define VWIN32_DIOC_DOS_DRIVEINFO   6

#pragma pack(push, 1)
typedef struct
{
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS;

typedef struct
{
    DWORD   diStartSector;
    WORD    diSectors;
    void*   diBuffer;
} DIOC_DISKIO;
#pragma pack(pop)

/*---------------------------------------------------------------------------*\
|    End Definitions for the VWIN32 VxD Not defined in a Microsoft header     |
\*---------------------------------------------------------------------------*/

#ifdef _DEBUG

#ifdef __cplusplus
extern "C" {
#endif // defined(__cplusplus)
extern int iVerbose;	// If TRUE, display debug messages.
#ifdef __cplusplus
}
#endif // defined(__cplusplus)

#define DEBUG_ONLY(instruction) instruction
#else
#define DEBUG_ONLY(instruction)
#endif // defined(_DEBUG)

// Structure hidden behing the LogDisk95Xxxx family handle
typedef struct
{
    HANDLE hVWin32;	// VWin32.VxD handle
    int iMode;		// 0=R/W, 1=R/O.
    BYTE bDrive;	// DOS volume number. 1=A, 2=B, 3=C, etc...
} LOGDISK, *PLOGDISK;

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    VWin32Open						      |
|									      |
|   Description:    Open VWin32.VxD.					      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    The VWin32.VxD handle, or NULL if VWin32.VxD not there.   |
|									      |
|   Notes:	    This VxD is present only under Windows 95/98/ME.	      |
|									      |
|   History:								      |
|									      |
|    2001/02  PM&SC Created routine OpenVWin32().			      |
|    2002/04/15 JFL Removed the assertion. Return NULL in case of failure.    |
|		    Renamed as VWin32Open().				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE VWin32Open()
    {
    HANDLE hVWin32;

    DEBUG_ONLY({if (iVerbose) printf("Opening VWin32.Vxd\n");});

    //Opening VWIN32 VxD
    hVWin32 = CreateFile("\\\\.\\vwin32",0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (hVWin32 == INVALID_HANDLE_VALUE) return NULL;
    return hVWin32;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    VWin32Close						      |
|									      |
|   Description:    Close VWin32.VxD.					      |
|									      |
|   Parameters:     HANDLE hVWin32	The VWin32.VxD device handle	      |
|									      |
|   Returns:	    Nothing						      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    2001/02  PM&SC Created routine CloseVWin32().			      |
|    2002/04/15 JFL Renamed as VWin32Close().				      |
*									      *
\*---------------------------------------------------------------------------*/

void VWin32Close(HANDLE hVWin32)
    {
    DEBUG_ONLY({if (iVerbose) printf("Closing VWin32.Vxd\n");});
    CloseHandle(hVWin32);
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    VWin32IoControl					      |
|									      |
|   Description:    Send DOS I/O controls to VWin32.VxD.		      |
|									      |
|   Parameters:     HANDLE hVWin32	    VWin32.VxD handle                 |
|		    DWORD nCode		    VWin32 operation code	      |
|		    DIOC_REGISTERS* pReg    I/O registers                     |
|									      |
|   Returns:	    0=Success; 1...255=DOS/BIOS Error code; Else HRESULT code.|
|									      |
|   Notes:	    See in WIN32 SDK the chapter:			      |
|		    "Using VWIN32 to Carry Out MS-DOS Functions"	      |
|		    1=DOS IOCtl; 2=DOS Int25; 3=DOS Int26; 4=BIOS Int13;      |
|		    6=DOS 7.1 Int21 Fct730x.				      |
|   History:								      |
|									      |
|    2001/02  PM&SC Created this routine				      |
|    2002/04/15 JFL Removed the assertions. Return WinError.h code if needed. |
|		    Reordered arguments with handle first.		      |
*									      *
\*---------------------------------------------------------------------------*/

int VWin32IoControl(HANDLE hVWin32, DWORD nCode, DIOC_REGISTERS* pReg)
    {
    BOOL fResult;
    DWORD nReturned; 
    
    if (!pReg) return (int)E_INVALIDARG;

    fResult = DeviceIoControl(  hVWin32, 
                                nCode,
                                pReg, sizeof(DIOC_REGISTERS), 
                                pReg, sizeof(DIOC_REGISTERS), 
                                &nReturned, 0);

    if (!fResult) return (int)E_FAIL;
    if (pReg->reg_Flags & 0x0001) return (int)(pReg->reg_EAX & 0xFFFF); // DOS error
    return (int)S_OK;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDisk95Open					      |
|									      |
|   Description:    Get a handle for a given logical disk.		      |
|									      |
|   Parameters:     char cDrive	    Logical Disk letter. A=flpy; C=hdisk...   |
|		    int iMode	    Access mode. 0=R/W; 1=R/O.		      |
|									      |
|   Returns:	    The logical disk handle, or NULL if no such logical disk. |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

HANDLE LogDisk95Open(char cDrive, int iMode)
    {
    PLOGDISK pLogDisk;
    BPB bpb;
    int iErr;

#ifdef _DEBUG
    if (iDebug)
        {
        printf("LogDisk95Open(cDrive=%c, iMode=%x)\n", cDrive, iMode);
	}
#endif // _DEBUG

    pLogDisk = (PLOGDISK)GlobalAlloc(GPTR, sizeof(LOGDISK));
    if (!pLogDisk) return NULL;
    
    pLogDisk->hVWin32 = VWin32Open();
    if (!pLogDisk->hVWin32) 
	{
	GlobalFree(pLogDisk);
	return NULL;
	}
	
    pLogDisk->bDrive = (BYTE)(toupper(cDrive) - '@'); // 1='A', 2='B', 3='C', etc...
    pLogDisk->iMode = iMode;

    /* Check volume existence by reading its BPB */
    iErr = LogDisk95GetBPB((HANDLE)pLogDisk, &bpb);
    if (iErr)
	{
	VWin32Close(pLogDisk->hVWin32);
	GlobalFree(pLogDisk);
	return NULL;
	}
    
    return (HANDLE)pLogDisk;
    }
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDisk95Close					      |
|									      |
|   Description:    Release resources reserved by LogDiskOpen.		      |
|									      |
|   Parameters:     HANDLE hDisk    logical disk handle.		      |
|									      |
|   Returns:	    Nothing.						      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

void LogDisk95Close(HANDLE hDrive)
    {
    PLOGDISK pLogDisk = (PLOGDISK)hDrive;
    
    VWin32Close(pLogDisk->hVWin32);
    GlobalFree(pLogDisk);
    return;
    }
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDisk95GetBPB					      |
|									      |
|   Description:    Get the BIOS Parameter Block of the logical disk.	      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
|		    BPB *pBpb			Buffer for the results.	      |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int LogDisk95GetBPB(HANDLE hDrive, BPB *pBpb)
    {
    char bSector[2048];
    int iErr;
    
    iErr = LogDisk95Read(hDrive, 0, 1, bSector);
    if (iErr) return iErr;
    
    CopyMemory(pBpb, bSector, sizeof(BPB));
    return 0;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDisk95Read					      |
|									      |
|   Description:    Read N sectors from the logical disk (Win9X version).     |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
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
|    2002/02/07 JFL Created this routine as an empty shell.		      |
|    2002/04/15 JFL Put meat in the shell.				      |
*									      *
\*---------------------------------------------------------------------------*/

/* These warnings are issues in Visual Studio 2015 and later, to signal WIN64
   portability isses. But this WIN95 code will never be ported to WIN64. */
#pragma warning(disable:4311) /* Ignore the "'type cast': pointer truncation" warning */
#pragma warning(disable:4302) /* Ignore the "'type cast': truncation" warning */

int LogDisk95Read(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    PLOGDISK pLogDisk = (PLOGDISK)hDrive;
    HRESULT hrError;
    DIOC_REGISTERS reg;
    DIOC_DISKIO info;

    /* First try the new DOS 7.1+ extended function supporting FAT32 */

    info.diBuffer = pBuf;
    info.diSectors = wNum;
    info.diStartSector = (DWORD)qwSector;

    ZeroMemory(&reg, sizeof(DIOC_REGISTERS));
    reg.reg_EAX = 0x7305;
    reg.reg_EBX = (DWORD)&info;
    reg.reg_ECX = 0xFFFF;
    reg.reg_EDX = pLogDisk->bDrive;
    // reg.reg_ESI = 0x0000;   // Reading unknown data

    hrError = VWin32IoControl(pLogDisk->hVWin32, VWIN32_DIOC_DOS_DRIVEINFO, &reg);
    if (!hrError) return 0;

    /* In case of failure, try the old DOS 3.31 to 7.0 function supporting FAT16 */
    info.diBuffer = pBuf;
    info.diSectors = wNum;
    info.diStartSector = (DWORD)qwSector;

    ZeroMemory(&reg, sizeof(DIOC_REGISTERS));
    reg.reg_EAX = pLogDisk->bDrive - 1; // Int 26h drive numbers are 0-based.
    reg.reg_EBX = (DWORD)&info;
    reg.reg_ECX = 0xFFFF;		// use DISKIO struct

    hrError = VWin32IoControl(pLogDisk->hVWin32, VWIN32_DIOC_DOS_INT25, &reg);
    return hrError;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    LogDiskWrite					      |
|									      |
|   Description:    Write N sectors to the logical disk.		      |
|									      |
|   Parameters:     HANDLE hDrive		Specifies the logical disk.   |
|		    QWORD qwSector              Sector LBA.                   |
|		    WORD wNum                   Number of sectors.            |
|		    void *pBuf                  Output buffer.                |
|									      |
|   Returns:	    0 = Success; Else OS-dependant error code.		      |
|									      |
|   Notes:	    This is an OS-independant API in the LogDiskXxxx family.  |
|									      |
|   History:								      |
|									      |
|    2002/02/07 JFL Created this routine				      |
|    2002/04/15 JFL Put meat in the shell.				      |
*									      *
\*---------------------------------------------------------------------------*/

int LogDisk95Write(HANDLE hDrive, QWORD qwSector, WORD wNum, void FAR *pBuf)
    {
    PLOGDISK pLogDisk = (PLOGDISK)hDrive;
    HRESULT hrError;
    DIOC_REGISTERS reg;
    DIOC_DISKIO info;

#ifdef _DEBUG
    if (iDebug)
        {
        char szqw[17];
        printf("HardDisk95Write(hDrive=%p, LBA=%s, N=%X, Buf@=%Fp)\n", 
    			hDrive, qwtox(qwSector, szqw), wNum, pBuf);
	}
    if (iReadOnly) printf("Read-only mode! Write canceled.\n");
#endif // _DEBUG
    if (iReadOnly) return 0;
    if (pLogDisk->iMode) return 3; // Int 26 Write protect error.

    /* First try the new DOS 7.1+ extended function supporting FAT32 */

    info.diBuffer = pBuf;
    info.diSectors = wNum;
    info.diStartSector = (DWORD)qwSector;

    ZeroMemory(&reg, sizeof(DIOC_REGISTERS));
    reg.reg_EAX = 0x7305;
    reg.reg_EBX = (DWORD)&info;
    reg.reg_ECX = 0xFFFF;
    reg.reg_EDX = pLogDisk->bDrive;
    reg.reg_ESI = 0x0001;   // Writing unknown data
    // reg.reg_ESI = 0x6001;   // Writing normal file data

    hrError = VWin32IoControl(pLogDisk->hVWin32, VWIN32_DIOC_DOS_DRIVEINFO, &reg);
    if (!hrError) return 0;

    /* In case of failure, try the old DOS 3.31 to 7.0 function supporting FAT16 */
    info.diBuffer = pBuf;
    info.diSectors = wNum;
    info.diStartSector = (DWORD)qwSector;

    ZeroMemory(&reg, sizeof(DIOC_REGISTERS));
    reg.reg_EAX = pLogDisk->bDrive - 1; // Int 26h drive numbers are 0-based.
    reg.reg_EBX = (DWORD)&info;
    reg.reg_ECX = 0xFFFF;		// use DISKIO struct

    hrError = VWin32IoControl(pLogDisk->hVWin32, VWIN32_DIOC_DOS_INT26, &reg);
    return hrError;
    }

