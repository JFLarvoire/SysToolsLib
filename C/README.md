SysToolsLib C tools
===================

This folder contains the sources for rebuilding the SysToolsLib C tools.

It contains the following subfolders:

Folder		| Content
--------------- | -----------------------------------------------------
MsvcLibX	| Microsoft Visual C++ runtime library extensions
SysLib		| Cross-OS system management library
SRC		| Sources of the SysToolsLib C tools

For detailed information about each component, refer to the README.md file in each folder.


Quick Guide for rebuilding everything in Windows
------------------------------------------------

1. Install Microsoft Visual C++ if you don't have it already.  
   If needed, it's part of the free Visual Studio Community Edition, available from this URL:  
   https://www.visualstudio.com/downloads/  
   Important: While installing Visual Studio Community Edition, make sure to select the following optional components:

    - The workload "Desktop Development with C++"
    - Options "C++/CLI support" and "Standard Library modules" (In the list at the right of the installation wizard)

2. Optional. Install the other compilers and SDKs for DOS and Windows that some tools depend on. See details further down.  
   Note: Most tools will build correctly without them, with just some features missing.

3. Download the whole SysToolsLib C folder contents into a %WORKDIR% directory.

4. Rebuild everything

        cd %WORKDIR%
        configure
        make

Note:

   - The configure.bat script needs to be run only once, the first time a build is done.
   - It must only be run again if other versions of the build tools (C compiler, etc) are installed, including
     the optional ones listed below.
   - Before running make.bat, verify in the configure.bat output that it correctly detected the location of your
     C compiler (CC) and Windows Software Development Kit (WINSDK).

### Individual components can be built separately if desired

1. Rebuild the MsvcLibX library.

        cd %WORKDIR%\MsvcLibX\src
        configure
        make

2. Rebuild the SysLib library

        cd %WORKDIR%\SysLib
        configure
        make

3. Rebuild all C tools.

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


Optional compilers and SDKs for DOS and Windows
-----------------------------------------------

After installing any of these tools, run configure.bat.  
This will update the config.HOSTNAME.bat file in each library directory.  
Subsequent builds with make.bat will automatically use the new tools and SDKs, and build the programs that depend on them.

- If you're interested in building Windows 95/98 tools, install Microsoft Visual 2005.  
  It is still available for MSDN subscribers in 2017.  
  It can be installed in parallel with more recent versions of Visual Studio.

- If you're interested in building BIOS and MS-DOS tools, install Microsoft Visual C++ 1.52.  
  It is still available for MSDN subscribers in 2016, as part of the Visual Studio 2005 DVD image, but not installed by default.  
  Gotcha: The setup program is a WIN16 program, which requires extra steps for installing it in modern versions of Windows:
  
   - Install a VM with Windows XP, that can run WIN16 programs. (This has to be an x86 VM, not an amd64 VM with XP/64)
   - Give that VM access to the host's file system.
   - Run Visual C++ 1.52 setup, and install it in the VM's C:\MSVC. (Necessary so that the setup builds vcvars.bat correctly.)
   - Once this is done, move the VM's C:\MSVC to the host's C:\MSVC. (vcvars.bat will thus refer to the host's C drive.)

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

