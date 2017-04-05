/*****************************************************************************\
*									      *
*   File name:	    VMM.H						      *
*									      *
*   Description:    VMM Definitions for use by 16 bits programs 	      *
*									      *
*   Notes:	    Adapted from Chicago's Beta VMM.H for the 32 bits C complr.
*		    Removed all code that created problem for the 16 bits one.*
*									      *
*		    Predefine the constant WIN40SERVICES to get Chicago's     *
*		    specific services.                                        *
*		    							      *
*   History:								      *
*    1995/03/10 JFL Created by Jean-François LARVOIRE			      *
*		    							      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _VMM16_H
#define _VMM16_H

#pragma warning (disable:4209)  // turn off redefinition warning

typedef unsigned char   UCHAR;
typedef unsigned short  USHORT;
typedef unsigned long	ULONG;

#pragma warning (default:4209)  // turn off redefinition warning

// Allow to assemble simple 32 bits instructions directly
#define eax ax
#define ebx bx
#define ecx cx
#define edx dx
#define esi si
#define edi di
#define esp sp
#define ebp bp

/******************************************************************************
 *
 *   The following are control block headers and flags of interest to VxDs.
 *
 *****************************************************************************/

struct cb_s {
    ULONG CB_VM_Status;         /* VM status flags */
    ULONG CB_High_Linear;       /* Address of VM mapped high */
    ULONG CB_Client_Pointer;
    ULONG CB_VMID;
    ULONG CB_Signature;
};

#define VMCB_ID 0x62634D56      /* VMcb */

/******************************************************************************
 *
 *          EQUATES FOR REQUIRED DEVICES
 *
 *   Device ID formulation note:
 *
 *  The high bit of the device ID is reserved for future use.
 *  Microsoft reserves the device ID's 0-1FFh for standard devices.  If
 *  an OEM VxD is a replacement for a standard VxD, then it must use the
 *  standard VxD ID.
 *
 *  OEMS WHO WANT A VXD DEVICE ID ASSIGNED TO THEM,
 *  PLEASE CONTACT MICROSOFT PRODUCT SUPPORT.  ID's are only required for
 *  devices which provide services, V86 API's or PM API's.  Also, calling
 *  services or API's by VxD name is now supported in version 4.0, so an
 *  ID may not be necessary as long as a unique 8 character name is used.
 *
 *****************************************************************************/

#define UNDEFINED_DEVICE_ID 0x00000
#define VMM_DEVICE_ID       0x00001 /* Used for dynalink table */
#define DEBUG_DEVICE_ID     0x00002
#define VPICD_DEVICE_ID     0x00003
#define VDMAD_DEVICE_ID     0x00004
#define VTD_DEVICE_ID       0x00005
#define V86MMGR_DEVICE_ID   0x00006
#define PAGESWAP_DEVICE_ID  0x00007
#define PARITY_DEVICE_ID    0x00008
#define REBOOT_DEVICE_ID    0x00009
#define VDD_DEVICE_ID       0x0000A
#define VSD_DEVICE_ID       0x0000B
#define VMD_DEVICE_ID       0x0000C
#define VKD_DEVICE_ID       0x0000D
#define VCD_DEVICE_ID       0x0000E
#define VPD_DEVICE_ID       0x0000F
#define BLOCKDEV_DEVICE_ID  0x00010
#define VMCPD_DEVICE_ID     0x00011
#define EBIOS_DEVICE_ID     0x00012
#define BIOSXLAT_DEVICE_ID  0x00013
#define VNETBIOS_DEVICE_ID  0x00014
#define DOSMGR_DEVICE_ID    0x00015
#define WINLOAD_DEVICE_ID   0x00016
#define SHELL_DEVICE_ID     0x00017
#define VMPOLL_DEVICE_ID    0x00018
#define VPROD_DEVICE_ID     0x00019
#define DOSNET_DEVICE_ID    0x0001A
#define VFD_DEVICE_ID       0x0001B
#define VDD2_DEVICE_ID      0x0001C /* Secondary display adapter */
#define WINDEBUG_DEVICE_ID  0x0001D
#define TSRLOAD_DEVICE_ID   0x0001E /* TSR instance utility ID */
#define BIOSHOOK_DEVICE_ID  0x0001F /* Bios interrupt hooker VxD */
#define INT13_DEVICE_ID     0x00020
#define PAGEFILE_DEVICE_ID  0x00021 /* Paging File device */
#define SCSI_DEVICE_ID      0x00022 /* SCSI device */
#define MCA_POS_DEVICE_ID   0x00023 /* MCA_POS device */
#define SCSIFD_DEVICE_ID    0x00024 /* SCSI FastDisk device */
#define VPEND_DEVICE_ID     0x00025 /* Pen device */
#define APM_DEVICE_ID       0x00026 /* Power Management device */
#define VPOWERD_DEVICE_ID   APM_DEVICE_ID   /* We overload APM since we replace it */
#define VXDLDR_DEVICE_ID    0x00027 /* VxD Loader device */
#define NDIS_DEVICE_ID      0x00028 /* NDIS wrapper */
#define BIOS_EXT_DEVICE_ID   0x00029 /* Fix Broken BIOS device */
#define VWIN32_DEVICE_ID        0x0002A /* for new WIN32-VxD */
#define VCOMM_DEVICE_ID         0x0002B /* New COMM device driver */
#define SPOOLER_DEVICE_ID       0x0002C /* Local Spooler */
#define WIN32S_DEVICE_ID    0x0002D /* Win32S on Win 3.1 driver */
#define DEBUGCMD_DEVICE_ID      0x0002E /* Debug command extensions */
/* #define RESERVED_DEVICE_ID   0x0002F /* Not currently in use */
/* #define ATI_HELPER_DEVICE_ID    0x00030 /* grabbed by ATI */

/* 31-32 USED BY WFW NET COMPONENTS     */
/* #define VNB_DEVICE_ID           0x00031 /* Netbeui of snowball */
/* #define SERVER_DEVICE_ID        0x00032 /* Server of snowball */

#define CONFIGMG_DEVICE_ID  0x00033 /* Configuration manager (Plug&Play) */
#define DWCFGMG_DEVICE_ID   0x00034 /* Configuration manager for win31 and DOS */
#define SCSIPORT_DEVICE_ID  0x00035 /* Dragon miniport loader/driver */
#define VFBACKUP_DEVICE_ID  0x00036 /* allows backup apps to work with NEC */
#define ENABLE_DEVICE_ID    0x00037 /* for access VxD */
#define VCOND_DEVICE_ID     0x00038 /* Virtual Console Device - check vcond.inc */
/* 39 used by WFW VFat Helper device */

/* 3A used by WFW E-FAX */
/* #define EFAX_DEVICE_ID   0x0003A /* EFAX VxD ID      */

/* 3B used by MS-DOS 6.1 for the DblSpace VxD which has APIs */
/* #define DSVXD_DEVICE_ID  0x0003B /* Dbl Space VxD ID */

#define ISAPNP_DEVICE_ID    0x0003C /* ISA P&P Enumerator */
#define BIOS_DEVICE_ID      0x0003D /* BIOS P&P Enumerator */
/* #define WINSOCK_DEVICE_ID       0x0003E  /* WinSockets */
/* #define WSIPX_DEVICE_ID     0x0003F  /* WinSockets for IPX */

#define IFSMgr_Device_ID    0x00040 /* Installable File System Manager */
#define VCDFSD_DEVICE_ID    0x00041 /* Static CDFS ID */
#define MRCI2_DEVICE_ID     0x00042 /* DrvSpace compression engine */
#define PCI_DEVICE_ID       0x00043 /* PCI P&P Enumerator */
#define PELOADER_DEVICE_ID  0x00044 /* PE Image Loader */
#define EISA_DEVICE_ID      0x00045 /* EISA P&P Enumerator */
#define DRAGCLI_DEVICE_ID   0x00046 /* Dragon network client */
#define DRAGSRV_DEVICE_ID   0x00047 /* Dragon network server */
#define PERF_DEVICE_ID	    0x00048 /* Config/stat info */

/*
 *   Far East DOS support VxD ID
 */

#define ETEN_Device_ID      0x00060 /* ETEN DOS (Taiwan) driver */
#define HBIOS_Device_ID     0x00061 /* HBIOS DOS (Korean) driver */
#define VMSGD_Device_ID    0x00062 /* DBCS Message Mode driver */
#define VPPID_Device_ID     0x00063 /* PC-98 System Control PPI */

#define BASEID_FOR_NAMEBASEDVXD        0xf000 /* Name based VxD IDs start here */
#define BASEID_FOR_NAMEBASEDVXD_MASK   0x0fff /* Mask to get the real vxd id */ 

/******************************************************************************
 *
 *	    VxD service tables macros
 *
 *****************************************************************************/

#define GetVxDServiceOrdinal(service)	__##service

#define Begin_Service_Table(device) \
    enum device##_SERVICES {

#define Declare_Service(service, local) \
    GetVxDServiceOrdinal(service),

#define End_Service_Table(device) \
    Num_##device##_Services};

/******************************************************************************
 *
 *	    How to call a VxD service
 *
 *****************************************************************************/

#define VxDCall(vxd, service) \
    _asm _emit 0xcd \
    _asm _emit 0x20 \
    _asm _emit (GetVxDServiceOrdinal(service) & 0xff) \
    _asm _emit (GetVxDServiceOrdinal(service) >> 8) & 0xff \
    _asm _emit (vxd##_DEVICE_ID) & 0xff \
    _asm _emit ((vxd##_DEVICE_ID) >> 8) & 0xff

#define VMMCall(service) \
    VxDCall(VMM, service)

#define VxDJmp(vxd, service) \
    _asm _emit 0xcd \
    _asm _emit 0x20 \
    _asm _emit (GetVxDServiceOrdinal(service) & 0xff) \
    _asm _emit ((GetVxDServiceOrdinal(service) >> 8) & 0xff) | 0x80 \
    _asm _emit (vxd##_DEVICE_ID) & 0xff \
    _asm _emit ((vxd##_DEVICE_ID) >> 8) & 0xff

#define VMMJmp VxDJmp

/******************************************************************************
 *
 *		V M M	S E R V I C E S
 *
 ******************************************************************************/

/*XLATOFF*/
#define VMM_Service Declare_Service
#pragma warning (disable:4003)      // turn off not enough params warning
/*XLATON*/

/*MACROS*/
Begin_Service_Table(VMM)

VMM_Service (Get_VMM_Version, LOCAL)    // MUST REMAIN SERVICE 0!

VMM_Service (Get_Cur_VM_Handle)
VMM_Service (Test_Cur_VM_Handle)
VMM_Service (Get_Sys_VM_Handle)
VMM_Service (Test_Sys_VM_Handle)
VMM_Service (Validate_VM_Handle)

VMM_Service (Get_VMM_Reenter_Count)
VMM_Service (Begin_Reentrant_Execution)
VMM_Service (End_Reentrant_Execution)

VMM_Service (Install_V86_Break_Point)
VMM_Service (Remove_V86_Break_Point)
VMM_Service (Allocate_V86_Call_Back)
VMM_Service (Allocate_PM_Call_Back)

VMM_Service (Call_When_VM_Returns)

VMM_Service (Schedule_Global_Event)
VMM_Service (Schedule_VM_Event)
VMM_Service (Call_Global_Event)
VMM_Service (Call_VM_Event)
VMM_Service (Cancel_Global_Event)
VMM_Service (Cancel_VM_Event)
VMM_Service (Call_Priority_VM_Event)
VMM_Service (Cancel_Priority_VM_Event)

VMM_Service (Get_NMI_Handler_Addr)
VMM_Service (Set_NMI_Handler_Addr)
VMM_Service (Hook_NMI_Event)

VMM_Service (Call_When_VM_Ints_Enabled)
VMM_Service (Enable_VM_Ints)
VMM_Service (Disable_VM_Ints)

VMM_Service (Map_Flat)
VMM_Service (Map_Lin_To_VM_Addr)

//   Scheduler services

VMM_Service (Adjust_Exec_Priority)
VMM_Service (Begin_Critical_Section)
VMM_Service (End_Critical_Section)
VMM_Service (End_Crit_And_Suspend)
VMM_Service (Claim_Critical_Section)
VMM_Service (Release_Critical_Section)
VMM_Service (Call_When_Not_Critical)
VMM_Service (Create_Semaphore)
VMM_Service (Destroy_Semaphore)
VMM_Service (Wait_Semaphore)
VMM_Service (Signal_Semaphore)
VMM_Service (Get_Crit_Section_Status)
VMM_Service (Call_When_Task_Switched)
VMM_Service (Suspend_VM)
VMM_Service (Resume_VM)
VMM_Service (No_Fail_Resume_VM)
VMM_Service (Nuke_VM)
VMM_Service (Crash_Cur_VM)

VMM_Service (Get_Execution_Focus)
VMM_Service (Set_Execution_Focus)
VMM_Service (Get_Time_Slice_Priority)
VMM_Service (Set_Time_Slice_Priority)
VMM_Service (Get_Time_Slice_Granularity)
VMM_Service (Set_Time_Slice_Granularity)
VMM_Service (Get_Time_Slice_Info)
VMM_Service (Adjust_Execution_Time)
VMM_Service (Release_Time_Slice)
VMM_Service (Wake_Up_VM)
VMM_Service (Call_When_Idle)

VMM_Service (Get_Next_VM_Handle)

//   Time-out and system timer services

VMM_Service (Set_Global_Time_Out)
VMM_Service (Set_VM_Time_Out)
VMM_Service (Cancel_Time_Out)
VMM_Service (Get_System_Time)
VMM_Service (Get_VM_Exec_Time)

VMM_Service (Hook_V86_Int_Chain)
VMM_Service (Get_V86_Int_Vector)
VMM_Service (Set_V86_Int_Vector)
VMM_Service (Get_PM_Int_Vector)
VMM_Service (Set_PM_Int_Vector)

VMM_Service (Simulate_Int)
VMM_Service (Simulate_Iret)
VMM_Service (Simulate_Far_Call)
VMM_Service (Simulate_Far_Jmp)
VMM_Service (Simulate_Far_Ret)
VMM_Service (Simulate_Far_Ret_N)
VMM_Service (Build_Int_Stack_Frame)

VMM_Service (Simulate_Push)
VMM_Service (Simulate_Pop)

// Heap Manager

VMM_Service (_HeapAllocate)
VMM_Service (_HeapReAllocate)
VMM_Service (_HeapFree)
VMM_Service (_HeapGetSize)

/*ENDMACROS*/

/****************************************************
 *
 *   Flags for heap allocator calls
 *
 *   NOTE: HIGH 8 BITS (bits 24-31) are reserved
 *
 ***************************************************/

#define HEAPZEROINIT    0x00000001
#define HEAPZEROREINIT  0x00000002
#define HEAPNOCOPY  0x00000004
#define HEAPLOCKEDIFDP  0x00000100
#define HEAPSWAP    0x00000200
#define HEAPINIT        0x00000400
#define HEAPCLEAN   0x00000800

// Page Manager

/*MACROS*/
VMM_Service (_PageAllocate)
VMM_Service (_PageReAllocate)
VMM_Service (_PageFree)
VMM_Service (_PageLock)
VMM_Service (_PageUnLock)
VMM_Service (_PageGetSizeAddr)
VMM_Service (_PageGetAllocInfo)
VMM_Service (_GetFreePageCount)
VMM_Service (_GetSysPageCount)
VMM_Service (_GetVMPgCount)
VMM_Service (_MapIntoV86)
VMM_Service (_PhysIntoV86)
VMM_Service (_TestGlobalV86Mem)
VMM_Service (_ModifyPageBits)
VMM_Service (_CopyPageTable)
VMM_Service (_LinMapIntoV86)
VMM_Service (_LinPageLock)
VMM_Service (_LinPageUnLock)
VMM_Service (_SetResetV86Pageable)
VMM_Service (_GetV86PageableArray)
VMM_Service (_PageCheckLinRange)
VMM_Service (_PageOutDirtyPages)
VMM_Service (_PageDiscardPages)
/*ENDMACROS*/

/****************************************************
 *
 *  Flags for other page allocator calls
 *
 *  NOTE: HIGH 8 BITS (bits 24-31) are reserved
 *
 ***************************************************/

#define PAGEZEROINIT        0x00000001
#define PAGEUSEALIGN        0x00000002
#define PAGECONTIG      0x00000004
#define PAGEFIXED       0x00000008
#define PAGEDEBUGNULFAULT   0x00000010
#define PAGEZEROREINIT      0x00000020
#define PAGENOCOPY      0x00000040
#define PAGELOCKED      0x00000080
#define PAGELOCKEDIFDP      0x00000100
#define PAGESETV86PAGEABLE  0x00000200
#define PAGECLEARV86PAGEABLE    0x00000400
#define PAGESETV86INTSLOCKED    0x00000800
#define PAGECLEARV86INTSLOCKED  0x00001000
#define PAGEMARKPAGEOUT     0x00002000
#define PAGEPDPSETBASE      0x00004000
#define PAGEPDPCLEARBASE    0x00008000
#define PAGEDISCARD     0x00010000
#define PAGEPDPQUERYDIRTY   0x00020000
#define PAGEMAPFREEPHYSREG  0x00040000
#define PAGENOMOVE      0x10000000
#define PAGEMAPGLOBAL       0x40000000
#define PAGEMARKDIRTY       0x80000000

/****************************************************
 *
 *      Flags for _PhysIntoV86,
 *      _MapIntoV86, and _LinMapIntoV86
 *
 ***************************************************/

#define MAPV86_IGNOREWRAP       0x00000001


// Informational services

/*MACROS*/
VMM_Service (_GetNulPageHandle)
VMM_Service (_GetFirstV86Page)
VMM_Service (_MapPhysToLinear)
VMM_Service (_GetAppFlatDSAlias)
VMM_Service (_SelectorMapFlat)
VMM_Service (_GetDemandPageInfo)
VMM_Service (_GetSetPageOutCount)
/*ENDMACROS*/

/*
 *  Flags bits for _GetSetPageOutCount
 */
#define GSPOC_F_GET 0x00000001

// Device VM page manager

/*MACROS*/
VMM_Service (Hook_V86_Page)
VMM_Service (_Assign_Device_V86_Pages)
VMM_Service (_DeAssign_Device_V86_Pages)
VMM_Service (_Get_Device_V86_Pages_Array)
VMM_Service (MMGR_SetNULPageAddr)

// GDT/LDT management

VMM_Service (_Allocate_GDT_Selector)
VMM_Service (_Free_GDT_Selector)
VMM_Service (_Allocate_LDT_Selector)
VMM_Service (_Free_LDT_Selector)
VMM_Service (_BuildDescriptorDWORDs)
VMM_Service (_GetDescriptor)
VMM_Service (_SetDescriptor)
/*ENDMACROS*/

/*
 *  Flag equates for _BuildDescriptorDWORDs
 */
#define BDDEXPLICITDPL  0x00000001

/*
 *  Flag equates for _Allocate_LDT_Selector
 */
#define ALDTSPECSEL 0x00000001

/*MACROS*/
VMM_Service (_MMGR_Toggle_HMA)
/*ENDMACROS*/

/*
 *  Flag equates for _MMGR_Toggle_HMA
 */
#define MMGRHMAPHYSICAL 0x00000001
#define MMGRHMAENABLE   0x00000002
#define MMGRHMADISABLE  0x00000004
#define MMGRHMAQUERY    0x00000008

/*MACROS*/
VMM_Service (Get_Fault_Hook_Addrs)
VMM_Service (Hook_V86_Fault)
VMM_Service (Hook_PM_Fault)
VMM_Service (Hook_VMM_Fault)
VMM_Service (Begin_Nest_V86_Exec)
VMM_Service (Begin_Nest_Exec)
VMM_Service (Exec_Int)
VMM_Service (Resume_Exec)
VMM_Service (End_Nest_Exec)

VMM_Service (Allocate_PM_App_CB_Area, VMM_ICODE)
VMM_Service (Get_Cur_PM_App_CB)

VMM_Service (Set_V86_Exec_Mode)
VMM_Service (Set_PM_Exec_Mode)

VMM_Service (Begin_Use_Locked_PM_Stack)
VMM_Service (End_Use_Locked_PM_Stack)

VMM_Service (Save_Client_State)
VMM_Service (Restore_Client_State)

VMM_Service (Exec_VxD_Int)

VMM_Service (Hook_Device_Service)

VMM_Service (Hook_Device_V86_API)
VMM_Service (Hook_Device_PM_API)

VMM_Service (System_Control)

//   I/O and software interrupt hooks

VMM_Service (Simulate_IO)
VMM_Service (Install_Mult_IO_Handlers)
VMM_Service (Install_IO_Handler)
VMM_Service (Enable_Global_Trapping)
VMM_Service (Enable_Local_Trapping)
VMM_Service (Disable_Global_Trapping)
VMM_Service (Disable_Local_Trapping)

//   Linked List Abstract Data Type Services

VMM_Service (List_Create)
VMM_Service (List_Destroy)
VMM_Service (List_Allocate)
VMM_Service (List_Attach)
VMM_Service (List_Attach_Tail)
VMM_Service (List_Insert)
VMM_Service (List_Remove)
VMM_Service (List_Deallocate)
VMM_Service (List_Get_First)
VMM_Service (List_Get_Next)
VMM_Service (List_Remove_First)
/*ENDMACROS*/

/*
 *   Flags used by List_Create
 */
#define LF_ASYNC_BIT        0
#define LF_ASYNC        (1 << LF_ASYNC_BIT)
#define LF_USE_HEAP_BIT     1
#define LF_USE_HEAP     (1 << LF_USE_HEAP_BIT)
#define LF_ALLOC_ERROR_BIT  2
#define LF_ALLOC_ERROR      (1 << LF_ALLOC_ERROR_BIT)
/*
 * Swappable lists must use the heap.
 */
#define LF_SWAP         (LF_USE_HEAP + (1 << 3))

/******************************************************************************
 *  I N I T I A L I Z A T I O N   P R O C E D U R E S
 ******************************************************************************/

// Instance data manager

/*MACROS*/
VMM_Service (_AddInstanceItem)

// System structure data manager

VMM_Service (_Allocate_Device_CB_Area)
VMM_Service (_Allocate_Global_V86_Data_Area, VMM_ICODE)
VMM_Service (_Allocate_Temp_V86_Data_Area, VMM_ICODE)
VMM_Service (_Free_Temp_V86_Data_Area, VMM_ICODE)
/*ENDMACROS*/

/*
 *  Flag bits for _Allocate_Global_V86_Data_Area
 */
#define GVDAWordAlign       0x00000001
#define GVDADWordAlign      0x00000002
#define GVDAParaAlign       0x00000004
#define GVDAPageAlign       0x00000008
#define GVDAInstance        0x00000100
#define GVDAZeroInit        0x00000200
#define GVDAReclaim	    0x00000400
#define GVDAInquire	    0x00000800
#define GVDAHighSysCritOK   0x00001000
#define GVDAOptInstance     0x00002000
#define GVDAForceLow	    0x00004000

/*
 *  Flag bits for _Allocate_Temp_V86_Data_Area
 */
#define TVDANeedTilInitComplete 0x00000001

// Initialization information calls (win.ini and environment parameters)

/*MACROS*/
VMM_Service (Get_Profile_Decimal_Int, VMM_ICODE)
VMM_Service (Convert_Decimal_String, VMM_ICODE)
VMM_Service (Get_Profile_Fixed_Point, VMM_ICODE)
VMM_Service (Convert_Fixed_Point_String, VMM_ICODE)
VMM_Service (Get_Profile_Hex_Int, VMM_ICODE)
VMM_Service (Convert_Hex_String, VMM_ICODE)
VMM_Service (Get_Profile_Boolean, VMM_ICODE)
VMM_Service (Convert_Boolean_String, VMM_ICODE)
VMM_Service (Get_Profile_String, VMM_ICODE)
VMM_Service (Get_Next_Profile_String, VMM_ICODE)
VMM_Service (Get_Environment_String, VMM_ICODE)
VMM_Service (Get_Exec_Path, VMM_ICODE)
VMM_Service (Get_Config_Directory, VMM_ICODE)
VMM_Service (OpenFile, VMM_ICODE)
/*ENDMACROS*/

// OpenFile, if called after init, must point EDI to a buffer of at least
// this size.

#define VMM_OPENFILE_BUF_SIZE       260

/*MACROS*/
VMM_Service (Get_PSP_Segment, VMM_ICODE)
VMM_Service (GetDOSVectors, VMM_ICODE)
VMM_Service (Get_Machine_Info)
/*ENDMACROS*/

#define GMIF_80486_BIT  0x10
#define GMIF_80486  (1 << GMIF_80486_BIT)
#define GMIF_PCXT_BIT   0x11
#define GMIF_PCXT   (1 << GMIF_PCXT_BIT)
#define GMIF_MCA_BIT    0x12
#define GMIF_MCA    (1 << GMIF_MCA_BIT)
#define GMIF_EISA_BIT   0x13
#define GMIF_EISA   (1 << GMIF_EISA_BIT)
#define GMIF_CPUID_BIT  0x14
#define GMIF_CPUID  (1 << GMIF_CPUID_BIT)

// Following service is not restricted to initialization

/*MACROS*/
VMM_Service (GetSet_HMA_Info)
VMM_Service (Set_System_Exit_Code)

VMM_Service (Fatal_Error_Handler)
VMM_Service (Fatal_Memory_Error)

//   Called by VTD only

VMM_Service (Update_System_Clock)

/******************************************************************************
 *          D E B U G G I N G   E X T E R N S
 ******************************************************************************/

VMM_Service (Test_Debug_Installed)      // Valid call in retail also

VMM_Service (Out_Debug_String)
VMM_Service (Out_Debug_Chr)
VMM_Service (In_Debug_Chr)
VMM_Service (Debug_Convert_Hex_Binary)
VMM_Service (Debug_Convert_Hex_Decimal)

VMM_Service (Debug_Test_Valid_Handle)
VMM_Service (Validate_Client_Ptr)
VMM_Service (Test_Reenter)
VMM_Service (Queue_Debug_String)
VMM_Service (Log_Proc_Call)
VMM_Service (Debug_Test_Cur_VM)

VMM_Service (Get_PM_Int_Type)
VMM_Service (Set_PM_Int_Type)

VMM_Service (Get_Last_Updated_System_Time)
VMM_Service (Get_Last_Updated_VM_Exec_Time)

VMM_Service (Test_DBCS_Lead_Byte)       // for DBCS Enabling
/*ENDMACROS*/

/* ASM
.errnz  @@Test_DBCS_Lead_Byte - 100D1h   ; VMM service table changed above this service
*/

/*************************************************************************
 *************************************************************************
 * END OF 3.00 SERVICE TABLE MUST NOT SHUFFLE SERVICES BEFORE THIS POINT
 *  FOR COMPATIBILITY.
 *************************************************************************
 *************************************************************************/

/*MACROS*/
VMM_Service (_AddFreePhysPage, VMM_ICODE)
VMM_Service (_PageResetHandlePAddr)
VMM_Service (_SetLastV86Page, VMM_ICODE)
VMM_Service (_GetLastV86Page)
VMM_Service (_MapFreePhysReg)
VMM_Service (_UnmapFreePhysReg)
VMM_Service (_XchgFreePhysReg)
VMM_Service (_SetFreePhysRegCalBk, VMM_ICODE)
VMM_Service (Get_Next_Arena, VMM_ICODE)
VMM_Service (Get_Name_Of_Ugly_TSR, VMM_ICODE)
VMM_Service (Get_Debug_Options, VMM_ICODE)
/*ENDMACROS*/

/*
 *  Flags for AddFreePhysPage
 */
#define AFPP_SWAPOUT     0x0001 // physical memory that must be swapped out
                                // and subsequently restored at system exit
/*
 *  Flags for PageChangePager
 */
#define PCP_CHANGEPAGER     0x1 // change the pager for the page range
#define PCP_CHANGEPAGERDATA 0x2 // change the pager data dword for the pages
#define PCP_VIRGINONLY      0x4 // make the above changes to virgin pages only


/*
 *  Bits for the ECX return of Get_Next_Arena
 */
#define GNA_HIDOSLINKED  0x0002 // High DOS arenas linked when WIN386 started
#define GNA_ISHIGHDOS    0x0004 // High DOS arenas do exist

/*MACROS*/
VMM_Service (Set_Physical_HMA_Alias, VMM_ICODE)
VMM_Service (_GetGlblRng0V86IntBase, VMM_ICODE)
VMM_Service (_Add_Global_V86_Data_Area, VMM_ICODE)

VMM_Service (GetSetDetailedVMError)
/*ENDMACROS*/

/*
 *  Error code values for the GetSetDetailedVMError service. PLEASE NOTE
 *  that all of these error code values need to have bits set in the high
 *  word. This is to prevent collisions with other VMDOSAPP standard errors.
 *  Also, the low word must be non-zero.
 *
 *  First set of errors (high word = 0001) are intended to be used
 *  when a VM is CRASHED (VNE_Crashed or VNE_Nuked bit set on
 *  VM_Not_Executeable).
 *
 *  PLEASE NOTE that each of these errors (high word == 0001) actually
 *  has two forms:
 *
 *  0001xxxxh
 *  8001xxxxh
 *
 *  The device which sets the error initially always sets the error with
 *  the high bit CLEAR. The system will then optionally set the high bit
 *  depending on the result of the attempt to "nicely" crash the VM. This
 *  bit allows the system to tell the user whether the crash is likely or
 *  unlikely to destabalize the system.
 */
#define GSDVME_PRIVINST     0x00010001  /* Privledged instruction */
#define GSDVME_INVALINST    0x00010002  /* Invalid instruction */
#define GSDVME_INVALPGFLT   0x00010003  /* Invalid page fault */
#define GSDVME_INVALGPFLT   0x00010004  /* Invalid GP fault */
#define GSDVME_INVALFLT     0x00010005  /* Unspecified invalid fault */
#define GSDVME_USERNUKE     0x00010006  /* User requested NUKE of VM */
#define GSDVME_DEVNUKE      0x00010007  /* Device specific problem */
#define GSDVME_DEVNUKEHDWR  0x00010008  /* Device specific problem:
                         *   invalid hardware fiddling
                         *   by VM (invalid I/O)
                         */
#define GSDVME_NUKENOMSG    0x00010009  /* Supress standard messages:
                         *   SHELL_Message used for
                         *   custom msg.
                         */
#define GSDVME_OKNUKEMASK   0x80000000  /* "Nice nuke" bit */

/*
 *  Second set of errors (high word = 0002) are intended to be used
 *  when a VM start up is failed (VNE_CreateFail, VNE_CrInitFail, or
 *  VNE_InitFail bit set on VM_Not_Executeable).
 */
#define GSDVME_INSMEMV86    0x00020001  /* base V86 mem    - V86MMGR */
#define GSDVME_INSV86SPACE  0x00020002  /* Kb Req too large - V86MMGR */
#define GSDVME_INSMEMXMS    0x00020003  /* XMS Kb Req      - V86MMGR */
#define GSDVME_INSMEMEMS    0x00020004  /* EMS Kb Req      - V86MMGR */
#define GSDVME_INSMEMV86HI  0x00020005  /* Hi DOS V86 mem   - DOSMGR
                         *           V86MMGR
                         */
#define GSDVME_INSMEMVID    0x00020006  /* Base Video mem   - VDD */
#define GSDVME_INSMEMVM     0x00020007  /* Base VM mem     - VMM
                         *   CB, Inst Buffer
                         */
#define GSDVME_INSMEMDEV    0x00020008  /* Couldn't alloc base VM
                         * memory for device.
                         */
#define GSDVME_CRTNOMSG     0x00020009  /* Supress standard messages:
                         *   SHELL_Message used for
                         *   custom msg.
                         */

/*MACROS*/
VMM_Service (Is_Debug_Chr)

//   Mono_Out services

VMM_Service (Clear_Mono_Screen)
VMM_Service (Out_Mono_Chr)
VMM_Service (Out_Mono_String)
VMM_Service (Set_Mono_Cur_Pos)
VMM_Service (Get_Mono_Cur_Pos)
VMM_Service (Get_Mono_Chr)

//   Service locates a byte in ROM

VMM_Service (Locate_Byte_In_ROM, VMM_ICODE)

VMM_Service (Hook_Invalid_Page_Fault)
VMM_Service (Unhook_Invalid_Page_Fault)
/*ENDMACROS*/

/*
 *  Flag bits of IPF_Flags
 */
#define IPF_PGDIR   0x00000001  /* Page directory entry not-present */
#define IPF_V86PG   0x00000002  /* Unexpected not present Page in V86 */
#define IPF_V86PGH  0x00000004  /* Like IPF_V86PG at high linear */
#define IPF_INVTYP  0x00000008  /* page has invalid not present type */
#define IPF_PGERR   0x00000010  /* pageswap device failure */
#define IPF_REFLT   0x00000020  /* re-entrant page fault */
#define IPF_VMM     0x00000040  /* Page fault caused by a VxD */
#define IPF_PM      0x00000080  /* Page fault by VM in Prot Mode */
#define IPF_V86     0x00000100  /* Page fault by VM in V86 Mode */

/*MACROS*/
VMM_Service (Set_Delete_On_Exit_File)

VMM_Service (Close_VM)
/*ENDMACROS*/

/*
 *   Flags for Close_VM service
 */

#define CVF_CONTINUE_EXEC_BIT   0
#define CVF_CONTINUE_EXEC   (1 << CVF_CONTINUE_EXEC_BIT)

/*MACROS*/
VMM_Service (Enable_Touch_1st_Meg)      // Debugging only
VMM_Service (Disable_Touch_1st_Meg)     // Debugging only

VMM_Service (Install_Exception_Handler)
VMM_Service (Remove_Exception_Handler)

VMM_Service (Get_Crit_Status_No_Block)
/*ENDMACROS*/

/* ASM
; Check if VMM service table has changed above this service
.errnz   @@Get_Crit_Status_No_Block - 100F1h
*/

#ifdef WIN40SERVICES

/*************************************************************************
 *************************************************************************
 *
 * END OF 3.10 SERVICE TABLE MUST NOT SHUFFLE SERVICES BEFORE THIS POINT
 *  FOR COMPATIBILITY.
 *************************************************************************
 *************************************************************************/

/*MACROS*/
VMM_Service     (_GetLastUpdatedThreadExecTime)

VMM_Service (_Trace_Out_Service)
VMM_Service (_Debug_Out_Service)
VMM_Service (_Debug_Flags_Service)
/*ENDMACROS*/

#endif /* WIN40SERVICES */


/*
 *   Flags for _Debug_Flags_Service service.
 *
 *   Don't change these unless you really really know what you're doing.
 *   We need to define these even if we are in WIN31COMPAT mode.
 */

#define DFS_LOG_BIT         0
#define DFS_LOG             (1 << DFS_LOG_BIT)
#define DFS_PROFILE_BIT         1
#define DFS_PROFILE         (1 << DFS_PROFILE_BIT)
#define DFS_TEST_CLD_BIT        2
#define DFS_TEST_CLD            (1 << DFS_TEST_CLD_BIT)
#define DFS_NEVER_REENTER_BIT       3
#define DFS_NEVER_REENTER       (1 << DFS_NEVER_REENTER_BIT)
#define DFS_TEST_REENTER_BIT        4
#define DFS_TEST_REENTER        (1 << DFS_TEST_REENTER_BIT)
#define DFS_NOT_SWAPPING_BIT        5
#define DFS_NOT_SWAPPING        (1 << DFS_NOT_SWAPPING_BIT)
#define DFS_TEST_BLOCK_BIT      6
#define DFS_TEST_BLOCK          (1 << DFS_TEST_BLOCK_BIT)

#define DFS_RARE_SERVICES	0xFFFFFF80

#define DFS_EXIT_NOBLOCK        (DFS_RARE_SERVICES+0)
#define DFS_ENTER_NOBLOCK       (DFS_RARE_SERVICES+DFS_TEST_BLOCK)

#define DFS_TEST_NEST_EXEC	(DFS_RARE_SERVICES+1)

#ifdef WIN40SERVICES

/*MACROS*/
VMM_Service (VMMAddImportModuleName)

VMM_Service (VMM_Add_DDB)
VMM_Service (VMM_Remove_DDB)

VMM_Service     (Test_VM_Ints_Enabled)
VMM_Service     (_BlockOnID)

VMM_Service     (Schedule_Thread_Event)
VMM_Service (Cancel_Thread_Event)
VMM_Service (Set_Thread_Time_Out)
VMM_Service (Set_Async_Time_Out)

VMM_Service (_AllocateThreadDataSlot)
VMM_Service (_FreeThreadDataSlot)
/*ENDMACROS*/

/*
 *  Flag equates for _CreateMutex
 */
#define         MUTEX_MUST_COMPLETE         1L

/*MACROS*/
VMM_Service     (_CreateMutex)

VMM_Service     (_DestroyMutex)
VMM_Service     (_GetMutexOwner)
VMM_Service (Call_When_Thread_Switched)

VMM_Service     (VMMCreateThread)
VMM_Service     (_GetThreadExecTime)
VMM_Service     (VMMTerminateThread)

VMM_Service     (Get_Cur_Thread_Handle)
VMM_Service     (Test_Cur_Thread_Handle)
VMM_Service     (Get_Sys_Thread_Handle)
VMM_Service     (Test_Sys_Thread_Handle)
VMM_Service     (Validate_Thread_Handle)
VMM_Service     (Get_Initial_Thread_Handle)
VMM_Service     (Test_Initial_Thread_Handle)
VMM_Service     (Debug_Test_Valid_Thread_Handle)
VMM_Service (Debug_Test_Cur_Thread)

VMM_Service (VMM_GetSystemInitState)

VMM_Service     (Cancel_Call_When_Thread_Switched)
VMM_Service     (Get_Next_Thread_Handle)
VMM_Service     (Adjust_Thread_Exec_Priority)

VMM_Service (_Deallocate_Device_CB_Area)
VMM_Service (Remove_IO_Handler)
VMM_Service (Remove_Mult_IO_Handlers)
VMM_Service (Unhook_V86_Int_Chain)
VMM_Service (Unhook_V86_Fault)
VMM_Service (Unhook_PM_Fault)
VMM_Service (Unhook_VMM_Fault)
VMM_Service (Unhook_Device_Service)

VMM_Service (_PageReserve)
VMM_Service (_PageCommit)
VMM_Service (_PageDecommit)
VMM_Service (_PagerRegister)
VMM_Service (_PagerQuery)
VMM_Service (_PagerDeregister)
VMM_Service (_ContextCreate)
VMM_Service (_ContextDestroy)
VMM_Service (_PageAttach)
VMM_Service (_PageFlush)
VMM_Service     (_SignalID)
VMM_Service (_PageCommitPhys)

VMM_Service (_Register_Win32_Services)

VMM_Service (Cancel_Call_When_Not_Critical)
VMM_Service (Cancel_Call_When_Idle)
VMM_Service (Cancel_Call_When_Task_Switched)

VMM_Service (_Debug_Printf_Service)
VMM_Service     (_EnterMutex)
VMM_Service     (_LeaveMutex)
VMM_Service     (Simulate_VM_IO)
VMM_Service     (Signal_Semaphore_No_Switch)

VMM_Service (_ContextSwitch)
VMM_Service (_PageModifyPermissions)
VMM_Service (_PageQuery)

VMM_Service     (_EnterMustComplete)
VMM_Service     (_LeaveMustComplete)
VMM_Service     (_ResumeExecMustComplete)
/*ENDMACROS*/

/*
 *  Flag equates for _GetThreadTerminationStatus
 */
#define THREAD_TERM_STATUS_CRASH_PEND       1L
#define THREAD_TERM_STATUS_NUKE_PEND        2L
#define THREAD_TERM_STATUS_SUSPEND_PEND     4L

/*MACROS*/
VMM_Service     (_GetThreadTerminationStatus)
VMM_Service     (_GetInstanceInfo)
/*ENDMACROS*/

/*
 *  Return values for _GetInstanceInfo
 */
#define INSTINFO_NONE   0       /* no data instanced in range */
#define INSTINFO_SOME   1       /* some data instanced in range */
#define INSTINFO_ALL    2       /* all data instanced in range */

/*MACROS*/
VMM_Service     (_ExecIntMustComplete)
VMM_Service (_ExecVxDIntMustComplete)

VMM_Service (Begin_V86_Serialization)

VMM_Service (Unhook_V86_Page)
VMM_Service (VMM_GetVxDLocationList)
VMM_Service (VMM_GetDDBList)
VMM_Service (Unhook_NMI_Event)

VMM_Service (Get_Instanced_V86_Int_Vector)
VMM_Service (Get_Set_Real_DOS_PSP)
/*ENDMACROS*/

#define GSRDP_Set   0x0001

/*MACROS*/
VMM_Service     (Call_Priority_Thread_Event)
VMM_Service     (Get_System_Time_Address)
VMM_Service (Get_Crit_Status_Thread)

VMM_Service (Get_DDB)
VMM_Service (Directed_Sys_Control)
/*ENDMACROS*/

// Registry APIs for VxDs
/*MACROS*/
VMM_Service (_RegOpenKey)
VMM_Service (_RegCloseKey)
VMM_Service (_RegCreateKey)
VMM_Service (_RegDeleteKey)
VMM_Service (_RegEnumKey)
VMM_Service (_RegQueryValue)
VMM_Service (_RegSetValue)
VMM_Service (_RegDeleteValue)
VMM_Service (_RegEnumValue)
VMM_Service (_RegQueryValueEx)
VMM_Service (_RegSetValueEx)
/*ENDMACROS*/

#ifndef REG_SZ      // define only if not there already

#define REG_SZ      0x0001
#define REG_BINARY  0x0003

#endif

#ifndef HKEY_LOCAL_MACHINE  // define only if not there already

#define HKEY_CLASSES_ROOT       0x80000000
#define HKEY_CURRENT_USER       0x80000001
#define HKEY_LOCAL_MACHINE      0x80000002
#define HKEY_USERS          0x80000003
#define HKEY_PERFORMANCE_DATA       0x80000004
#define HKEY_CURRENT_CONFIG     0x80000005
#define HKEY_DYN_DATA       0x80000006

#endif

/*MACROS*/
VMM_Service (_CallRing3)
VMM_Service (Exec_PM_Int)
VMM_Service (_RegFlushKey)
VMM_Service (_PageCommitContig)
VMM_Service (_GetCurrentContext)

VMM_Service     (_LocalizeSprintf)
VMM_Service     (_LocalizeStackSprintf)

VMM_Service (Call_Restricted_Event)
VMM_Service (Cancel_Restricted_Event)

VMM_Service (Register_PEF_Provider, VMM_ICODE)

VMM_Service     (_GetPhysPageInfo)

VMM_Service (_RegQueryInfoKey)
VMM_Service     (MemArb_Reserve_Pages)
/*ENDMACROS*/

/*
 *  Return values for _GetPhysPageInfo
 */
#define PHYSINFO_NONE   0       /* no pages in the specified range exist */
#define PHYSINFO_SOME   1       /* some pages in the specified range exist */
#define PHYSINFO_ALL    2       /* all pages in the specified range exist */

// New timeslicer services
/*MACROS*/
VMM_Service     (Time_Slice_Sys_VM_Idle)
VMM_Service     (Time_Slice_Sleep)
VMM_Service     (Boost_With_Decay)
VMM_Service     (Set_Inversion_Pri)
VMM_Service     (Reset_Inversion_Pri)
VMM_Service     (Release_Inversion_Pri)
VMM_Service     (Get_Thread_Win32_Pri)
VMM_Service     (Set_Thread_Win32_Pri)
VMM_Service     (Set_Thread_Static_Boost)
VMM_Service     (Set_VM_Static_Boost)
VMM_Service     (Release_Inversion_Pri_ID)
VMM_Service     (Attach_Thread_To_Group)
VMM_Service     (Detach_Thread_From_Group)
VMM_Service     (Set_Group_Static_Boost)

VMM_Service (_GetRegistryPath, VMM_ICODE)
VMM_Service (_GetRegistryKey)
/*ENDMACROS*/

// TYPE definitions for _GetRegistryKey

#define REGTYPE_ENUM    0
#define REGTYPE_CLASS   1
#define REGTYPE_VXD     2

// Flag definitions for _GetRegistryKey
#define REGKEY_OPEN                 0
#define REGKEY_CREATE_IFNOTEXIST    1

// Flag definitions for _Assert_Range
#define ASSERT_RANGE_NULL_BAD   0x00000000
#define ASSERT_RANGE_NULL_OK    0x00000001
#define ASSERT_RANGE_NO_DEBUG   0x80000000
#define ASSERT_RANGE_BITS       0x80000001

/*MACROS*/
VMM_Service (Cleanup_Thread_State)
VMM_Service (_RegRemapPreDefKey)
VMM_Service (End_V86_Serialization)
VMM_Service (_Assert_Range)
VMM_Service (_Sprintf)
VMM_Service (_PageChangePager)
VMM_Service (_RegCreateDynKey)
VMM_Service (_RegQMulti)

// Additional timeslicer services
VMM_Service     (Boost_Thread_With_VM)
/*ENDMACROS*/

// Flag definitions for Get_Boot_Flags

#define BOOT_CLEAN              0x00000001
#define BOOT_DOSCLEAN           0x00000002
#define BOOT_NETCLEAN           0x00000004
#define BOOT_INTERACTIVE        0x00000008

/*MACROS*/
VMM_Service     (Get_Boot_Flags)
VMM_Service     (Set_Boot_Flags)

// String and memory services
VMM_Service (_lstrcpyn)
VMM_Service (_lstrlen)
VMM_Service (_lmemcpy)

VMM_Service     (_GetVxDName)

// For vwin32 use only
VMM_Service (Force_Mutexes_Free)
VMM_Service (Restore_Forced_Mutexes)
/*ENDMACROS*/

// Reclaimable low memory services
/*MACROS*/
VMM_Service     (_AddReclaimableItem)
VMM_Service     (_SetReclaimableItem)
VMM_Service     (_EnumReclaimableItem)
/*ENDMACROS*/

// completely wake sys VM from idle state
/*MACROS*/
VMM_Service	(Time_Slice_Wake_Sys_VM)
VMM_Service	(VMM_Replace_Global_Environment)
VMM_Service	(Begin_Non_Serial_Nest_V86_Exec)
VMM_Service	(Get_Nest_Exec_Status)
/*ENDMACROS*/

// Bootlogging services

/*MACROS*/
VMM_Service	(Open_Boot_Log)
VMM_Service	(Write_Boot_Log)
VMM_Service	(Close_Boot_Log)
VMM_Service	(EnableDisable_Boot_Log)
/*ENDMACROS*/

#endif /* WIN40SERVICES */

/*MACROS*/
End_Service_Table(VMM)
/*ENDMACROS*/

/******************************************************************************
 *
 *		End of VMM Services
 *
 ******************************************************************************/


/******************************************************************************
 *              PAGE TABLE EQUATES
 *****************************************************************************/


#define P_SIZE      0x1000      /* page size */

/******************************************************************************
 *
 *              PAGE TABLE ENTRY BITS
 *
 *****************************************************************************/

#define P_PRESBIT   0
#define P_PRES      (1 << P_PRESBIT)
#define P_WRITEBIT  1
#define P_WRITE     (1 << P_WRITEBIT)
#define P_USERBIT   2
#define P_USER      (1 << P_USERBIT)
#define P_ACCBIT    5
#define P_ACC       (1 << P_ACCBIT)
#define P_DIRTYBIT  6
#define P_DIRTY     (1 << P_DIRTYBIT)

#define P_AVAIL     (P_PRES+P_WRITE+P_USER) /* avail to user & present */


#endif	// _VMM16_H
