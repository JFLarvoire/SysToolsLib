SysToolsLib C tools
===================

This folder contains the sources for rebuilding the SysToolsLib C tools.

It contains the following subfolders:

Folder		| Content
--------------- | -----------------------------------------------------
MsvcLibX	| Microsoft Visual C++ runtime library extensions
SysLib		| Cross-OS system management library
SRC		| Sources of the SysToolsLib C tools

For detailed information about each component, refer to the README.txt file in each folder.


Quick Guide for rebuilding everything in Windows
------------------------------------------------

1. Install Microsoft Visual C++ if you don't have it already.  
   If needed, it's part of the free Visual Studio Express, available from this URL:  
   https://www.visualstudio.com/en-us/products/visual-studio-express-vs.aspx  
   Important: After installing Visual Studio Express Community Edition, make sure to install the optional C++ components:

    - Start Visual Studio
    - Click New project
    - Select Visual C++
    - Double click "Install Visual C++ Tools for Windows Desktop"
    - Click OK  
    ...
    - Make sure both "Common Tools for Visual C++ 2015" and "Windows 8.1 SDK and Universal CRT SDK" are selected.  
    ...

2. Download the whole SysToolsLib C folder contents into a %WORKDIR% directory.

3. Rebuild everything

        cd %WORKDIR%
        configure
        make

Note:

   - The configure.bat script needs to be run only once, the first time a build is done.
   - It must only be run again if other versions of the build tools (C compiler, etc) are installed.
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

