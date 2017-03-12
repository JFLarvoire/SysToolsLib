# Change Log

Major changes for the System Tools Library are recorded here.

For more details about changes in a particular area, see the README.txt and/or NEWS.txt file in each subdirectory.

## [unreleased] 2017-03-12
### Added
- C/include/configure.bat, make.bat: Added support for Visual Studio 2017
- C/include/src2obj.bat, src2obj.mak: Allows declaring SOURCES instead of OBJECTS in Files.mak. Much simpler to use. See example in C/MsvcLibX/Files.mak.
- C/include/All.mak: Use variable DIRS from Files.mak to recurse into all the subdirectories to build.
- C/MsvcLibX/src/open.c: New UTF-8 version of open().
- C/MsvcLibX/src/getpagesize.c: New UTF-8 version of getpagesize().
- Added UTF-8 versions of fputc() and fwrite().
- Added configuration variable IGNORE_NMAKEFILE for dealing with unwanted NMakefile homonyms.

### Changed
- UTF-8 programs now write 16-bits Unicode to the console. This allows displaying any Unicode character, even if it's not in the current code page.
  When stdout is redirected to a pipe or a file, the output is still converted to the current code page. This is the same as cmd.exe's own behaviour.
- Redesigned UTF-8 programs initialization. Now compatible with any main() routine declaration, with 0 or 2 or 3 arguments.
- configure.bat and make.bat do not automatically set persistent environment variables with the library paths. This did more harm than good, when dealing with multiple copies of the libraries.
  It's possible to revert to the old behaviour by defining PERSISTENT_VARS first.
- Many small changes and bug fixes.

## [1.7] 2017-01-09
### Added
- C/SRC/inicomp.c: A tool for comparing .ini or .reg files.

### Changed
- C/SRC/dict.h, - C/SRC/tree.h: Added the ability to create multimap-like dictionaries, for use in inicomp.c.

## [unreleased] 2016-12-31
### Added
- C/SRC/msgbox.c: A tool for displaying various types of Windows Message Boxes from batch files.

## [unreleased] 2016-12-16
### Changed
- C/include/configure.bat:
   - Updated the batch library framework.
   - Configure sub-projects recursively by default. 
   - Added option -R to prevent recursion if desired.
   - Fixed displaying the output of sub-instances of this script.
   - Avoid duplicate searches of MS tools in sub-instances, and of our own libs in some cases. (Known bug: We're still searching our own libraries multiple times in many cases.)
   - Added option -p to request setting persistent library path variables. By default, don't.
- C/include/make.bat:
   - Updated the batch library framework.
   - Improved the heuristic to detect pseudo-targets that need not be logged. (Ex: clean)
   - Added option -q to force running nmake without capturing its output. (Default for pseudo-targets)
   - Added option -Q to force capturing nmake output into TARGET.log. (Default for real targets)
- Batch/regx.bat: Updated the batch debugging framework.
- Batch/trouve.bat: Improved output filtering to convert / to \ only in file names, and fixed a few bugs.

### Fixed
- Batch/Library.bat:
   - Fixed issues when sourcing this library from another script.
   - Changed %EXEC% to not capture commands output by default. Fixes a usability bug: Not %EXEC% without options is fully transparent.

## [unreleased] 2016-12-09
### Changed
- Batch/Library.bat:
   - Added the ability to source Library.bat from another batch script.
   - Added routine :Prep2ExpandVars, useful for passing variables across endlocal barriers.
- C/SRC/sector.cpp and gpt.cpp:
   - Reformated the source to modern coding standards.
   - Improved the partition size formating readability.
   - Use the same units (GB, not GiB, etc) for disk and partition sizes.
   - Added options -H and -I to control the disk and partition size SI unit.
   - In gpt.cpp, added options -t and -x to control the sector number base (10 or 16).
   - Added many new partition types.
   - Added option -V to display the version.

### Fixed
- C/include/configure.bat and make.bat: Numerous fixes.
- PowerShell/PSService.ps1: Fixed incorrect hyphen.
- Batch/Library.bat:
   - %RETURN% failed if an UPVAR value contained a '?'.
   - %EXEC% exit code was not displayed correctly when called with expansion disabled.
   - %EXEC% failed if commands contained a ^.
   - %FUNCTION% incorrectly displayed arguments containing ^!% in debug mode.
   - %POPARG% now correctly handles trick characters ^!% correctly in all expansion modes.
- Batch/regx.bat:
   - Restructured to fix serious issues if value names or values contained tricky characters like: ^!%
   - The -X option now works correctly.
   - Enumerating 0 sub-keys in a valid key does not return an error anymore.

## [unreleased] 2016-11-07
### Changed
- Fixed and improved the way C make files and scripts use the optional OUTDIR.
- C/include/configure.bat: Performance improvements and fixed bugs with very old Visual Studio versions.
- Batch/Library.bat: Updated the recent %EXEC% entry errorlevel fixes to work with %DO% too.

## [unreleased] 2016-11-05
### Changed
- Updated the make scripts and make files to always reuse the initial script instance, when invoked recursively in subdirectories.
- C/include/configure.bat now has a -r option for recursively configuring every C subdirectory.
- C/include/make.bat now automatically uses the top instance log file, avoiding the need to explicitely use the -L option in recursive calls.
- Clarified "make cleanenv" output: It now displays the actual commands it had to run to cleanup the environment, or nothing if there was no need.
- Batch/Library.bat: Indent sub-scripts output in debug mode.

### Fixed
- Batch/Library.bat: Avoid log file redirection failures in recursive scripts.
- Several make files had the clean target defined twice, which caused a make warning. (Although it did work fine.)
- C/include/make.bat displayed a "file not found" error in recursive makes. (Due to incorrect log file handling in this case.)
- Several configure.*.bat files had the unwanted side effect of creating %OUTDIR%.
- Batch/u2w.bat and w2u.bat: Use remplace.exe new option -st instead of -t.

## [unreleased] 2016-10-21
### Changed
- Fixed various bugs and added missing inference rules, so that it's now possible to build assembly language programs in 16, 32, and 64-bits modes.
- Added commands del and rd to regx.bat.

## [unreleased] 2016-10-19
### Fixed
- Bugs in configure.bat, make.bat, and library.bat, that sometimes caused build failures in Windows XP.

## [unreleased] 2016-10-18
### Added
- Tcl/nlines.tcl: A tool for counting non-commented source lines.

## [1.6.2] - 2016-10-13
### Added
- C/SysLib/: A directory, with a new System Management library. See the README there for details.
- C/SRC/sector.cpp: Source for building sector.exe, a tool for raw hard disk I/O.
- C/SRC/gpt.cpp: Source for building gpt.exe, a tool for displaying legacy and GPT disk partitions.
- C/SRC/uuid.cpp: Source for building uuid.exe, a tool for managing UUIDs. An option displays the system UUID.
- C/SRC/smbios*.c: Sources for building smbios.exe, a tool for managing the System Management BIOS.
- Added a cleanenv target to all NMakefile files, to help testing multiple versions of the whole SysToolsLib.
- Added a release target to C/NMakefile, to automate building binary releases.

### Changed
- Updated the make system for building the SysLib library, and programs depending on it.
- Batch/trouve.bat: Added options -d, -l, -L.
  Allows finding files containing a string (or not), without getting every matching line.

## [unreleased] 2016-10-12
### Added
- The SysLib library can now be built in Linux, and used in Linux programs.
- Recursive Unix make files in C/, and C/MsvcLibX/, allowing to rebuild all C libraries and tools with a single make command.

## [unreleased] 2016-10-11
### Changed
Moved debugm.h and all common Windows make system scripts and nmake files to C/INCLUDE.  
This avoids having duplicate files in multiple subdirectories.  
Added proxy scripts in each subdirectory to avoid having to add C/INCLUDE to the PATH. 

## [unreleased] 2016-10-08
### Added
- Recursive Windows make files in C/ and C/MsvcLibX/, allowing to rebuild all C libraries and tools with a single make command.

### Changed
- Debug macro DEBUG_ON() now sets the debug level, and a new DEBUG_MORE() increases it.  
  Conversely, new macros DEBUG_LESS() and DEBUG_OFF() reverse the previous ones.  
  All four are usable outside of a DEBUG_CODE() block, and do nothing in release mode.

### Fixed
 - Fixed an incompatiility in MsvcLibX with the (very old) Visual Studio 2003.
 - Target distclean now removes the `config.*.bat` files output by configure.bat.

## [unreleased] 2016-10-04
### Fixed
- C/*/*mak*:
   - Fixed logging in case an OUTDIR is defined. This resolves the issue doing multiple builds at the same time.
   - Use the shell PID to generate unique temp file names.
- C/*/All.mak:
   - Updated the fix comparing the WIN95 and WIN32 C compilers.

## [unreleased] 2016-10-03
### Added
- Batch/touch.bat: A poor man's touch. Uses touch.exe if available, else uses pure batch.

### Changed
- Batch/Library.bat: New implementation of routine GetPID.
- Docs/catalog.md: Added missing files, and many examples.
- C make system updates to help making releases.

### Fixed
- C/*/All.mak: Fixed errors comparing the WIN95 and WIN32 C compilers.

## [1.5.1] 2016-09-29
### Changed
- C/*/*.mak:
   - Added an OUTDIR variable, to optionally define a different output directory base.
   - Display FAILED messages on the console when compilations or links fail.
- C/*/configure.bat:
   - Make sure the configure.*.bat scripts are invoked in a predictable order: The alphabetic order.
   - Also search for configure.*.bat in %windir% and %HOME%. Allows to globally define your own preferences.
   - Added a -o option to set the OUTDIR variable.  
     (Recommended: In test VMs accessing the host sources, set it in a "%windir%\configure.system.bat" script.)
- PowerShell/PSService.ps1:
   - Added a $ServiceDescription string global setting, and use it for the service registration.

### Fixed
- C/MsvcLibX/include/msvclibx.h: Fixed an issue that prevented the RC compiler to use our new derived windows.h.
- C/SRC/*.c: Minor changes to avoid warnings. No functional code change in most cases.
- C/MsvcLibX/src/main.c: Fixed a bug that caused empty "" arguments to be lost in UTF-8 programs.  
  This affected remplace.exe and redo.exe.
- C/*/dos.mak: Fixed an issue that caused double goal definition warnings, for DOS builds of programs that have their own .mak file.
- PowerShell/PSService.ps1: Fixed issue #5 starting services with a name that begins with a number.

## [1.5] 2016-09-15
### Changed
- C/MsvcLibX/*: Added a windows.h include file, that includes the Windows SDK's own Windows.h, then add its own UTF-8
  extensions. This minimizes changes when converting a Windows ANSI console application to support UTF-8.   
  Moved several internal derived Windows functions with UTF-8 support to their own module, and made them public
  in the new windows.h.
- C/SRC/conv.c:
   - Added the ability to convert a file in-place.
   - Automatically detect if the output file is the same as the input file.
   - Added several options: -same, -bak, -st
   - The help screen now displays the current code pages used in the system.

### Fixed
- C/SRC/*: Fixed several issues that caused build failures in Linux.
- C/SRC/detab.c, lessive.c, remplace.c: Fixed a serious bug that caused a file to be trunctated to 0-length if the output
  file was the same as the input file. Al three now use the same in-place conversion features created for conv.c.
- C/MsvcLibX/src/realpath.c, C/SRC/truename.c:  
   - Bug fix: Add the drive letter if it's not specified.
   - Bug fix: Detect and report output buffer overflows.
   - Convert short WIN32 paths to long paths.
   - Resize output buffers, to avoid wasting lots of memory.
- PowerShell/PSService.ps1: Fixed issue #4 detecting the System account. Now done in a language-independent way.

## [unreleased] 2016-09-05
### Changed
- Added support for C source files encoded as UTF-8 with BOM.  
  This removes a serious weakness in the previous design, where many C/SRC files contained UTF-8 characters, but no BOM.  
  Several Windows tools like Notepad incorrectly identified the encoding, and sometimes corrupted the UTF-8 characters.  
  The change was not trivial, because MS C compilers do react incorrectly when they encounter a UTF-8 BOM:  
   - MSVC 1.5 for DOS fails with an invalid character error.
   - Visual C++ for Win32 switches to a 16-bits character mode that we do _not_ want to use.
- Reencoded many sources as fully UTF-8 with BOM:  
  backnum.c, dirc.c, dirsize.c, driver.c, dump.c, lessive.c, redo.c, remplace.c, truename.c, update.c, which.c, whichinc.c
- Significantly improved conv.c. It's options now on par with that of remplace.c.
- Fixed several bugs in make.bat and configure.bat.

## [unreleased] 2016-06-27
### Changed
- PowerShell/ShadowCopy.ps1
   - Extended the 2-day preservation periods for a 4th week.
- C/SRC/remplace.c:
   - Added regular expression ranges, like [a-z]. Version 2.5.
- Added HPE copyright string in every source file.

## [unreleased] 2016-06-09
### Changed
- PowerShell/Library.ps1:
   - Added Test-TCPPort routine.
   - Added PSThread management routines.
   - Added Named Pipe management routines.
   - Added Using and New sample routines.

- PowerShell/PSService.ps1
   - Added PSThread management routines.
   - Added Named Pipe management routines.
   - The -Service handler in the end has been rewritten to be event-driven, with a second thread waiting for control messages coming in via a named pipe.                    

- PowerShell/ShadowCopy.ps1
   - Added 2-day preservation periods for the 2nd & 3rd week.

- PowerShell/Window.ps1
   - Made -Get the default command switch.
   - Allow passing in Window objects via the input pipe.
   - Added the -Step switch to all spacing windows regularly.
   - Added the -WhatIf switch to allow testing moves.
   - Added the -Capture command switch.
   - Added limited support for PowerShell v2.
   - Added the -Children switch to enumerate immediate children.
   - Added the -All switch to enumerate all windows.
   - Get the Program name for all windows in the -All case.
   - Added fields PID and Class to the window objects.
   - Also enumerate popup windows by default.
   - Added a 100ms delay before screen captures, to give time to the system to redraw all fields that are reactivated.

### Fixed
- PowerShell/PSService.ps1:
   - Fixed the -Service finally clause not getting called when stopping the service.
   - Fixed the remaining zombie task when stopping the service.

## [unreleased] 2016-05-11
### Changed
- C/SRC/update.c: Added option -F/--force to overwrite read-only files. Version 3.5.

### Fixed
- PowerShell/ShadowCopy.ps1: Fixed the number of trimesters calculation.

## [unreleased] 2016-05-02
### Changed
- Tcl/flipmails: Improved support for French and Asian mail headers with Unicode chars.

### Fixed
- Tcl/Library.bat: Fixed routines ReadHosts and EtcHosts2IPs.

## [unreleased] 2016-04-21
### Added
- PowerShell/ShadowCopy.ps1: A script for managing Volume Shadow Copies.

### Changed
- Tcl/Cascade.tcl: Added option -x to force the horizontal indent.

## [1.4.1] 2016-04-17
### Added
- Docs/Catalog.md: A list of all released files.
- PowerShell/Reconnect.ps1: Missing file, required by Out-ByHost.ps1.
- Batch/Reconnect.bat: Pure batch script for doing most of the same.
- Tcl/get.tcl: Replaces the obsolete get.bat that I had released by mistake.

## [1.4] 2016-04-15
Publicly released on github.com

### Changed
- Updated C area's configure.bat/make.bat/*.mak in preparation of new libraries releases.

## [1.4] - 2016-04-07
### Added
- A new Docs directory, with several docs inside.

### Changed
- Merged Tools.zip and Scripts.zip into a single SysTools.zip available in the release area.
- Minor updates to various scripts and C tools.

## [Unreleased] - 2015-12-16
### Changed
- Scripts: Minor improvements.
- C Tools: Major rewrite of the configure.bat/make.bat scripts, and associated make files.   
	   C tools can now target other Microsoft OS/processor targets, like WIN95, IA64 or ARM.

## [1.3] - 2015-09-24
### Added
- PowerShell/IESec.ps1			Test Internet if Explorer Enhanced Security is enabled
- PowerShell/Rename-Networks.ps1	Rename networks consistently on HP servers with many NICs
- PowerShell/Window.ps1			Move and resize windows
- PowerShell/PSService.ps1		A template for a Windows service written in pure PowerShell

### Changed
- Tcl/cfdt.tcl		Added the --from option to copy the time of another file
- Tcl/ilo.tcl		Allow specifying the list of systems in an @inputfile.  
			Improved routine DnsSearchList, to avoid dependancy on twapi in most cases.				    
			Improved heuristics to distinguish system and ilo names.
- C/SRC/configure.bat	Fix the detection of the Microsoft Assembler

## [1.2] - 2014-12-11
### Changed
- ScriptLibs.zip	New name for SourceLibs.zip, with numerous improvements
- Scripts.zip		Added a collection of scripts in these same languages
- C Tools		Main changes:
			- An improved make system.
			- An improved mechanism for adding changes to existing .h files.
			- Updated all routines to support for WIN32 pathnames >= 260 characters.
			- A few new routines.
			Detailed change Log: See ReadMe.txt in the C subdirectory.

## [1.1] - 2014-04-01
### Added
- MsvcLibX.zip	Microsoft Standard C library extensions
- ToolsSRC.zip	C/C++ tools sources
- Tools.zip	Win32 executables

## [1.0] - 2013-12-11
Initial release internally within HP of my scripting libraries

### Added
- SourceLibs.zip	A library of functions for various script languages
