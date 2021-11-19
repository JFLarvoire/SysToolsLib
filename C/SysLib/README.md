System Library
==============


Introduction
------------

This library contains system management routines that have no equivalent in the standard C library.  
It hides OS-specific implementations behind a common API, so that the same source can be compiled for
all operating systems.

Currently this System Library supports MS-DOS, Windows 95/98, and Windows NT/.../10.  
Note that as far as system management is concerned, the 95 and NT families are two completely distinct Operating Systems.

It does not yet support Linux, but it should be relatively straightforward to add Linux versions of each routine.
So in combination with the MsvcLibX library, it should be possible to build powerful system management tools,
with the same source for DOS, all flavors of Windows, and Linux.


Categories of functions
-----------------------

The available functions can be grouped in a number of families:

Category          | Description
------------------|---------------------------------------------------------------------------------------------------
64-bits QWORDs    | Manipulate 64-bit unsigned integers, even in 16-bits programs.
Object printf     | Print formatted C++ objects, as conveniently as C's printf(), with format strings similar to C#'s.
SM-BIOS           | Access System Management BIOS tables.
UUID              | UUID/GUID management routines. (Create; Compare; Format; Get the PC UUID; etc)
Block device I/O  | I/O to hard disks, logical volumes, files, and eventually physical floppies and CDs.
Hard disk I/O     | I/O to hard disk sectors.
Logical disk I/O  | I/O to logical volumes visible to the operating system.
Boot sector       | Manage boot sectors.
GPT               | Manage entries in EFI GPT partition tables.
NetBIOS           | Send/Receive NetBIOS packets; Get MAC address.
Strings           | Case-insentitive search and replace.

See <System%20Library%20Reference.htm> for a description of the architecture, and the main function calls.


Building the SysLib library
---------------------------

- Preliminary steps: Install required software:
   - Install and build the [MsvcLibX library](./MsvcLibX) in "%MY_LIBS_DIR%\MsvcLibX\".
   - Install the include files from the [gnu-efi library](https://sourceforge.net/projects/gnu-efi/)
     into "%MY_LIBS_DIR%\gnu-efi\inc\", and optionally patch them as explained further down.  
     (Necessary for UUID and GPT management support, and patching necessary only for 16-bits programs support.)  
     (It's not necessary to install the rest of the gnu-efi sources, nor to build the gnu-efi library.)
   - Optionally install the Windows 98 DDK. See explanations further down.  
     (Only necessary to rebuild support for Windows 95/98 block device I/O, etc.)
   - Optionally install the LAN Manager 2.1 Programmer's ToolKit. See explanations further down.  
     (Only necessary to rebuild support for NetBIOS and MAC address for DOS programs.)
- Extract the SysLib library sources into another directory at the same level. Ex: %MY_LIBS_DIR%\SysLib\.
- Open a cmd window, and run:

      cd "%MY_LIBS_DIR%\SysLib"
      configure
      make

Note: configure.bat will output a warning about the optional libraries that it cannot find.
You may safely ignore that warning.

For details about the configure.bat and make.bat scripts, see the MsvcLibX library README files.


gnu-efi patches for 16-bits compiler support
--------------------------------------------

If you want to build the SysLib library for MS-DOS with GPT support, it's necessary to make the following changes.  
All these add a conditional compilation around union definitions that are incompatible with the MSVC 1.5 C++ compiler:
       
    #ifndef _MSDOS      /* Prevent errors when compiling as C++ in MSVC 1.5 */
    [Code incompatible with the MSVC 1.5 C++ compiler]
    #endif       /* !defined(_MSDOS) */

* gnu-efi\inc\efidevp.h: Add a conditional compilation around union definitions in the end (about lines 473 to the end):

      #ifndef _MSDOS  /* Prevent errors when compiling as C++ in MSVC 1.5 */
      
      typedef union {
      [...]
      } EFI_DEV_PATH;
      
      typedef union {
      [...]
      } EFI_DEV_PATH_PTR;
      
      #endif   /* !defined(_MSDOS) */

* gnu-efi\inc\efiapi.h: Add a conditional compilation around union definitions, about lines 582 to 588):

      #ifndef _MSDOS  /* Prevent errors when compiling as C++ in MSVC 1.5 */
      
      typedef struct {
      [...]
      } EFI_CAPSULE_BLOCK_DESCRIPTOR;
      
      #endif   /* !defined(_MSDOS) */
       
* gnu-efi\inc\efilib.h: Add a conditional compilation around the following definitions (About lines 763 to 783):

      #ifndef _MSDOS  /* Prevent errors when compiling as C++ in MSVC 1.5 */
      
      typedef union {
      [...]
      } EFI_PCI_ADDRESS_UNION;
      
      [...]
      
      EFI_STATUS
      PciFindDevice (
      [...]
      );
      
      #endif   /* !defined(_MSDOS) */


Windows 98 DDK installation
---------------------------

The Windows 98 DDK is required only for building block I/O routines for Windows 95 and 98.

The Windows 98 DDK is available to Microsoft MSDN subscribers in the
[MSDN downloads](https://msdn.microsoft.com/en-us/subscriptions/downloads/) area.

The setup is a WIN16 program, so it must be run in a 32-bits Windows XP VM. (The last version supporting WIN16.)  
Run that setup.exe.  
In addition to the default build environment, select the optional files:
"Legacy and Related Driver Samples" / "Storage Driver Samples"  
Finally copy the 98DDK directory contents from the VM to your development PC, into "%MY_LIBS_DIR%\98DDK\".

Then make the following changes:

* 98DDK\inc\win98\vmm.h: Allow using VxD C services that return a value in EAX:

  Change

      typedef (_cdecl * VXD_C_SERVICE)();

  To

      typedef int (_cdecl * VXD_C_SERVICE)(); /* Some VxD C services return a value in EAX */

* 98DDK\src\block\inc\dcb.h: Put it with the other include files, then rename a structure to avoid a conflict:

  Copy that file to 98DDK\inc\win98\, then, in the copy, change:  

      typedef struct  _DCB { /* */
      [...]
      } DCB, *PDCB;

  to

      typedef struct  _IOSDCB { /* Renamed DCB as IOSDCB, to avoid conflict with WIN32's winbase.h own DCB structure. */
      [...]
      } IOSDCB, *PIOSDCB;



LAN Manager 2.1 Programmer's ToolKit installation
-------------------------------------------------

This tool kit allows to write programs interfacing with the MS-DOS network layer, called LAN Manager.

I got it on the "Microsoft Platform Archive" disk #2 "16-bit SDKs and Tools", in directory \LMPTK.

It's still available for download for MSDN subscribers in the
[MSDN downloads](https://msdn.microsoft.com/en-us/subscriptions/downloads/) area, in
16-bit SDK and DDKs / 16-bit SDKs and Tools Part 2 (English)

Copy all files and subdirectories from \LMPTK\DISK1, DISK2, and DISK3, and merge them into "%MY_LIBS_DIR%\LMPTK\".  
(We only need those from DISK1, but the other files and samples might be interesting.)


Known issues
------------

I had not tested the Windows 95/98 code for a long long time, since about year 2002, when I last used a Windows 98 PC.  
The code used to work reliably, and I had not changed it since then, so I expected it to still work fine.  
Yet one quick attempt in early 2017 to test raw hard disk I/O in a VM with Windows 98 failed, with a protection error.  
After investigating the issue, it seems that the root cause is in the routine that switches from WIN32 ring 3 mode
to ring 0 mode, for doing VxD calls. This routine starts by doing an SGDT instruction to get the GDT base.  
Unfortunately the SGDT instruction is _not_ privileged, and when run in a VM, it's the host OS GDT base that's returned,
and not the guest OS's.  
In july 2017, I rewrote the Win95/98 raw hard disk I/O to use DPMI to execute Virtual 86 BIOS calls.  
This works fine, but I expect the performance to be poor compared to the old implementation.  
Any help at fixing the ring0.c module, so that it can work in a VM, is welcome. That is, how to get the guest OS GDT base?


To do
-----

* Port additional routines to Linux, like block I/O.
* In 2017 I added support for block I/O to floppys, an idea that had been waiting for 20 years.
  The obvious next step is block I/O to CD/DVD/BR.  
  This would allow writing a tool for creating floppy images from floppys, or .iso files from CDs.
* The QWORD support can be improved, to add other operators, and make its usage even more seamless in 16-bits programs.
* The oprintf routine can be improved. See the to do list in the oprintf.cpp header.
* The SMBIOS routines for DOS could be made smaller by accessing the tables in ROM istead of a copy in RAM.


Copyright
---------

All these files are Copyright (c) Hewlett-Packard Enterprise, and released under the Apache 2.0 license,
except the efi*.h include files, which are based on the gnu-efi ia32, Copyright (c) 1998 Intel Corporation,
and released under the BSD license.

