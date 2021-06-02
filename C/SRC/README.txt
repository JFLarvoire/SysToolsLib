		SysToolsLib C/C++ system management tools



Introduction
____________

This collection of system management tools was built upon 30 years of work.
It started as a number of small ad-hoc tools for managing MS-DOS files, and
more generally the DOS PCs configuration.
Then we got OS/2 servers, and I ported some of these tools to run under OS/2.
Later on these servers were replaced by Windows NT servers, and I had to port
these tools again.
That was all before Linux even existed.
About 10 years later, I started working on Linux for the first time. Linux
brought a bonanza of management tools... Many were far more powerful than mine,
and I quickly switched to using them or their Windows ports. But surprisingly,
some of my tools had no equivalent under Linux. Or sometimes they had one, but
one that required ridiculously complex command lines for doing common tasks.
And I began porting the Windows tools I missed most to Linux.
To make a long story short, this proved a complex task, which eventually led to
the creation of the MsvcLibX library to hide the differences between Windows
and Linux standard C libraries. Since then, the same sources are used for
building my system management tools for all supported OS/processor targets.

Jean-François Larvoire
2016-12-31


System tools
____________

Supported operating systems are shown in brackets. [All] = [DOS,Win,Lin]
The Linux version does not have a .exe suffix.
For more details about each tool, use its -? option, or see its C source file header.

# File system management [All]

backnum.exe	Make a backup copy of a file, appending a unique number to its name.
dirc.exe	Compares two directories side by side.
dirsize.exe	Compute the total size used by a directory, optionally recursively.
redo.exe	Execute a command recursively in a whole directory tree.
truename.exe    Resolve all links. For oldtimers who miss command.com's internal truename
update.exe	Copy files only if newer.
which.exe	Superset of Linux which, supporting Windows' PATHEXT and special PowerShell rules.
whichinc.exe	Search for include files.
zap.exe         Delete files and directories.

# Text filtering and conversion [All]

1clip.exe	Pipe Windows clipboard contents into a program. [Win] Ex: 1clip | sort
2clip.exe	Pipe a program output into Windows clipboard. [Win] Ex: dir | 2clip
chars.exe	Display a table with all 8-bit characters.
conv.exe	Convert from/to various character sets.
deffeed.exe	Remove form feeds, replacing then with new lines.
detab.exe	Remove tabulations, replacing them with spaces.
dump.exe	Hexadecimal dump.
encoding.exe    Find the encoding of text files. Ex: Windows | UTF-8 | UTF-16
remplace.exe	Replace characters. Very limited regular expressions subset.
tee.exe		Duplicate stdin into two or more output streams.
trim.exe	Remove blank characters from the end of lines.

# System management [DOS,Win]

clocks.exe	Measure the system clock frequency, and the resolution of various timing functions. [All]
cpuid.exe	Identify the processor, and display its various capabilities.
driver.exe	Enumerate Windows services and drivers [Win]
gpt.exe		Display GUID Partition Tables
inicomp.exe	Compare .INI files, after internally sorting sections and definitions. [All]
msgbox.exe	Display various types of Windows message boxes. Useful in batch files. [Win]
sector.exe	Display or copy disk sectors. Decode or modify the MBR partition table.
smbios.exe	Dump SMBIOS structures
uuid.exe	Get the SMBIOS system UUID, or generate new UUIDs.


Common features
_______________

Multiple OS-specific executable versions, built from a common source.
Using Unix APIs as much as possible.

All tools support common command-line options:
  -?	Display a help screen
  -d	Debug mode
  -v	Verbose mode
  -V	Display the version
  -X	No-exec mode: Display what to do, but don't do it.

All file management tools use Unicode (UTF-8) internally.
- They will display non-ASCII characters correctly in any code page.
- They support command-line arguments containing Unicode characters.
Related options: (Useful when piping to files or other programs.)
  -A	Force outputing ANSI (Code page 1252) characters
  -O	Force outputing OEM (Code page 437) characters
  -U	Force outputing UTF-8 (Code page 65001) characters

All file management tools support Windows Symlinks, SymlinkDs, and Junctions.
- They will recurse safely, avoiding loops even if the filesystem contains some.
They support pathnames up to 32K, longer than the 260-char limit of basic WIN32 APIs.

All text filtering tools support piping input and output.
They're actually most powerful when chained together.
Ex: To convert the clipboard text content from Unix to Windows line endings:
1clip | remplace \n \r\n | 2clip

Some tools have distinct "normal" and "debug" versions.
(Useful when debugging features have a high memory or performance cost.)
In this case the -d option is only available in the "debug" version.
A set of macros in debugm.h helps adding powerful execution tracing features
in debug mode. (This debugm.h file is the only MsvcLibX library file needed
for building tools for Linux.) 


Building the SysToolsLib C/C++ tools
____________________________________

1) For Windows

Prerequisites:
- Install Microsoft Visual C++, from any recent version of Visual Studio.
- Install and build the MsvcLibX library.

Then extract ToolsSRC.zip to a new source directory. Ex: %WORKDIR%\SRC
Then go back to the command prompt and run:
cd %WORKDIR%\SRC
configure
make

The 32 and 64-bits executables are stored in the WIN32 and WIN64 subdirectories
respectively.


2) For Linux

Prerequisites:
- Extract the debugm.h include file from MsvcLibX.zip, and put it somewhere in
  your Linux include path. Ex: ~/include
    mkdir ~/include
    cp $WORKDIR/MsvcLibX/include/debugm.h ~/include
  Then run this command (and put it in your .bashrc file):
    export C_INCLUDE_PATH=~/include
  Important: Do not point C_INCLUDE_PATH at MsvcLibX/include, as this directory
  contains duplicates for standard include files (Ex: stdio.h), that will fail
  to compile in Linux.

Extract ToolsSRC.zip contents into a new source directory. Ex: $WORKDIR/SRC
Then go back to a Linux shell and run:
cd $WORKDIR/SRC
./configure
make

The executables are stored in a subdirectory named after the Linux uname.


3) For MS-DOS

- Install Microsoft Visual C++ 1.52c, from the Visual Studio 2005 DVD.
  This is the latest version that can build programs for DOS.
  It is still available for download on the MSDN Developer Network.
  Note that several versions of Visual C++ can safely be installed side by side.

Then go the command prompt and repeat the exact same procedure as for Windows:
(The configure.bat script will detect the DOS compiler, and make.bat will build
 for DOS and Windows thereafter.)
cd %SRC%\SysTools\SRC
configure
make

The 16-bits executables are stored in the DOS subdirectory.
Note that when rebuiling the WIN32 version after the DOS version, the WIN32 
executable will use the DOS version as its 16-bits stub. This allows to make
universal executables that run in all versions of DOS and Windows.


4) For Windows 95

- Install Microsoft Visual C++ 8, from Visual Studio 2005.
  This is the latest version that can build programs for Windows 95/98.
  (Older versions should also work, but have not been tested for a long time.)
  It is still available for download on the MSDN Developer Network.
  Note that several versions of Visual C++ can safely be installed side by side.

Then go the command prompt and repeat the exact same procedure as for Windows:
(The configure.bat script will detect the old compiler, and make.bat will
 optionally support building for Windows 95 thereafter.)
cd %SRC%\SysTools\SRC
configure
make "OS=WIN95"

The WIN32 executables supporting Windows 95 are stored in the WIN95 subdirectory.


5) For a particular set of DOS and Windows versions

Once multiple versions of the Microsoft C compiler are installed, and have been
configured by configure.bat, it's possible to build any set of target versions
at the same time by defining multiple OS values. Ex:

make "OS=DOS WIN95 WIN64"


6) For Windows using MinGW

Use the Linux procedure and shell scripts.
The executables will be in a MINGW32 or MINGW64 subdirectory.
Note that (as of 2014) MinGW has serious limitations in its support for
symbolic links, and for code pages in Windows. So the programs built with it
are less capable than those built with MSVC plus the MsvcLibX library.


Building one particular tool
____________________________

1) For DOS or Windows

Specify the target OS either in the path, or in the OS variable. Ex:
make WIN32\TOOLNAME.exe
or
make "OS=WIN32" TOOLNAME.exe

To build the debug version:
make WIN32\Debug\TOOLNAME.exe
or
make "OS=WIN32" "DEBUG=1" TOOLNAME.exe

In both cases, it's possible to build multiple OS versions at once by setting
multiple values for the OS variable.

If there is any compilation or link error, the TOOLNAME.log file will pop up.
If there's no error, it's possible to check for warnings by opening it anyway:
notepad TOOLNAME.log


2) For Linux, or for Windows with MinGW

make works provided that the right target names are specified, including the
output subdirectory.

An exe shell script makes it easier to build one tool without specifying paths:
./exe TOOLNAME

Define the _DEBUG variable to force building the debug version.
export _DEBUG=1

It's also possible to run the compiler directly:
OS=`uname -s`
PROC=`uname -p`
OUTDIR=$OS.$PROC
mkdir -p $OUTDIR/debug
gcc TOOLNAME.c -o $OUTDIR/TOOLNAME			# Release mode version	     
gcc -D_DEBUG TOOLNAME.c -o $OUTDIR/debug/TOOLNAME	# Debug version



The debug system
The make file system
Support for Windows 95/98
Support for UTF-8 sources
Support for NTFS symlinks and junctions
_______________________________________

See MsvcLibX' ReadMe.txt for details

