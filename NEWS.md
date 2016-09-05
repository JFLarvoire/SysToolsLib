# Change Log

Major changes for the System Tools Library are recorded here.

For more details about changes in a particular area, see the README.txt and/or NEWS.txt file in each subdirectory.

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
- PowerShell\IESec.ps1			Test Internet if Explorer Enhanced Security is enabled
- PowerShell\Rename-Networks.ps1	Rename networks consistently on HP servers with many NICs
- PowerShell\Window.ps1			Move and resize windows
- PowerShell\PSService.ps1		A template for a Windows service written in pure PowerShell

### Changed
- Tcl\cfdt.tcl		Added the --from option to copy the time of another file
- Tcl\ilo.tcl		Allow specifying the list of systems in an @inputfile.  
			Improved routine DnsSearchList, to avoid dependancy on twapi in most cases.				    
			Improved heuristics to distinguish system and ilo names.
- C\SRC\configure.bat	Fix the detection of the Microsoft Assembler

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
