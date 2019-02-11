SysToolsLib C tools
===================

This folder contains the sources for rebuilding the SysToolsLib C tools.

It contains the following subfolders:

Folder		| Content
--------------- | -----------------------------------------------------
BiosLib         | Routines for writing C programs running in the legacy BIOS.
LoDosLib        | Routines for writing MS-DOS drivers and TSRs.
MsvcLibX	| Microsoft Visual C++ runtime library extensions
PModeLib        | Routines for managing the x86 processors protected mode.
SysLib		| Cross-OS system management library
SRC		| Sources of the SysToolsLib C tools

For detailed information about each component, refer to the README.md file in each folder.

For a list of all available tools, see [Catalog.md](../Docs/Catalog.md).


Quick Guide for rebuilding everything in Windows
------------------------------------------------

1. Install Microsoft Visual C++ if you don't have it already.  
   If needed, it's part of the free Visual Studio Community Edition, available from this URL:  
   https://www.visualstudio.com/downloads/  
   Important: While installing Visual Studio Community Edition, make sure to select the following optional components:

    - The workload "Desktop Development with C++"
    - Options "C++/CLI support" and "Standard Library modules" (In the list at the right of the installation wizard)

2. Optional. Install the other compilers and SDKs for DOS and Windows that some tools depend on. See details further down.  
   Note: All tools will build correctly without these optional compilers and SDKs, with just some features missing.

3. Download the whole SysToolsLib C folder contents into a %WORKDIR% directory.

4. Rebuild everything

        cd %WORKDIR%
        configure
        make

Note:

- The configure.bat script needs to be run only once, the first time a build is done.  
- Before running make.bat, verify in the configure.bat output that it correctly detected the location of your
  C compiler (CC) and Windows Software Development Kit (WINSDK).
- Configure.bat must only be run again if other versions of the build tools (C compiler, etc) are installed,
  including the optional ones listed below, or if some of the build tools or libraries have been moved to another
  directory.

### Individual components can be built separately if desired

If you have a C compiler for DOS:
 
1. Rebuild the BiosLib library.

        cd %WORKDIR%\BiosLib
        configure
        make

2. Rebuild the loDosLib library.

        cd %WORKDIR%\loDosLib
        configure
        make

3. Rebuild the PModeLib library.

        cd %WORKDIR%\PModeLib
        configure
        make

In all cases:

4. Rebuild the MsvcLibX library.

        cd %WORKDIR%\MsvcLibX\src
        configure
        make

5. Rebuild the SysLib library.

        cd %WORKDIR%\SysLib
        configure
        make

6. Rebuild all C tools.

        cd %WORKDIR%\SRC
        configure
        make


Quick guide for rebuilding everything in Linux
----------------------------------------------

1. Download the whole SysToolsLib C folder contents into a $WORKDIR directory.

2. Rebuild everything

        cd $WORKDIR
        make

### Individual components can be built separately if desired

1. Rebuild the SysLib library

        cd $WORKDIR/SysLib
        make

2. Rebuild all C tools.

        cd $WORKDIR/SRC
        make

Note: The other components (BiosLib/LoDos/Lib/PModeLib/MsvcLibX) are for DOS or Windows only.


Optional compilers and SDKs for DOS and Windows
-----------------------------------------------

After installing any of these tools, run configure.bat in the base %WORKDIR%.  
This will update the config.HOSTNAME.bat file in each library directory.  
Subsequent builds with make.bat will automatically use the new tools and SDKs, and build the programs that depend on them.

- If you're interested in building Windows 95/98 tools, install Microsoft Visual 2005.  
  It is still available for MSDN subscribers in 2017.  
  It can be installed in parallel with more recent versions of Visual Studio.

- If you're interested in building BIOS and MS-DOS tools, install Microsoft Visual C++ 1.52c.  
  It is still available for MSDN subscribers in 2017, as part of the Visual Studio 2005 DVD image, but not installed by default.  
  Gotcha: The VC++ 1.52 compiler is a WIN32 program that runs in all 32 and 64-bits versions of Windows. But
  unfortunately the VC++ 1.52 setup.exe program is a WIN16 program, which only runs on old 32-bits versions of Windows.
  This requires doing extra steps for successfully installing the compiler in modern 64-bits versions of Windows:
  
   - Install a 32-bits VM running Windows XP, that can run WIN16 programs out-of-the-box. (This has to be an x86 VM, not an amd64 VM with XP/64)  
     Note: Newer 32-bits x86 versions of Windows can still run WIN16 programs, but this may require some tinkering.
     If needed, look for instructions on the Internet.
   - Give that VM access to the host's file system.
   - Run the VC++ 1.52 setup in the VM, and install it in the VM's C:\MSVC. (Necessary so that the setup builds vcvars.bat correctly.)
   - Once this is done, copy the VM's C:\MSVC to the host's C:\MSVC. (vcvars.bat will thus refer to the host's C drive.)

- The UUID and GPT management tools depend on the gnu-efi sources.  
  Install the include files from the [gnu-efi library](https://sourceforge.net/projects/gnu-efi/)  
  into "%MY_LIBS_DIR%\gnu-efi\inc\", and optionally patch them as explained in the [SysLib/README.md](SysLib/README.md) file.  
  (The patching is necessary only for 16-bits programs support.)  
  (It's not necessary to install the rest of the gnu-efi sources, nor to build the gnu-efi library.)

- The Win95 version of some tools requires the Windows 98 DDK. See explanations in the [SysLib/README.md](SysLib/README.md) file.  
  (Only necessary to rebuild support for Windows 95/98 block device I/O, etc.)

- The DOS version of some tools requires the LAN Manager 2.1 Programmer's ToolKit.
  See explanations in the [SysLib/README.md](SysLib/README.md) file.  
  (Only necessary to rebuild support for NetBIOS and MAC address for DOS programs.)

- The tools can be built for ARM and ARM64. This has been verified to work with Visual Studio 2019 Preview.
  But the tools themselves have not been tested, for lack of a test system running an ARM or ARM64 version of Windows.  
  To build for these targets, run for example:
  
        cd %WORKDIR%
        configure
        :# Assuming that configure found both the ARM and ARM64 compilers
        make "OS=ARM ARM64"


Procedure for generating a new release
--------------------------------------

        cd %WORKDIR%
        configure
        make
        make release
