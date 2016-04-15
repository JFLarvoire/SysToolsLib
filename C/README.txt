This folder contains the sources for rebuilding the SysToolsLib C tools.

It contains the following subfolders:

- MsvcLibX	Microsoft Visual C++ runtime library extensions
- SRC		Sources of the SysToolsLib C tools

For detailed information about each component, refer to the README.txt file in each folder.


Quick Guide for rebuilding everything in Windows
________________________________________________

0) Install Microsoft Visual C++ if you don't have it already.
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

1) Download the above folders into a %WORKDIR% directory.

2) Rebuild the MsvcLibX library.

cd %WORKDIR%\MsvcLibX\src
configure
make

3) Rebuild all tools.

cd %WORKDIR%\SRC
configure
make


Quick guide for rebuilding everything in Linux
______________________________________________

1) Download both the MsvcLibX and SRC folders into a $WORKDIR directory.

2) Go to the SRC directory and rebuild all tools.

cd $WORKDIR/SRC
chmod +x configure
./configure
make
