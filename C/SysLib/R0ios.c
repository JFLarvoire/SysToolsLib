/*****************************************************************************\
*                                                                             *
*   Filename:	    R0ios.c						      *
*									      *
*   Description:    Windows 95 I/O Supervisor VxD access from WIN32.	      *
*                                                                             *
*   Notes:	    Compilable both in C and C++ without any warning.	      *
*		    							      *
*		    See Win98 DDK's blockdev.h, ios.h, and ior.h for	      *
*		    definitions and parameters for IOS commands.	      *
*		    							      *
*   History:								      *
*    2001-09-07 JFL Moved IOS access routines from Ring0.c to this new file.  *
*    2001-10-01 JFL Added include file r0ios.h.				      *
*    2001-12-21 JFL Include qword.h.					      *
*    2017-06-29 JFL Fixed compilation warnings. No functional code change.    *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#pragma message("Compiling " __FILE__ )

#include "Ring0.h"	// General Ring0 access mechanism
#include "R0ios.h"	// Public definitions for this module
#include "qword.h"

/*---------------------------------------------------------------------------*\
|									      *
|   Description:    IOS service table					      |
|									      |
|   Notes:	    This table is not defined in ios.h			      |
|									      |
|   History:								      |
|									      |
|     2001/05/30 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#define IOS_DEVICE_ID	0x00010
#define	IOS_Service	Declare_Service

Begin_Service_Table	(IOS, VxD)

  IOS_Service(IOS_Get_Version, LOCAL)
  IOS_Service(IOS_BD_Register_Device, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Find_Int13_Drive, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Get_Device_List, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_SendCommand, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_BD_Command_Complete, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Synchronous_Command, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Register, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Requestor_Service, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Exclusive_Access, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Send_Next_Command, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Set_Async_Time_Out, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_Signal_Semaphore_No_Switch, VxD_PAGEABLE_CODE)
  IOS_Service(IOSIdleStatus, VxD_PAGEABLE_CODE)
  IOS_Service(IOSMapIORSToI24, VxD_PAGEABLE_CODE)
  IOS_Service(IOSMapIORSToI21, VxD_PAGEABLE_CODE)
  IOS_Service(PrintLog, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_deregister, VxD_PAGEABLE_CODE)
  IOS_Service(IOS_wait, VxD_PAGEABLE_CODE)
  IOS_Service(_IOS_SpinDownDrives, VxD_PAGEABLE_CODE)
  IOS_Service(_IOS_query_udf_mount, VxD_PAGEABLE_CODE)

End_Service_Table(IOS, VxD)

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0IosFindInt13Drive					      |
|									      |
|   Description:    Front end to IOS_Find_Int13_Drive()			      |
|									      |
|   Parameters:     DWORD dwUnitNumber		BIOS drive number	      |
|									      |
|   Returns:	    _BlockDev_Device_Descriptor *			      |
|									      |
|   Notes:	    Finds the IOS descriptor block for a given BIOS drive     |
|									      |
|   History:								      |
|									      |
|     2001/05/30 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

DWORD R0IosFindInt13DriveCB(DWORD dwUnitNumber)
    {
    _asm
    	{
	mov	eax, dwUnitNumber  ; int 13h unit number
	VxDCall(IOS_Find_Int13_Drive);
	mov	eax, edi ; address of BDD
	jnc	done
	xor	eax, eax
done:
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning

_BlockDev_Device_Descriptor *R0IosFindInt13Drive(DWORD dwUnitNumber)
    {
    return (_BlockDev_Device_Descriptor *)R0CallCallBack(R0IosFindInt13DriveCB, dwUnitNumber);
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0IosGetDeviceList					      |
|									      |
|   Description:    Front end to IOS_Get_Device_List()			      |
|									      |
|   Parameters:     None						      |
|									      |
|   Returns:	    _BlockDev_Device_Descriptor *			      |
|									      |
|   Notes:	    Finds the first IOS descriptor block		      |
|									      |
|   History:								      |
|									      |
|     2001/09/06 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#pragma warning(disable:4100)	// Ignore the unreferenced formal parameter warning

DWORD R0IosGetDeviceListCB(DWORD dwParm)
    {
    _asm
    	{
	VxDCall(IOS_Get_Device_List);
	mov	eax, edi ; address of BDD list head
	jnc	done
	xor	eax, eax
done:
	}
#pragma warning(disable:4035)	// Ignore the no return value warning
    }
#pragma warning(default:4035)	// Restore the no return value warning
#pragma warning(default:4100)	// Restore the unreferenced formal parameter warning

_BlockDev_Device_Descriptor *R0IosGetDeviceList(void)
    {
    return (_BlockDev_Device_Descriptor *)R0CallCallBack(R0IosGetDeviceListCB, 0);
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0IosSendCommand					      |
|									      |
|   Description:    Front end to IOS_SendCommand()			      |
|									      |
|   Parameters:     _BlockDev_Command_Block *pCmdData	    Command descriptor|
|		    _BlockDev_Device_Descriptor *pDevData   Device descriptor |
|									      |
|   Returns:	    0 = Success, else IORS status.			      |
|									      |
|   Notes:	    This implementation manages only blockdev.386 compatible  |
|		    functions and parameters.				      |
|									      |
|									      |
|   History:								      |
|									      |
|     2001/05/30 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

void *pR0IosCommandCompletionProcedure = NULL;  // Pointer to the second instance in non-pageable pool.
int iR0FreeIosCommandCompletionProcedure = FALSE;

void R0FreeIosCommandCompletionProcedure(void)	// Cleanup routine called atexit().
    {
    if (pR0IosCommandCompletionProcedure)
        {
        R0HeapFree(pR0IosCommandCompletionProcedure, 0);
        pR0IosCommandCompletionProcedure = NULL;
        }
    }

void R0IosCommandCompletionProcedure()		// Must be immediately before R0IosSendCommandCB().
    {
    _BlockDev_Command_Block *pBCB;
    VMM_SEMAPHORE waitsem;			// semaphore to wait on

    _asm mov pBCB, esi
    waitsem = (VMM_SEMAPHORE)(pBCB->BD_CB_Req_Req_Handle);
    
    _asm mov eax, waitsem
    VMMCall(Signal_Semaphore_No_Switch);	// wakeup waiting thread
    }

DWORD R0IosSendCommandCB(DWORD dwRef)	// Must follow immediately after R0IosCommandCompletionProcedure().
    {
    _BlockDev_Command_Block *pBCB = *((_BlockDev_Command_Block **)dwRef);
    
    // Clone CompletionProcedure in non-pageable space
    if (!pR0IosCommandCompletionProcedure)
        {
        DWORD dwSize;
#pragma warning(disable:4054) /* Disable the warning "'type cast' : from function pointer to data pointer 'void *'" */
        dwSize = (char *)(void *)R0IosSendCommandCB - (char *)(void *)R0IosCommandCompletionProcedure;
        pR0IosCommandCompletionProcedure = _HeapAllocate(dwSize, 0);
        if (!pR0IosCommandCompletionProcedure) return BDS_Memory_Error;
        memcpy(pR0IosCommandCompletionProcedure, (void *)R0IosCommandCompletionProcedure, dwSize);
#pragma warning(default:4054) /* Restore the warning "'type cast' : from function pointer to data pointer 'void *'" */
        }

    // Update the BCB to manage the completion
    pBCB->BD_CB_Cmd_Cplt_Proc = (DWORD)pR0IosCommandCompletionProcedure;
    pBCB->BD_CB_Req_Req_Handle = (DWORD)Create_Semaphore(0);
    if (!pBCB->BD_CB_Req_Req_Handle) return BDS_Unrec_Error;
    
    // Execute the I/O
    _asm
    	{
    	mov	ebx, dwRef
	mov	esi, [ebx]	; CmdData  ; address of BCB or IOR
	mov	edi, [ebx+4]	; DevData  ; address of BDD or DCB
	VxDCall(IOS_SendCommand)
	}

    // Wait for its completion
    Wait_Semaphore(pBCB->BD_CB_Req_Req_Handle, BLOCK_SVC_INTS);
    
    // Cleanup
    Destroy_Semaphore(pBCB->BD_CB_Req_Req_Handle);
    
    return pBCB->BD_CB_Cmd_Status;
    }

#pragma warning(disable:4100) /* Disable the warning "unreferenced formal parameter" */

DWORD R0IosSendCommand(_BlockDev_Command_Block *pCmdData, _BlockDev_Device_Descriptor *pDevData)
    {
    if (!iR0FreeIosCommandCompletionProcedure)
        {
        atexit(R0FreeIosCommandCompletionProcedure);
        iR0FreeIosCommandCompletionProcedure = TRUE;	// Make sure to do this only once.
        }

    return R0CallCallBack(R0IosSendCommandCB, (DWORD)&pCmdData);
    }

#pragma warning(default:4100) /* Restore the warning "unreferenced formal parameter" */

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0IosService					      |
|									      |
|   Description:    Front end to IOS services 				      |
|									      |
|   Parameters:     ISP *pIsp		Service descriptor, derived from ISP. |
|									      |
|   Returns:	    0 = Success, else ISP error.			      |
|									      |
|   Notes:	    Registers as a dummy device driver to get the service     |
|		    routine address.					      |
|									      |
|   History:								      |
|									      |
|     2001/09/13 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#include <ilb.h>	// IOS Linkage Block
#include <drp.h>	// Driver Registration Packet Structure

PFNISP pIosServiceRoutine = NULL;   // Address of IOS service routine.
ILB ilb = {0};			    // IOS information packet passed to registered drivers

DWORD R0IosServiceCB(DWORD dwParm)
    {
    PISP pIsp = (PISP)dwParm;

    if (!pIosServiceRoutine)	// If first call, then determine the address
	{
	DRP drp = 
	    { 
		{'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'}, // CHAR   DRP_eyecatch_str[8];  // eye catcher string
	    DRP_FSD,	// ULONG  DRP_LGN;              // load group number / Chosen to avoid calls to aer
	    NULL,	// PVOID  DRP_aer;              // addr of asynchronous event routine
	    &ilb,	// PVOID  DRP_ilb;              // address of ILB
	    "DummyIosDriver", // CHAR   DRP_ascii_name[16];   // driver name
	    1,		// BYTE   DRP_revision;         // driver revision
	    0,		// ULONG  DRP_feature_code;     // feature code
	    DRP_IF_STD , // USHORT DRP_if_requirements;  // I/F requirements
	    0,		// UCHAR  DRP_bus_type;         // type of I/O bus; see below
	    0,		// USHORT DRP_reg_result;       // registration result; see below
	    0,		// ULONG  DRP_reference_data;   // data to pass with AEP_INITIALIZE
		{0},	// UCHAR  DRP_reserved1[2];     // reserved
	    0,		// ULONG  DRP_reserved2[1];     // reserved; must be zero
	    };

	_asm
    	    {
	    lea	    eax, drp
	    push    eax
	    VxDCall(IOS_Register);
	    add	    esp, 4
	    }

	pIosServiceRoutine = ilb.ILB_service_rtn;
	}

    if (!pIosServiceRoutine) return 0xBADBAD;

    pIosServiceRoutine(pIsp);
    return pIsp->ISP_result;
    }

WORD R0IosService(PISP pIsp)
    {
    return (WORD)R0CallCallBack(R0IosServiceCB, (DWORD)pIsp);
    }

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    R0IosFindDcb					      |
|									      |
|   Description:    Find the DCB for a given device			      |
|									      |
|   Parameters:     BYTE bDeviceType	    DCB_type_floppy, DCB_type_disk,   |
|					    DCB_type_cdrom, etc...	      |
|		    BYTE bUnitNumber	    0...9 the index in the DCB list,  |
|					    0x80...0x89 The hard disk BIOS #. |
|									      |
|   Returns:	    IOSDCB * if found, or NULL if not found.		      |
|									      |
|   Notes:	    Hard disks unit numbers are equal to their BIOS drive     |
|		    number (0x80, 0x81, ...). Drive 80 is first in the DCB    |
|		    chain on my test PC, but this unknown in the general case.|
|									      |
|		    Floppy disks unit numbers are always 0 on my test PCs...  |
|		    which is the same as their BIOS drive number, but proves  |
|		    nothing. Ordering in case of multiple floppys is unknown. |
|									      |
|		    The CD-ROM unit number is 4 on my test PC under WinME.    |
|		    this is why I changed the bUnitNumber parameter to mean   |
|		    the index in the DCB list, instead of the actual number.  |
|									      |
|   History:								      |
|									      |
|     2001/10/02 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

IOSDCB *R0IosFindDcb(BYTE bDeviceType, BYTE bUnitNumber)
    {
    ISP_GET_FRST_NXT_DCB ispGfnDCB;
    int iIndex = 0;

    ispGfnDCB.ISP_gfnd_hdr.ISP_func = ISP_GET_FIRST_NEXT_DCB;   // Standard header / Function number
    ispGfnDCB.ISP_gfnd_dcb_type = 0xFF;			    // device type or 0ffh / List all types

    for (ispGfnDCB.ISP_gfnd_dcb_offset = 0; 
	 !R0IosService((PISP)&ispGfnDCB); 
	 ispGfnDCB.ISP_gfnd_dcb_offset = ispGfnDCB.ISP_gfnd_found_dcb)
	{
	PIOSDCB pDCB = (PIOSDCB)(ispGfnDCB.ISP_gfnd_found_dcb);

	if (pDCB->DCB_cmn.DCB_physical_dcb != (DWORD)pDCB) continue;	// This is not a physical device
	if (pDCB->DCB_cmn.DCB_device_type != bDeviceType) continue;	// This is not the requested type

	if (iIndex++ == (int)bUnitNumber) return pDCB;	// Index found.
	
	// Special case for hard disks
	if (   (bDeviceType == DCB_type_disk)
	    && (pDCB->DCB_cmn.DCB_unit_number == bUnitNumber)
	   ) return pDCB;				// Hard disk BIOS drive number found!
	}

    return NULL;
    }

