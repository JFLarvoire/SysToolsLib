/*****************************************************************************\
*                                                                             *
*   Filename:	    NetBIOS.c						      *
*									      *
*   Description:    OS-independant NetBIOS access routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    The Win32 implementation is trivial.		      *
*									      *
*   History:								      *
*    1995/05/30 JFL Adapted from "Windows Network Programming" book.	      *
*    2000/09/21 JFL Restructured.					      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "NetBIOS.h"

/*****************************************************************************\
*                                                                             *
*                               WIN32 Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _WIN32
#endif /* _WIN32 */

/*****************************************************************************\
*                                                                             *
*                               MSDOS Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _MSDOS

#include <dos.h>
#include <memory.h>

/*---------------------------------------------------------------------------*\
*                                                                             *
|  Function:   NetBios							      |
|                                                                             |
|  Purpose:    Call NetBIOS, passing it a Network Control Block 	      |
|                                                                             |
|  Parameters: See below                                                      |
|                                                                             |
|  Return:     NetBIOS return code					      |
|                                                                             |
|  Creation:   95/05/30 by JFL						      |
|                                                                             |
|  Modification history:                                                      |
|  Date     Author   Description                                              |
|  -------- -------- ------------------------------------------------------   |
*                                                                             *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4704)   /* Ignore the inline assembler etc... warning */

API_FUNCTION NetBios(LPNCB lpNCB)
    {
    BYTE bRetCode;
    BOOL bNoWait;

    bNoWait = ((lpNCB->ncb_command & ASYNCH) != 0);

    _asm
	{
	les	bx, lpNCB
	int	5CH
	mov	bRetCode, al
	}
    if (!bNoWait)
	bRetCode = lpNCB->ncb_retcode;

    return bRetCode;
    }

#pragma warning(default:4704)   /* Restore the inline assembler etc... warning */

/*---------------------------------------------------------------------------*\
*                                                                             *
|  Function:   IsNetBiosInstalled					      |
|                                                                             |
|  Purpose:    Make sure NetBios is available to receive commands	      |
|                                                                             |
|  Parameters: See below                                                      |
|                                                                             |
|  Return:     TRUE if NetBIOS is available				      |
|                                                                             |
|  Creation:   95/05/30 by JFL						      |
|                                                                             |
|  Modification history:                                                      |
|  Date     Author   Description                                              |
|  -------- -------- ------------------------------------------------------   |
*                                                                             *
\*---------------------------------------------------------------------------*/

BOOL IsNetBiosInstalled(void)
    {
    unsigned int wNBRetCode;
    NCB ncbTest;
    DWORD dwHandler;

    /* Make sure interrupt vector 5C is not NULL */
    dwHandler = (DWORD)(_dos_getvect(NB_INTERRUPT));
    if ((dwHandler == 0) || (dwHandler >= 0xF0000000))
	return FALSE;

    /* Issue an invalid command, expecting a meaningful error code */
    memset(&ncbTest, 0, sizeof(NCB));
    ncbTest.ncb_command = 0xFF;
    wNBRetCode = NetBios(&ncbTest);

    switch (wNBRetCode)
	{
	case NRC_ILLCMD:
	/* case NB_ERR_BAD_ADAPTER: (?) */
	    return TRUE;

	default:
	    /* Return codes 0x40 to 0x4F mean "Unusual network condition"
			    0x50 to 0xFE mean "Adapter malfunction"
		These mean NetBIOS is there */
	    if ((wNBRetCode >= 0x40) && (wNBRetCode <= 0xFE))
		return TRUE;
	    else
		return FALSE;
	}
    }

#endif /* _MSDOS */

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/

