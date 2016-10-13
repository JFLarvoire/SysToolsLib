/*****************************************************************************\
*                                                                             *
*   Filename:	    macaddr.c						      *
*									      *
*   Description:    OS-independant MAC address query routine		      *
*                                                                             *
*   Notes:	    The Windows version gets the MAC address from Windows.    *
*		    The DOS version first tries to get it from the SMBIOS     *
*		    tables (which is fast, but not available on all systems), *
*		    else if the LMPTK is available it gets it from LAN        *
*		    Manager (which always works, but is very slow.)	      *
*		    							      *
*   History:								      *
*    2016-04-23 JFL Split this file off of uuid.c.			      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "macaddr.h"

extern int iVerbose;	/* Defined in main module. If TRUE, display detailed information */
#ifdef _DEBUG
extern int iDebug;	/* Defined in main module. If TRUE, display debug information */
#endif /* _DEBUG */

/*****************************************************************************\
*                                                                             *
*                           BIOS and MSDOS Routines                           *
*                                                                             *
\*****************************************************************************/

#if defined(_MSDOS)

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>

#include "smbios.h"	/* SmBios access, to get system info, including MAC@ */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    GetMacAddress					      |
|									      |
|   Description:    Find the MAC address of the LAN card.		      |
|									      |
|   Parameters:     BYTE *pbMacAddr	Where to store the MAC address	      |
|									      |
|   Returns:	    TRUE if found, FALSE if not found.			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    1997-09-02 JFL Created this routine				      |
|    2016-04-23 JFL Use NetBIOS only for DOS, and if LanMan 2.1 PTK is avail. |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(HAS_LMPTK) && !defined(_BIOS)
#define USE_NETBIOS 1
#else
#define USE_NETBIOS 0
#endif

#if USE_NETBIOS

#include <sys/types.h>
#include <time.h>
#include <process.h>
#include <dos.h>

#include "NetBIOS.h"	/* NETBIOS access */

#else	/* In the absence of NetBIOS APIs, only use the SMBIOS table data */

#define TRUE 1
#define FALSE 0

#endif /* USE_NETBIOS */

#pragma warning(disable:4706) /* Avoid warnings "assignment within conditional expression" */

int GetMacAddress(BYTE *pbMacAddr)
    {
    HSMBIOS hSMBIOS = NULL;
    PSMBIOSLANINFO pLanInfo = NULL;	/* Mark it as invalid */
    int iFound = FALSE;

    memset(pbMacAddr, 0, MACADDRESS_SIZE);	 /* Assume failure */

    /* First try to get the MAC address from the SMBIOS tables.
       HP PCs record it during POST in table type #0x85 = 133.
       HP Servers record several during POST in table type #209.
       Reading SMBIOS tables is much faster than querying NetBIOS. */
    if (   (hSMBIOS = SmBiosOpen())
        && (SmBiosIsHpPc(hSMBIOS)) /* Make sure HP-proprietary structures are usable */
        && (pLanInfo = (PSMBIOSLANINFO)SmBiosAllocStruct(hSMBIOS))
        && (SmBiosGetStructByType(hSMBIOS, 0x85, pLanInfo) >= 0)
       )
	{
	memcpy(pbMacAddr, pLanInfo->bMacAddress, MACADDRESS_SIZE);
	SmBiosClose(hSMBIOS);
	iFound = TRUE;
	}
#if USE_NETBIOS
    else
        {
	NCB        sNcb;			 	/* Our NCB structure */
	char       buf[NETBIOSBUFSIZE];
	PASTATBUF  pAstatBuf = (PASTATBUF)buf;		/* Returned adapter status buffer */
	BYTE       bFiller = 0;				/* default filler byte = 0 */
	BYTE       bLana = 0;				/* default to adapter 0 */
	BYTE       bLastByte = bFiller;
	API_RET_TYPE rc;
	BYTE       fNameFlag = UNINITIALIZED;
	BYTE       stRemoteName[NCBNAMSZ];
	
	memset (&stRemoteName, bFiller, NCBNAMSZ);	/* default to local status */
	stRemoteName[0] = '*';
	
	/* Prepare to submit the adapter status request */
	
	sNcb.ncb_command = NCBASTAT;
	sNcb.ncb_buffer = (char far *)pAstatBuf;
	sNcb.ncb_length = NETBIOSBUFSIZE;
	sNcb.ncb_lana_num = bLana;
	
	memset (&(sNcb.ncb_name), bFiller, NCBNAMSZ);
	memset (&(sNcb.ncb_callname), bFiller, NCBNAMSZ);
	memcpy (&(sNcb.ncb_callname), &stRemoteName, NCBNAMSZ);
	
	if (fNameFlag != AADDRESS)
	    sNcb.ncb_callname[NCBNAMSZ-1] = bLastByte;
	
	rc = NetBios(&sNcb);
	
	switch (sNcb.ncb_retcode)
	    {
	    case 0:
	    case 6:
		memcpy(pbMacAddr, pAstatBuf->adapter_address, MACADDRESS_SIZE);
		iFound = TRUE;
		break;
	
	    default:
#ifdef _DEBUG
	        if (iDebug) printf("NetBios error 0x%x (%u)",
				     sNcb.ncb_retcode, sNcb.ncb_retcode);
#endif /* defined(_DEBUG) */
		break;
	    }
	}
#endif /* USE_NETBIOS */
	    
#ifdef _DEBUG
    if (iDebug)
    	{
    	if (iFound) 
    	     {
    	     int i;
    	     printf("MAC Address = ");
    	     for (i=0; i<MACADDRESS_SIZE; i++) printf("%02.2X", pbMacAddr[i]);
    	     printf("\n");
    	     }
    	}
#endif /* defined(_DEBUG) */
    	
    free(pLanInfo);	/* NULL pointers are ignored. */
    
    return iFound;
    }

#pragma warning(default:4706)

#endif /* defined(_MSDOS) */

/*****************************************************************************\
*                                                                             *
*                               WIN32 Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _WIN32

#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

int GetMacAddress(BYTE *pbMacAddr) {
  IP_ADAPTER_INFO AdapterInfo;			/* Allocate 1 structure for the first adapter */
  DWORD dwBufLen = sizeof(AdapterInfo);		/* Buffer size */
 
  DWORD dwStatus = GetAdaptersInfo(&AdapterInfo,/* [out] buffer to receive data */
				   &dwBufLen);	/* [in] size of receive data buffer */

  if ((dwStatus != ERROR_SUCCESS) && (dwStatus != ERROR_BUFFER_OVERFLOW)) {
    ZeroMemory((LPVOID)pbMacAddr, MACADDRESS_SIZE);
    return 0; /* Not found */
  }
 
  CopyMemory((LPVOID)pbMacAddr, AdapterInfo.Address, MACADDRESS_SIZE); /* Copy the MAC Address */
  return 1; /* Found */
}

#endif /* defined(_WIN32) */

