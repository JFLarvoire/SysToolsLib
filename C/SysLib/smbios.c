/*****************************************************************************\
*									      *
*   File name:	    smbios.c						      *
*									      *
*   Description:    SMBIOS tables management routines			      *
*									      *
*   Notes:	    This file is designed to be simply reusable:	      *
*		    Just include it in other programs that need to access     *
*		    SMBIOS tables.					      *
*		    							      *
*		    Specification up to version 2.0 were called DMI BIOS.     *
*		    Specification version 2.1 and above are called SMBIOS.    *
*		    DMI stands for Desktop Management Interface.	      *
*		    SMBIOS stands for System Management BIOS.		      *
*		    							      *
*		    TO DO: Change the GetXxxx routines to return FAR pointers *
*		    to the original tables, instead of a copy.		      *
*		    This would make the code faster and smaller.	      *
*		    							      *
*   History:								      *
*    1996-05-20 JFL Initial implementation of dmidump.c.		      *
*    1997-06-13 JFL Added SMBIOS 2.0 support.	 			      *
*    1998-10-30 JFL Fixed bug in SMBIOS 2.0 table read. 		      *
*    1998-11-03 JFL Added detection of SMBIOS 2.1 table-based support.	      *
*    1998-11-04 JFL Added detection of HP extensions to SMBIOS 2.0 table acc. *
*    1999-07-08 JFL Restructured to make the code more easily reusable.       *
*		    Created routines SmBiosInit() & SmBiosGetStructByHandle().*
*    1999-07-12 JFL Split this file out of dmidump.c.			      *
*		    Added routine SmBiosGetStructByType(); 		      *
*    2001-01-15 JFL Compile nothing if used in the WIN32 environment.	      *
*    2001-12-20 JFL Changed back constant name from $SMBIOS to $DMI.	      *
*    2002-07-15 JFL Added routine Dmi2GetString().			      *
*    2010-06-14 JFL Adapted to WIN32.                                         *
*    2016-03-28 JFL Added support for use of 32-bits tables in WIN64.         *
*		    Added preliminary support for SMBIOS 3.0 64-bits tables.  *
*    2016-03-29 JFL Added routine SmBiosGetStructureExtendedSize.	      *
*    2016-04-27 JFL Added alternate WIN32 method using GetSystemFirmwareTable.*
*		    Added functions SmBiosOpen and SmBiosClose.		      *
*		    							      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "smbios.h"
#include "debugm.h"

#include <memory.h>
#include <string.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>

#define FALSE 0
#define TRUE 1

#ifndef NULL
  #ifdef __cplusplus
    #define NULL 0
  #else
    #define NULL ((void *)0)
  #endif
#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 applic. */

#ifndef WINVER
#define WINVER 0x0400
#endif /* WINVER */

/* if defined, the following flags inhibit the definitions in windows.h */
#define NOGDICAPMASKS     /* CC_*, LC_*, PC_*, CP_*, TC_*, RC_ */
#define NOVIRTUALKEYCODES /* VK_* */
#define NOWINMESSAGES     /* WM_*, EM_*, LB_*, CB_* */
#define NOWINSTYLES       /* WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_* */
#define NOSYSMETRICS      /* SM_* */
#define NOMENUS           /* MF_* */
#define NOICONS           /* IDI_* */
#define NOKEYSTATES       /* MK_* */
#define NOSYSCOMMANDS     /* SC_* */
#define NORASTEROPS       /* Binary and Tertiary raster ops */
#define NOSHOWWINDOW      /* SW_* */
/* #define OEMRESOURCE    /* OEM Resource values */
#define NOATOM            /* Atom Manager routines */
#define NOCLIPBOARD       /* Clipboard routines */
#define NOCOLOR           /* Screen colors */
#define NOCTLMGR          /* Control and Dialog routines */
#define NODRAWTEXT        /* DrawText() and DT_* */
#define NOGDI             /* All GDI defines and routines */
#define NOKERNEL          /* All KERNEL defines and routines */
#define NOUSER            /* All USER defines and routines */
#define NONLS             /* All NLS defines and routines */
#define NOMB              /* MB_* and MessageBox() */
#define NOMEMMGR          /* GMEM_*, LMEM_*, GHND, LHND, associated routines */
#define NOMETAFILE        /* typedef METAFILEPICT */
#define NOMINMAX          /* Macros min(a,b) and max(a,b) */
#define NOMSG             /* typedef MSG and associated routines */
#define NOOPENFILE        /* OpenFile(), OemToAnsi, AnsiToOem, and OF_* */
#define NOSCROLL          /* SB_* and scrolling routines */
#define NOSERVICE         /* All Service Controller routines, SERVICE_ equates, etc */
#define NOSOUND           /* Sound driver routines */
#define NOTEXTMETRIC      /* typedef TEXTMETRIC and associated routines */
#define NOWH              /* SetWindowsHook and WH_* */
#define NOWINOFFSETS      /* GWL_*, GCL_*, associated routines */
#define NOCOMM            /* COMM driver routines */
#define NOKANJI           /* Kanji support stuff */
#define NOHELP            /* Help engine interface */
#define NOPROFILER        /* Profiler interface */
#define NODEFERWINDOWPOS  /* DeferWindowPos routines */
#define NOMCX             /* Modem Configuration Extensions */
#include <windows.h>

#define _fmemcpy memcpy
#define _fmemcmp memcmp

#endif /* defined(_WIN32) */

/********************** End of OS-specific definitions ***********************/

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    FindHeader						      |
|									      |
|   Description:    Find the SMBIOS or PnP descriptor structure in the BIOS.  |
|									      |
|   Parameters:     DWORD dwSignature					      |
|									      |
|   Returns:	    far pointer to the header, or NULL if not found.	      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1997-06-13 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_MSDOS) && !defined(_BIOS)

LPVOID FindHeader(DWORD dwExpected)
    {
    LPVOID lp;
    int i;

    DEBUG_PRINTF(("FindHeader('%4.4s')\n", &dwExpected));

    for (lp = (LPVOID)0xF0000000L; lp < (LPVOID)0xF000FFF0L; *(DWORD *)&lp += 0x10)
	{
	LPSMBIOSHEADER lpSmBiosHdr = (LPSMBIOSHEADER)lp;

	if (lpSmBiosHdr->dwSignature == dwExpected)
	    {
	    LPBYTE lpb;
	    BYTE b;

	    /* Check the header checksum */
	    lpb = (LPBYTE)lp;
	    DEBUG_PRINTF(("Found signature at %lp\n", lp));
#pragma warning(disable:4135) /* Avoid warnings "conversion between different integral types" */ /* MSVC 1.5 bug: warning triggered by lpb[i] */
	    for (i=b=0; i<lpSmBiosHdr->bHeaderLength; i++) b += lpb[i];
#pragma warning(default:4135)
	    if (b) continue;	/* Invalid checksum */

	    return lp;		/* Found */
	    }
	}

    return NULL;
    }

#endif /* defined(_MSDOS) && !defined(_BIOS) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosInit						      |
|									      |
|   Description:    Find the SMBIOS API					      |
|									      |
|   Parameters:     SMBIOS21HEADER *pSmBiosFound  Pointer to an output buffer |
|		    DWORD dwPreferred		  Preferred access method     |
|						  0 = no preference	      |
|									      |
|   Returns:	    The SMBIOS version. AH = Major version, AL = Minor version|
|		    0 = SMBIOS not found. 0x201 = SMBIOS 2.1. Etc.	      |
|									      |
|		    Fills the *pSmBiosFound structure with information about  |
|		    the SMBIOS tables found, if any.			      |
|									      |
|   Notes:	    SMBIOS 2.1 headers contain more information than previous |
|		    versions. This is why this program uses the 2.1 header    |
|		    in all cases.					      |
|									      |
|		    The pSmBiosFound argument is optional. If it is NULL, no  |
|		    information will be returned beyond the SMBIOS version.   |
|		    The information returned is necessary and sufficient for  |
|		    routine SmBiosGetStructByHandle() to enumerate all tables,|
|		    whatever the SMBIOS version is.			      |
|									      |
|   History:								      |
|    1999-07-08 JFL Created this routine, reusing code from DMIDUMP.C.        |
|    2010-06-14 JFL Added a WIN32 version using the table in the registry.    |
|    2016-04-27 JFL Added alternate WIN32 method using GetSystemFirmwareTable.|
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS

#pragma warning(disable:4706) /* Avoid warnings "assignment within conditional expression" */

int SmBiosInit(SMBIOS21HEADER *pSmBiosFound, DWORD dwPreferred)
    {
    LPBYTE lpb;
    int iErr;
    BYTE bRevision;
    WORD wNumStructures;
    DWORD dwBase;
    WORD wTotal;
    int iSmBiosVersion = 0;
    LPVOID lp;
    int i;
    WORD wMax;
    WORD wLen;

    /* First look for SMBIOS 2.1+ 32-bits-RAM-style tables */

    if ((!dwPreferred) || (dwPreferred == _SM_))
	{
	LPSMBIOS21HEADER lpSmBios21Hdr;

	if ((lpSmBios21Hdr = (LPSMBIOS21HEADER)FindHeader(_SM_)))   /* "_SM_" */
	    {
	    if (pSmBiosFound) _fmemcpy(pSmBiosFound, lpSmBios21Hdr, sizeof(SMBIOS21HEADER));

	    BYTE1(iSmBiosVersion) = pSmBiosFound->bMajorVersion;
	    BYTE0(iSmBiosVersion) = pSmBiosFound->bMinorVersion;

	    return iSmBiosVersion;
	    }

	/* SMBIOS 2.1+ 32-bits-RAM-style calls not supported. Try something else. */
	}

    /* Then look for SMBIOS 2.0 HP-proprietary extensions */

    if ((!dwPreferred) || (dwPreferred == _DMI))
	{
	LPSMBIOS20HPHEADER lpSmBios20Hdr = NULL;

	for (lp = (LPVOID)0xF0000000L; lp < (LPVOID)0xF000FFF0L; *(DWORD *)&lp += 1)
	    {
	    lpSmBios20Hdr = (LPSMBIOS20HPHEADER)lp;

	    if (!_fmemcmp(&lpSmBios20Hdr->bSignature, "_DMI20_NT_", sizeof(lpSmBios20Hdr->bSignature)))
		{
		break;		/* Found */
		}

	    lpSmBios20Hdr = NULL;
	    }
	if (lpSmBios20Hdr)
	    {
	    bRevision = lpSmBios20Hdr->bVersion;

	    BYTE1(iSmBiosVersion) = (BYTE)(bRevision >> 4);
	    BYTE0(iSmBiosVersion) = (BYTE)(bRevision & 0x0F);

	    wTotal = wLen = wMax = 0;
	    for (i=0; lpSmBios20Hdr->table[i].wOffset; i++)
		{
		wLen = lpSmBios20Hdr->table[i].wSize;
		wTotal += wLen + 1;
		if (wLen > wMax) wMax = wLen;
		}

	    if (pSmBiosFound)
		{
		memset(pSmBiosFound, 0, sizeof(SMBIOS21HEADER));
		pSmBiosFound->dwSignature = _DMI;	 /* "_DMI" */
		pSmBiosFound->bHeaderLength = sizeof(SMBIOS21HEADER);
		pSmBiosFound->bMajorVersion = BYTE1(iSmBiosVersion);
		pSmBiosFound->bMinorVersion = BYTE0(iSmBiosVersion);
		pSmBiosFound->wMaxStructSize = wMax;
		pSmBiosFound->wTotalSize = wTotal;
		pSmBiosFound->dwPhysicalAddress = dwBase;
		pSmBiosFound->wNumStructures = (WORD)i;
		pSmBiosFound->bcdRevision = bRevision;
		/* Store HP 2.0 header address in reserved area */
		*(LPSMBIOS20HPHEADER *)(pSmBiosFound->bFormatted) = lpSmBios20Hdr;
		}

	    return iSmBiosVersion;
	    }

	/* SMBIOS 2.0 HP-proprietary tables not supported. Try something else. */
	}

    /* Then look for SMBIOS 2.0+ 16-bits-PnP-style tables */

    if ((!dwPreferred) || (dwPreferred == $PnP))
	{
	LPPNPHEADER lpPnpHdr;
	LPPNPBIOS lpPnpBios;

	if ((lpPnpHdr = (LPPNPHEADER)FindHeader($PnP)))    /* "$PnP" */
	    {
	    lpPnpBios = lpPnpHdr->lpRmEntry;
	    DEBUG_PRINTF(("Found $PnP API at %lp\n", lpPnpBios));
	    /* Get SMBIOS Information */
	    iErr = lpPnpBios(0x50, (LPVOID)&bRevision, (LPVOID)&wNumStructures,
			     (LPVOID)&wMax, (LPVOID)&dwBase,
			     (LPVOID)&wTotal, lpPnpHdr->wRmDS);
	    if (!iErr)
		{
		if (!bRevision) bRevision = (BYTE)0x20; /* Work around VMWare VMs bug */
		BYTE1(iSmBiosVersion) = (BYTE)(bRevision >> 4);
		BYTE0(iSmBiosVersion) = (BYTE)(bRevision & 0x0F);

		if (pSmBiosFound)
		    {
		    memset(pSmBiosFound, 0, sizeof(SMBIOS21HEADER));
		    pSmBiosFound->dwSignature = $PnP;   /* "$PnP" */
		    pSmBiosFound->bHeaderLength = sizeof(SMBIOS21HEADER);
		    pSmBiosFound->bMajorVersion = BYTE1(iSmBiosVersion);
		    pSmBiosFound->bMinorVersion = BYTE0(iSmBiosVersion);
		    pSmBiosFound->wMaxStructSize = wMax;
		    pSmBiosFound->wTotalSize = wTotal;
		    pSmBiosFound->dwPhysicalAddress = dwBase;
		    pSmBiosFound->wNumStructures = wNumStructures;
		    pSmBiosFound->bcdRevision = bRevision;
		    /* Store PnP header address in reserved area */
		    *(LPPNPHEADER *)(pSmBiosFound->bFormatted) = lpPnpHdr;
		    }

		return iSmBiosVersion; /* Use it if found */
		}

	    /* SMBIOS 2.0+ PnP-style calls not supported. Try something else. */
	    }
	}

    /* Else look for SMBIOS 1.0 tables */

    if ((!dwPreferred) || (dwPreferred == $DMI))
	{
	LPSMBIOSHEADER lpSmBiosHdr;

	if ((lpSmBiosHdr = (LPSMBIOSHEADER)FindHeader(0x494D4424)))    /* "$DMI" */
	    {
	    /* OK, it's valid. We've found the SMBIOS 1.0 header */
	    /* printf("SMBIOS 1.0 Header at %lp.\n", lpSmBiosHdr); */

	    bRevision = lpSmBiosHdr->bVersion;
	    BYTE1(iSmBiosVersion) = (BYTE)(bRevision >> 4);
	    BYTE0(iSmBiosVersion) = (BYTE)(bRevision & 0x0F);

	    /* Scan the SMBIOS structures to get the max size */
	    wNumStructures = 0;
	    wMax = 0;
	    for (lpb = (LPBYTE)(lpSmBiosHdr->lpStructures);
		 lpb < ((LPBYTE)(lpSmBiosHdr->lpStructures)
				 + lpSmBiosHdr->wStructuresLength);
		 lpb += lpb[1])
		{
		if (!lpb[1]) break;	/* Prevent infinite loops */
		wNumStructures += 1;
		/* Structure length = lpb[1] */
		if (wMax < lpb[1]) wMax = lpb[1];
		if (!lpb[0]) break;	/* Type 0 is listed last */
		}

	    if (pSmBiosFound)
		{
		memset(pSmBiosFound, 0, sizeof(SMBIOS21HEADER));
		pSmBiosFound->dwSignature = $DMI;	  /* "$DMI" */
		pSmBiosFound->bHeaderLength = sizeof(SMBIOS21HEADER);
		pSmBiosFound->bMajorVersion = BYTE1(iSmBiosVersion);
		pSmBiosFound->bMinorVersion = BYTE0(iSmBiosVersion);
		pSmBiosFound->wMaxStructSize = wMax;
		pSmBiosFound->wTotalSize = lpSmBiosHdr->wStructuresLength;
		lp = lpSmBiosHdr->lpStructures;
		pSmBiosFound->dwPhysicalAddress = ((DWORD)WORD1(lp) << 4)
					       + WORD0(lp);
		pSmBiosFound->wNumStructures = wNumStructures;
		pSmBiosFound->bcdRevision = bRevision;
		/* Store SMBIOS 1.0 pointer in reserved area */
		*(LPSMBIOSHEADER *)(pSmBiosFound->bFormatted) = lpSmBiosHdr;
		}

	    return iSmBiosVersion; /* Use it if found */
	    }

	/* SMBIOS BIOS 1.0 not supported. Nothing left to try. */
	}

    return 0;
    }
#pragma warning(default:4706)

#endif /* defined(_MSDOS) */

#ifdef _WIN32

/* Function GetSystemFirmwareTable() is only available in XP SP2 and later */
typedef UINT (WINAPI *PGETSYSTEMFIRMWARETABLE)(
  DWORD FirmwareTableProviderSignature,
  DWORD FirmwareTableID,
  PVOID pFirmwareTableBuffer,
  DWORD BufferSize
);

int SmBiosInit(SMBIOS21HEADER *pSmBiosFound, DWORD dwPreferred) {
  DWORD dwSize;
  BYTE *lpb;
  int iSmbiosVersion = 0;
  BYTE *pSmBiosBuf = NULL;	/* Buffer for SMBIOS tables copy */
  BYTE *pSmBiosData = NULL;	/* Address of the first SMBIOS table */
  PRAWSMBIOSDATA pRawSMBIOSData = NULL;
  BYTE bMethod = 0;		/* Index of the access method actually used */

  /* First try using function GetSystemFirmwareTable() if it is available */
  if ((!dwPreferred) || (dwPreferred == 1)) {
    PGETSYSTEMFIRMWARETABLE pGetSystemFirmwareTable;

    pGetSystemFirmwareTable = (PGETSYSTEMFIRMWARETABLE)GetProcAddress(
      GetModuleHandle("kernel32.dll"), "GetSystemFirmwareTable"
    );
    if (pGetSystemFirmwareTable) { /* The function is available */
      dwSize = pGetSystemFirmwareTable('RSMB', 0, NULL, 0);
      pSmBiosBuf = malloc(dwSize);
      if (pSmBiosBuf) {
	DWORD dwSize2 = pGetSystemFirmwareTable('RSMB', 0, pSmBiosBuf, dwSize);
	if (dwSize2 == dwSize) {
	  pRawSMBIOSData = (PRAWSMBIOSDATA)pSmBiosBuf;
	  bMethod = 1;
	  goto found_win32_raw_header;
	}
      } else {
win32_not_enough_memory:
#if _DEBUG
	printf("Error. Not enough memory for SMBIOS tables copy. %ld bytes needed.\n", dwSize);
#endif
	return 0;
      }
    }
  }

  /* Then try using the mssmbios.sys copy of the SMBIOS table in the registry.
     Documented as unreliable, but available in the registry for XP ... Win10.
     Note: Some fields, like the system UUID field, are cleared for dubious "Security Reasons". */
  if ((!dwPreferred) || (dwPreferred == 2)) {
    HKEY hKey;
    LONG lError;
    DWORD dwType = REG_BINARY;
    char *pszKey = "SYSTEM\\CurrentControlSet\\services\\mssmbios\\Data";
#if _DEBUG
    char *pszAction = "open key";
#endif
    lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0L, KEY_READ, &hKey);
    if (lError == ERROR_SUCCESS) {
#if _DEBUG
      pszAction = "query value size";
#endif
      dwSize = sizeof(DWORD);
      lError = RegQueryValueEx(hKey, "SMBiosData", NULL, &dwType, NULL, &dwSize);
      if (lError == ERROR_SUCCESS) {
	pSmBiosBuf = malloc(dwSize);
	if (!pSmBiosBuf) goto win32_not_enough_memory;
#if _DEBUG
	pszAction = "read value";
#endif
	lError = RegQueryValueEx(hKey, "SMBiosData", NULL, &dwType, pSmBiosBuf, &dwSize);
	RegCloseKey(hKey);
	if (lError == ERROR_SUCCESS) {
	  pRawSMBIOSData = (PRAWSMBIOSDATA)pSmBiosBuf;
	  bMethod = 2;
	  goto found_win32_raw_header;
	}
      }
    }
    if (lError != ERROR_SUCCESS) {
#if _DEBUG
      printf("Error. Failed to %s HKLM\\%s.\n", pszAction, pszKey);
#endif
      return 0;                
    }
  }

  /* TO DO: Add a method using \Device\PhysicalMemory in XP SP0 and older */
  /*        https://msdn.microsoft.com/en-us/library/ms724259(v=vs.85).aspx */

  /* TO DO: Add a method using Ring0 physical memory access for Windows 95 */

  /* All available methods failed */
#if _DEBUG
  printf("Error. GetSystemFirmwareTable() failed.\n");
#endif
  return 0;

found_win32_raw_header:
  /* Fill up the simulated SMBIOS 2.1 header */
  pSmBiosData = pRawSMBIOSData->SMBIOSTableData;
  /* Set the return value to the SMBIOS version */
  BYTE0(iSmbiosVersion) = pRawSMBIOSData->SMBIOSMinorVersion;
  BYTE1(iSmbiosVersion) = pRawSMBIOSData->SMBIOSMajorVersion;
  /* Build an SMBIOS 2.1 header */
  pSmBiosFound->dwSignature = _SM_; /* "_SM_" */
  pSmBiosFound->bHeaderLength = sizeof(SMBIOS21HEADER);
  pSmBiosFound->bMajorVersion = pRawSMBIOSData->SMBIOSMajorVersion;
  pSmBiosFound->bMinorVersion = pRawSMBIOSData->SMBIOSMinorVersion;
  pSmBiosFound->wMaxStructSize = 0; /* Done further down: Compute the actual size */
  pSmBiosFound->wTotalSize = (WORD)(pRawSMBIOSData->Length); /* Assume total is always less than 64K */
  /* Record the access method in the first byte of the reserved area, at offset 0B */
  pSmBiosFound->bFormatted[0] = bMethod;
#ifndef _WIN64	/* WIN32 case - Address is 32-bits */
  pSmBiosFound->dwPhysicalAddress = (DWORD)(pSmBiosData);
#else		/* WIN64 case = Address is 64-bits */
  pSmBiosFound->dwPhysicalAddress = DWORD0(pSmBiosData);
  /* Store the upper half of the address in the rest of the reserved area, at offset 0C */
  *(DWORD *)(pSmBiosFound->bFormatted+1) = DWORD1(pSmBiosData);
#endif
  pSmBiosFound->wNumStructures = 0; /* Done further down: Compute the number of structures */
  pSmBiosFound->bcdRevision = (pSmBiosFound->bMajorVersion << 4) | pSmBiosFound->bMinorVersion;

  /* Scan the SMBIOS structures to get the max size */
  for (lpb = (LPBYTE)(pSmBiosData);
       lpb < ((LPBYTE)(pSmBiosData) + pSmBiosFound->wTotalSize);
       ) {
    BYTE *lpb0 = lpb;
    WORD wSize;
    if (!lpb[1]) break;	/* Prevent infinite loops */
    pSmBiosFound->wNumStructures += 1;
    lpb += lpb[1]; /* Skip the structure body */
    if (!*lpb) lpb++; /* Skip one extra NUL if there's no trailing string */
    while (*(lpb++)) while (*(lpb++)) ; /* Skip trailing strings */
    wSize = (WORD)(lpb-lpb0);
    if (pSmBiosFound->wMaxStructSize < wSize) pSmBiosFound->wMaxStructSize = wSize;
  }

  return iSmbiosVersion;
}

#endif /* defined(_WIN32) */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosOpen/Close					      |
|									      |
|   Description:    Manage SMBIOS access like files, with an SMBIOS handle    |
|									      |
|   Notes:	    							      |
|									      |
|   History:								      |
|    2016-04-27 JFL Created these routines.				      |
*									      *
\*---------------------------------------------------------------------------*/

HSMBIOS SmBiosOpen() {
  HSMBIOS hSMBIOS = (HSMBIOS)malloc(sizeof(SMBIOS21HEADER));
  if (hSMBIOS) {
    if (!SmBiosInit(hSMBIOS, 0)) {
      free(hSMBIOS);
      hSMBIOS = NULL;
    }    
  }
  return hSMBIOS;
}

void SmBiosClose(HSMBIOS hSMBIOS) {
  if (!hSMBIOS) return; /* It's OK to close a closed handle */
#ifdef _WIN32
  /* Free the table copy we got from Windows */
  free(SmBiosGetDataAddress(hSMBIOS) - sizeof(RAWSMBIOSDATA));
#endif /* _WIN32 */
  /* Free the handle structure */
  free(hSMBIOS);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosGetDataAddress				      |
|									      |
|   Description:    Get a valid pointer to the SMBIOS table data	      |
|									      |
|   Notes:	    The physical tables header contains a physical address,   |
|		    which is not a valid pointer in any C memory model.	      |
|									      |
|   History:								      |
|    2016-03-28 JFL Created this routine.  				      |
*									      *
\*---------------------------------------------------------------------------*/

LPBYTE SmBiosGetDataAddress(SMBIOS21HEADER *p21Hdr) {
  LPBYTE lpb;
  #ifdef _MSDOS
    /* Convert the physical address to a 16:16 far pointer */
    WORD1(lpb) = (WORD)(p21Hdr->dwPhysicalAddress >> 4);
    WORD0(lpb) = (WORD)(p21Hdr->dwPhysicalAddress & 0x0F);
  #endif
  #ifdef _WIN32
    #ifndef _WIN64	/* WIN32 case - Address is 32-bits */
      lpb = (void far *)(p21Hdr->dwPhysicalAddress);
    #else		/* WIN64 case = Address is 64-bits */
      DWORD0(lpb) = (DWORD)(p21Hdr->dwPhysicalAddress);
      DWORD1(lpb) = *(DWORD *)(p21Hdr->bFormatted+1);
    #endif
  #endif
  return lpb;
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosAllocStruct					      |
|									      |
|   Description:    Create a buffer big enough for any SMBIOS table data      |
|									      |
|   Notes:	    							      |
|									      |
|   History:								      |
|    2016-04-27 JFL Created this routine.  				      |
*									      *
\*---------------------------------------------------------------------------*/

void *SmBiosAllocStruct(HSMBIOS hSMBIOS) {
  return malloc(hSMBIOS->wMaxStructSize);
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosGetFullStructureSize				      |
|									      |
|   Description:    Get the full size of an SMBIOS structure with its strings |
|									      |
|   Parameters:     BYTE *pStruct 	    Structure address		      |
|									      |
|   Returns:	    The full size of the SMBIOS structure, including strings. |
|									      |
|   Notes:	    The size field at index 1 does NOT include the trailing   |
|		    strings.						      |
|									      |
|   History:								      |
|    2016-03-29 JFL Created this routine from code in SmBiosGetStructByHandle.|
*									      *
\*---------------------------------------------------------------------------*/

int SmBiosGetFullStructureSize(LPVOID lpStruct) {
  BYTE FAR *lpb = lpStruct;
  lpb += lpb[1];			/* Pass the structure per se */
  if (!*lpb) lpb++;			/* Skip extra NUL if there's no trailing string */
  while (*(lpb++)) while (*(lpb++)) ;	/* Pass trailing strings */
  return (int)(lpb - (LPBYTE)lpStruct);	/* No structure is greater than 64KB anyway  */
}

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosGetStructByHandle				      |
|									      |
|   Description:    Get a copy of a given SMBIOS structure 		      |
|									      |
|   Parameters:     HSMBIOS pHdr	SMBIOS tables descriptor	      |
|		    int hStruct 	Structure handle		      |
|		    void *pBuf		Output buffer			      |
|									      |
|   Returns:	    The handle of the next table. -1=none. 0=Error.	      |
|									      |
|   Notes:	    This routine is designed to hide the SMBIOS implementation|
|									      |
|		    When the BIOS supports multiple SMBIOS methods, it is     |
|		    possible to select the one that will be used here:	      |
|		    Set the dwSignature to one of possible methods:	      |
|		     $DMI   Old SMBIOS 1.0 tables in ROM		      |
|		     $PnP   SMBIOS 2.0+ PnP-style API			      |
|		     _DMI   HP-proprietary 32-bits SMBIOS 2.0 tables in ROM   |
|		     _SM_   SMBIOS 2.1+ tables in ROM			      |
|									      |
|		    For the $PnP case, store the PnP BIOS structure in the    |
|		    first four bytes of field bFormatted.		      |
|		    This is done automatically by SmBiosInit().		      |
|									      |
|   History:								      |
|    1999/07/08 JFL Created this routine, reusing code from DMIDUMP.C.        |
|    1999/08/17 JFL Added a workaround for a bug in the Gap BIOS:	      |
|		    When invoking the $PnP method from a DOS box under NT,    |
|		    the first call to read table 0 succeeds, but it reports   |
|		    that this is the last table. To work around this, use     |
|		    the SMBIOS 2.1+ 32-bits table access method (_SM_).	      |
*									      *
\*---------------------------------------------------------------------------*/

int SmBiosGetStructByHandle(HSMBIOS p21Hdr, int hStruct, void *pBuf)
    {
    BYTE *pStruct = (BYTE *)pBuf;
#ifdef _MSDOS
    int iErr;
    LPPNPHEADER lpPnpHdr;
    LPPNPBIOS lpPnpBios;
    int hStruct0;
    static int isGap = FALSE;
#endif /* defined(_MSDOS) */
    int i;
    BYTE FAR *lpb;
    BYTE FAR *lpb0;

    /* Clear the buffer to avoid seeing false strings afterwards */
    /* Fixes a bug with some BIOS versions that do not append a second
      	NUL after the last string. */
    memset(pStruct, 0, p21Hdr->wMaxStructSize);

    switch (p21Hdr->dwSignature)
	{
#ifdef _MSDOS
	case $PnP:	/* SMBIOS 2.0+ 16-bits-PnP-style tables */
	    if (isGap) goto case_SM_;
	    /* We saved the PnP pointer in an unused portion of the SMBIOS header */
	    lpPnpHdr = *(LPPNPHEADER *)(p21Hdr->bFormatted);
	    lpPnpBios = lpPnpHdr->lpRmEntry;
	    /* The handle is defined by the BIOS */
	    hStruct0 = hStruct; /* Remember which handle we were looking for */
	    iErr = lpPnpBios(0x51, (LPVOID)&hStruct, (LPVOID)pStruct,
			     (WORD)(p21Hdr->dwPhysicalAddress >> 4),
			     lpPnpHdr->wRmDS);
	    if (iErr) return 0;

	    if ((hStruct0 == 0) && (hStruct == -1)) /* If there's only one table */
		{   /* This is the Gap bug under NT. Use the _SM_ method now on */
		isGap = TRUE;
		hStruct = 1;	/* The _SM_ method number tables sequencially */
		}

	    return hStruct; /* Handle of next structure */
#endif /* defined(_MSDOS) */

	case $DMI:	/* SMBIOS 1.0 tables in ROM */
	    /* The handle is the table number (our choice) */
	    if ((WORD)hStruct >= p21Hdr->wNumStructures) return 0; /* inexistent */

	    /* Scan the SMBIOS structures */
	    /* Convert the physical address to a 16:16 far pointer */
	    WORD1(lpb) = (WORD)(p21Hdr->dwPhysicalAddress >> 4);
	    WORD0(lpb) = (WORD)(p21Hdr->dwPhysicalAddress & 0x0F);
	    for (i = 0; i < hStruct; i++, lpb += lpb[1]) ; /* Pass previous ones */
	    _fmemcpy(pStruct, lpb, lpb[1]);	/* Copy the requested structure */
	    hStruct += 1;
	    if ((WORD)hStruct == p21Hdr->wNumStructures) hStruct = -1; /* No more */
	    return hStruct;

	case _DMI:	/* SMBIOS 2.0 HP-proprietary extensions */
	case _SM_:	/* SMBIOS 2.0+ tables in ROM */
	    #ifdef _MSDOS
case_SM_:
		/* Don't support the case when the 32-bits tables are above 1MB */
		if (p21Hdr->dwPhysicalAddress > 0x100000L) return 0;
	    #endif

	    /* Scan the SMBIOS structures */
	    lpb = SmBiosGetDataAddress(p21Hdr);
	    lpb0 = lpb; /* Useless as hStruct >= 0, but avoids a warning */
	    for (i = 0; i <= hStruct; i++) {   /* Pass previous structures */
		lpb0 = lpb;
		lpb += SmBiosGetFullStructureSize(lpb);
		}
	    _fmemcpy(pStruct, lpb0, lpb - lpb0); /* Copy the requested structure */
	    hStruct += 1;
	    if ((WORD)hStruct == p21Hdr->wNumStructures) hStruct = -1; /* No more */
	    return hStruct;

	default:
	    /* Tables access method unsupported by this program yet */
	    return 0;
	}
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosGetStructByType				      |
|									      |
|   Description:    Get a copy of a given SMBIOS structure 		      |
|									      |
|   Parameters:     HSMBIOS pHdr	SMBIOS tables descriptor	      |
|		    int iType		Structure type			      |
|		    void *pBuf		Output buffer			      |
|									      |
|   Returns:	    The handle of the table found. -1=not found.	      |
|									      |
|   Notes:	    This routine is designed to hide the SMBIOS implementation|
|									      |
|		    IMPORTANT: The output buffer must be large enough for     |
|		    the largest SMBIOS structure, even if only a shorter one  |
|		    is being read.					      |
|									      |
|		    See comments in SmBiosGetStructByHandle() for details on  |
|		    the p21Hdr parameter.				      |
|									      |
|   History:								      |
|    1999-07-12 JFL Created this routine.				      |
|    2016-07-09 JFL Use only -1 as an error, as 0 is a valid structure handle.|
*									      *
\*---------------------------------------------------------------------------*/

int SmBiosGetStructByType(HSMBIOS p21Hdr, int iType, void *pBuf)
    {
    int hStruct;
    int hNext = -1;
    BYTE bType;
    BYTE *pStruct = (BYTE *)pBuf;

    /* Scan structures until we find the right one */
    for (hStruct = 0; hStruct != -1; hStruct = hNext)
	{
	hNext = SmBiosGetStructByHandle(p21Hdr, hStruct, pStruct);
	if (!hNext) return -1; /* Unexpected error reading the structure */

	bType = pStruct[0];
	if (iType == (int)bType) return hStruct;   /* Found */
	}

    return -1; /* Not found */
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosGetString					      |
|									      |
|   Description:    Get the Nth string in an SMBIOS 2 structure.	      |
|									      |
|   Parameters:     HSMBIOS pHdr	SMBIOS tables descriptor	      |
|		    BYTE *pStruct	SMBIOS 2.x structure address	      |
|		    unsigned int n	Index of the string to get. First = 1.|
|									      |
|   Returns:	    char *pszString, or NULL if bad index.		      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    2002/07/15 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

char *SmBiosGetString(HSMBIOS p21Hdr, BYTE *pStruct, unsigned int n)
    {
    int i;
    char *psz;

    if (!*(WORD far *)(pStruct+pStruct[1])) return NULL; /* No strings */
    /* Protect ourselves against buggy tables that may lack the final extra NUL */
    for (i = pStruct[1]; (WORD)i < p21Hdr->wMaxStructSize; i+=((int)strlen(psz)+1))
	{
	psz = (char *)(pStruct+i);
	if (!*psz) break;	/* End ef strings reached */
        if (!--n) return psz;	/* Nth string reached */
	}
    return NULL;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    SmBiosIsHpPc					      |
|									      |
|   Description:    Check if the system was manufactured by HP / HPE	      |
|									      |
|   Notes:	    This is a prerequisite before reading HP-specific tables. |
|									      |
|   History:								      |
|    2016-04-27 JFL Created this routine.  				      |
|    2016-07-03 JFL Added DMI 1.x method.  				      |
*									      *
\*---------------------------------------------------------------------------*/

#define streq(s1, s2) (!strcmp(s1, s2))

int SmBiosIsHpPc(HSMBIOS p21Hdr) {
  int isHP = FALSE;
  PSMBIOSSYSTEMINFO pSmbiosSystemInfo = (PSMBIOSSYSTEMINFO)SmBiosAllocStruct(p21Hdr);
  if (pSmbiosSystemInfo) {
    if (p21Hdr->bMajorVersion == 1) {	/* DMI BIOS 1.x */
      if (SmBiosGetStructByType(p21Hdr, 0x80, pSmbiosSystemInfo) >= 0) {
	isHP = (((WORD *)pSmbiosSystemInfo)[1] == 0x5048); /* 'HP' at offset 2 in structure type 0x80 */
      }
    } else {				/* SMBIOS 2.0 or later */
      if (SmBiosGetStructByType(p21Hdr, 0, pSmbiosSystemInfo) >= 0) {
	char *pszMaker = SmBiosGetString(p21Hdr, (BYTE *)pSmbiosSystemInfo, pSmbiosSystemInfo->bManufacturer);
	isHP = (   (streq(pszMaker, "HP"))
		|| (streq(pszMaker, "Hewlett-Packard"))
		|| (streq(pszMaker, "Hewlett Packard Enterprise"))
	       );
      }
    }
    free(pSmbiosSystemInfo);
  }
  return isHP;
}

