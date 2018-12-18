# Change Log

Major changes for the System Tools Library are recorded here.

For more details about changes in a particular area, see the README.txt and/or NEWS.txt file in each subdirectory.

## [Unreleased] 2018-12-18
### Changed
- C/SRC/update.c:
  * Added option -P to show the file copy progress.
  * Added option -- to force ending switches.

## [1.14.1] 2018-12-07
### New
- C/Font.c: Font.exe has been significantly improved, adding the ability to save and restore the console font.

## [1.14] 2018-11-19
### New
- Batch/TclSetup.bat: Configure Tcl for running .tcl scripts.
  Setup code split off of tclsh.bat, and improved to match features in PySetup.bat.
### Changed
- Batch/PySetup.bat: Accept start commands with quotes, or without if valid.   
  Accept start commands using copies of the default command.
  Added a verification that there's no additional command associated with the class.				      
- Batch/tclsh.bat: Use the improved FindTclsh routine from TclSetup.bat. Removed options -s and -t.
- Batch/Library.bat: Added routine TrimRightSlash; Improved routine condquote2.

## [Unreleased] 2018-11-16
### New
- Tcl/camel.tcl: New filter script, for use in pipes, to convert strings to camel case.
- Tcl/lower.tcl: New filter script, for use in pipes, to convert strings to lower case.
- Tcl/upper.tcl: New filter script, for use in pipes, to convert strings to upper case.
- Batch/12.bat: Added options -? for help, -A for ANSI character mode, -U for Unicode character mode,
  -V for the script version, -X for no-eXecute mode.
- Batch/hosts.bat: Added the ability to search entries in the hosts file.  
  Added a help screen.
### Changed
- Tcl/FlipMails: Change other common Unicode symbols to ASCII marks.
### Fixed
- C/SRC/2note.c: Fixed a memory allocation failure when converting Unix line endings to Windows endings. 

## [Unreleased] 2018-10-16
### Changed
- PowerShell/ShadowCopy.ps1: Added command -Previous, and arguments -Pathname and -Restore, to get and restore previous
  versions of files.
- Tcl/FlipMails.tcl: If the mail has double interline, halve interlines.  
  Decode many common Unicode emoticons to ASCII art. 

## [Unreleased] 2018-09-18
### Changed
- C/include/debugm.h: * DEBUG_FREEUTF8() now clears the buffer pointer, to allow calling DEBUG_FREEUTF8() multiple times.
### Fixed
- C/MsvcLibX/src/symlink.c: Dynamically allocate path buffers in all routines, to avoid stack overflows.
  This prevents update.exe from crashing when updating junctions.

## [Unreleased] 2018-09-18
### New
- PowerShell/ShadowCopy.ps1: Added options -Mount and -Dismount.

## [Unreleased] 2018-09-11
### Changed
- C/SRC/sector.cpp: Do not dump when explicitly setting output to "-".
  Added option -D to force dumping output to "-".
  Write error messages to stderr.
- Batch/hosts.bat: Use start, so that the script doesn't wait for elevate.exe completion.
  Added option -V to display the script version
- Batch/TimeX.bat: Changed option -t to display the start and end times
  Removed the SetTime routine, which was useless.
- Tcl/FlipMails.tcl: Changed the source encoding to utf-8.
  Make sure the I/O encodings match the console code page.
  Recognize several new Unicode bullet types.
- Batch/12.bat: Use codepage 65001 = UTF-8, to handle all Unicode characters in the pipe. This is possible now that
  FlipMails.tcl correctly handles code pages, like my C tools did.

## [Unreleased] 2018-07-02
### Changed
- Batch/hosts.bat: Removed the dependancy on elevate.exe, using it only if it is available.
  Added option -X to test what command is executed.
- Batch/regx.bat: Added command md to create a key.
### Fixed
- Batch/timex.bat: Now passes ! characters correctly in the command arguments.

## [Unreleased] 2018-06-04
### New
- Batch/history.bat: Display a list of previous commands entered at the cmd prompt.
- Tcl/unixtime.tcl: New script for converting Unix time stamps (# of seconds since 1970-01-01) <--> ISO 8601 date/time
### Changed
- Batch/Library.bat:  
  Added function :Sub.Init to create a SUB variable containing a SUB (Ctrl-Z) character.
- Batch/PySetup.bat:  
  Also search %HOMEDRIVE% if it's not C:.
- C/SRC/redo.mak:  
  Added optional support for a redo.rc, if present in the current directory.
- C/SRC/update.c:  
  Added option -e to erase target files not in the source.  
  Copy empty directories if the copyempty flag is set.  
  Prefix all error messages with the program name.
- C/SRC/zap.c:  
  The force option now deletes read-only files. It's not necessary for recursive deletions anymore.  
  Prefix all error messages with the program name.
- Tcl/base64dec.tcl:  
  Added option -q|--quiet.
- Tcl/base64enc.tcl:  
  The base64 package is not always available.
- Tcl/rxrename.tcl:  
  Added option -- to stop parsing options.
### Fixed
- C/SRC/update.c:  
  The force option did corrupt the mode flag.  
  Avoid a crash in update_link() on invalid links.
- Tcl/rxrename.tcl:  
  Fixed an exception when renaming files beginning with a -.

## [1.13] 2018-05-02
### New
- C/SRC/font.c: A new program to get information about available fonts.

## [Unreleased] 2018-04-26
### Fixed
- C/SRC/dirc.c, dirsize.c, redo.c: Make sure WIN32 pathname buffers are larger than 260 Bytes.  
  Together with 2018-04-26 changes, this ensures that dirsize and redo support paths > 260 bytes.  
  Redo still has an unavoidable limitation, documented in the help, in Windows installations without the Windows 10
  260-bytes paths fix enabled in the registry. (But it does generate correct commands when using {} arg replacements.)

## [Unreleased] 2018-04-26
### New
- C/MsvcLibX/src/iconv.c: Added a routine printfW(), for printing wide strings with the correct conversions.
- C/MsvcLibX/include/stdio.h: Define wprintf() as printfW().
- C/Include/debugm.h: Added macros DEBUG_WPRINTF(), DEBUG_WENTER() and DEBUG_WLEAVE(), using wprintf(). 
### Fixed
- C/MsvcLibX/src/chdir.c getcwd.c: For paths > 260 bytes, on systems without the Windows 10 workaround,
  manage the current directory locally. (Not used by any other stdio library routine for now, so don't expect stdio
  calls with relative paths to work if the current directory length is > 260 bytes.)
- C/SRC/dirc.c, truename.c: Minor improvements to take advantage of the above changes. Now lists paths > 260 bytes correctly.

## [1.12] 2018-03-26
### New
- A new tools called sml2.exe, originating from my [libxml2 fork](https://github.com/JFLarvoire/libxml2).
- C/SRC/which.c: New options -l & -v for diagnosing common search failures. which is now built by default for Linux.
### Changed
- C/NMakefile: Generalized the 'make release' mechanism created for adding ag.exe. Now adds any executable in Extras/bin/...  
  Moved ag.exe there, and added sml2.exe there too.
- *.mak: Automated the generation of link dependencies on outside libraries. No need to declare them manually anymore.  
  This ensures the automatic relink of programs when their dependent libraries changed. No need to do a `make clean` anymore.
### Fixed
- No more compilation warnings in any version of Visual Studio 8 to 15.
- C/Include/configure.bat: Fixed the search for the Windows kit 8.0.
- C/SRC/rd.c, zap.c: Fixed misleading or missing error messages.

## [Unreleased] 2018-03-06
### New
- C/SRC/2note.c: A program for sending a pipe output to a new Notepad instance.
### Changed
- C/SRC/1clip.c, 2clip.c: Updated the help message to show actual system-specific code page numbers in all cases.
- C/SRC/zap.c: Added options -i and -I. Ignore case in Windows by default. Added options -f and -rf, to delete complete directories.
- All makefiles: Dynamically build the list of dependent libraries, and automatically relink only the programs that use them. 
- Batch/Library.bat:
  New faster version of the FALSE.EXE macro.
  Simpler and faster versions of function is_dir.
  Added functions dirname, filename, has_wildcards
### Fixed
- Fixed _all_ warnings that appeared in Visual Studio 2015.
- MsvcLibX/src/dirent.c: Fixed alphasort() when files differ only by case.
- C/SysLib/FileW32.cpp: Added the ability to read 64-bit sizes in WIN64.

## [Unreleased] 2018-02-01
### Fixed
- The UTF-8 arguments and standard I/O initialization did not work with Visual Studio 14 and later.

## [Unreleased] 2018-01-25
### New
- Batch/mountw.bat: Mount a .wim Windows disk Image using a Unix-like command.
- Batch/umountw.bat: Unmount a .wim Windows disk Image using a Unix-like command.
- C/SRC/codepage.c: Display console font information, and a warning if it's a raster font.
### Changed
- C/Nmakefile: Add 32-bits *.exe from WIN32, if not available in WIN95.

## [Unreleased] 2018-01-23
### New
- C/Nmakefile: Added a 'make source-release' rule.
- C/Include/*: Added $(LIBRARIES) to the $(PROGRAM).* dependency list, so that it is relinked if one of its libraries changes.
  The LIBRARIES variable is generated by lib2libs.mak/.bat based on the LIB and LIBS variables.
### Changed
- C/SRC/1clip.c: Now built using MsvcLibX => Outputs UTF-16 to the console.

## [1.11] 2018-01-10
### New
- Batch/CheckEOL.bat: Check the line ending type (Windows/Unix/Mac) for a set of files.
- Batch/vcvars.bat: Run vcvarsall.bat for the latest Visual C++ installed.
- PowerShell/Get-Console.ps1: A tool for capturing the console screen as HTML or RTF or text,
  and copying it to the clipboard or a file.
### Fixed
- C/LoDosLib/dosdrv.h: Added workaround to avoid warnings when running h2inc.exe.
- C/SRC/cpuid.c: Fixed DOS warnings.
### Changed
- C/SRC/2clip.c: Remove the UTF8 BOM when writing RTF.
- C/Include/debugm.h: Added debug macros RETURN_PTR(p), RETURN_LONG(l), RETURN_CSTRING(s), RETURN_CPTR(p).
- C/NMakefile: 'make release' adds ag.exe if it's linked in C\Ag\.

## [Unreleased] 2017-12-14
### Fixed
- tcl/sml.tcl: Avoid a crash if the input contains less than 2 characters.

## [Unreleased] 2017-12-10
### Fixed
- PowerShell/PSService.ps1: Fixed an issue where stopping the service would leave the PowerShell process -Service still running.
### Changed
- PowerShell/PSService.ps1: Added the ability to run the service as a different user.
- Tcl/b64dec.tcl: Now compatible with Tcl versions <= 8.4.
- C/SRC/1clip.c:
  * Display the HTML Format header in debug mode.
  * Added option -r to get RTF data.
- C/SRC/2clip.c: Added options -h and -r, for copying HTML and RTF.

## [Unreleased] 2017-11-17
### Fixed
- The library included a useless b64dec.bat, instead of b64dec.tcl.
- C/SRC/chars.c: Fixed the output of the NUL character, which broke the columns alignment
### Changed
- C/SRC/chars.c: Added a -a option to output all characters, even those known to break alignment.
  Useful for testing filtering programs with _all_ 8-bit characters.
### New
- Tcl/b64dec.tcl: A base64 decoder, that was listed in the catalog, but missing in the library.
- Tcl/b64enc.tcl: A base64 encoder, symmetric to b64dec.tcl.

## [Unreleased] 2017-11-14
### Fixed
- C/MsvcLibX/*: Additional fixes for support for paths > 260 characters for dirsize.exe.
- C/SRC/dirsize.c: Added support for paths > 260 bytes in Windows 10 with the registry opt-in enabled.
  But it's still broken in Windows <= 8, and in Windows 10 if the registry opt-in has not been enabled.
- */configure.bat, */make.bat: Work around a cmd bug that broke the cmd windows title.
- C/include/src2objs.mak: Changed the output file name to $(PROGRAM).objects.
  This fixes the bug which caused libraries to be rebuilt even if no source had changed.

### Changed
- C/Include/*: Changed the default output dir to bin instead of .
- C/Include/*.mak: Added rules to build DLLs.
- */configure.bat, */make.bat: Added a search in the PATH, to be forward-compatible.
  Added the :CheckDir subroutine to search consistently.

## [Unreleased] 2017-10-27
### Fixed
- C/SRC/which.c: Also search for shell internal commands. Added options -i and -I to control that internal command search.
- C/NMakeFile: Improved make release messages and errors. Also release DOS, WIN64, etc, programs, if available.
- C/NMakeFile & *.mak : Changed the default output directory from . to bin. Ex: WIN32 output now goes to bin\WIN32\.

## [Unreleased] 2017-10-25
### Fixed
- C/MsvcLibX/*: Additional fixes for support for paths > 260 characters for The Silver Searcher (ag.exe).
  The tools in C/SRC/* are not affected.

## [Unreleased] 2017-10-09
### New
- C/SRC/zap.c: Remove files, without complaining if already absent.

### Fixed
- C/SRC/rd.c: The help screen was displayed twice.

## [1.10] 2017-10-06
### Fixed
- C/MsvcLibX/*: Fixed several serious bugs that broke support for paths > 260 characters.
  (It worked for file names that passed the threshold, but not for directory names that did :-()
  Now tested extensively with deeply nested paths up to 400 characters, and with the new md.exe and rd.exe tools.

### New
- C/SRC/md.c: Create a directory, all its parents, and don't complain if any exists.
  The verbose mode displays the list of directories actually created, possibly empty.
- C/SRC/rd.c: Remove a directory, without complaining if already absent.
  The force mode removes all contents. The no-exec mode allows testing what would be deleted.
  The verbose mode displays the list of all files and directories actually removed, possibly empty.

## [Unreleased] 2017-09-11
### Fixed
- Batch/regx.bat: Fixed the set -t option.
- Tcl/sml.tcl: Fixed several bugs exposed by the libxml2 test suite.

### Changed
- C/SRC/which.c: Now checks for environment variable NoDefaultCurrentDirectoryInExePath to select the command that will run.
- Batch/regx.bat:  
  Remove PowerShell-like drive colons and trailing \.  
  In verbose mode, type now "casts" the value type.
- Tcl/sml.tcl: Improved the self-test to compare conversion results both ways, and to optionally run recursively.

## [1.9.1] 2017-09-04
### New
- Batch/subcmd.bat: A script that starts a sub cmd shell, changing the prompt to show the shell depth level.
- Batch/wm.bat: A script that invokes WinMerge, even if it's not in the PATH.

### Fixed
- Batch/xfind.bat: Output the same # of : as find did. (0,1,2,...). This also avoids an extra : output by zapbaks.bat.
- Tcl/cascade.tcl: get_window_coordinates and minimize_window may throw exceptions.  
  Added a -V|--version option.
- C/MsvcLibX/include/sys/stat.h: Sockets and Fifos ARE supported in WIN32. Enable macros S_ISSOCK and S_ISFIFO.
- Tcl/cfdt.tcl: Fixed error {can't read "mtime": no such variable.}  
  Improved the debug and error reporting.  
  Added option -q|--quiet.
  
## [1.9] 2017-08-30
### Changed
- C/Makefile, C/SRC/Makefile:  
  `make install` now verifies that $bindir is in the PATH.  
  `make check` now checks if $bindir is in the PATH.  
  Make sure the installed scripts are executable.
- Batch/12.bat: Removed the -A options as the filters are now fixed.
  Instead, temporarily change to the system's default ANSI codepage.

### Fixed
- C/include/*.mak: Bugfix: The help target did output a "1 file copied" msg.
- C/NMakefile: Added a help target overrifing the one in All.mak.
- Batch/regx.bat: Fixed the type command output.
- Scripts.lst: Added missing scripts PySetup.bat and nlines.tcl.
- Tcl/show.tcl: Added "no such device" to the list of errors to ignore.

## [Unreleased] 2017-08-25
### New
- C/MsvcLibX/src/iconv.c: Added new routines vfprintfM(), fprintfM() and printfM() for printing multiple encodings.
- C/include/debugm.h: Added an EXE_OS_NAME definition. It'll eventually replace all OS_NAME definitions everywhere.
- Tcl/regsub.tcl: New tool to change text using regular expressions.

### Changed
- Tcl/README.txt: Converted to README.md, and updated.

### Fixed
- C/include/debugm.h: Fixed bugs that broke debug builds in Linux.

## [Unreleased] 2017-08-03
### New
- C/SysLib/FDisk*.cpp & FloppyDisk.*: Routines for block I/O to floppy disks, supporting DOS, WIN95, WIN32/64.
- C/SRC/sector.cpp: Added support for floppy disks.

### Changed
- C/SRC/sector.cpp, C/SRC/gpt.cpp: Updated the FormatSize() routine to make it more readable, and fixed the output in DOS.

### Fixed
- C/SRC/sector.cpp: When copying disk images into an existing file, the image was appended to the file, instead of
  overwriting it.

## [Unreleased] 2017-07-09
### New
- C/SysLib/VxDCall.c & VxDCall.h: Routines for WIN95 applications calling VxD WIN32 services, and VxD Device IO Controls.
- C/SysLib/HDisk95.c: New implementation using DPMI to call V86 int 13H. Uses VxDCall.c & VxDCall.h.

### Fixed
- C/*: Fixed most remaining compilation warnings.
- C/SRC/sector.cpp: Fixed the WIN95 version.

## [Unreleased] 2017-06-28
### Fixed
- C/MsvcLibX/*: Fixed non _UTF8_SOURCE programs initialization by using new constant _UTF8_LIB_SOURCE instead within MsvcLibX sources.
- C/SRC/gpt.cpp: Fixed warnings. No functional code change.
- C/SRC/1clip.c: Help displays the actual ANSI and OEM code pages. Fixed the link warning.
- C/SRC/msgbox.c: Fixed the link warning.
- C/SysLib/qword.h: Disable the warning C4710 "function 'QWORD::operator=' not expanded".

## [Unreleased] 2017-06-27
### Changed
- C/MsvcLibX/include/reparsept.h, C/MsvcLibX/src/dirent.c, C/MsvcLibX/src/readlink.c:  
  Renamed IO_REPARSE_TAG_LXSS_SYMLINK as IO_REPARSE_TAG_LX_SYMLINK.  
  Added several other missing reparse tags, and decode them in debug mode.

## [Unreleased] 2017-06-22
### Changed
- Tcl/cfdt.tcl: Display the list of files processed, to monitor progress.

### Fixed
- Tcl/cfdt.tcl: Do not display help if wildcards produce 0 names.

## [Unreleased] 2017-06-15
### Added
- Batch/PySetup.bat: A tool for configuring *.py scripts to use one of the Python interpreters available.

## [Unreleased] 2017-05-31
### Added
- C/SRC/asInvoker.manifest, asInvoker.rc: Default manifest to use for WIN95 and WIN32 builds.

### Changed
- C/SRC/conv.c, detab.c, lessive.c, remplace.c, update.c: Display MsvcLibX library version in DOS & Windows.
  This allows tracking new builds with updated libraries, when the tools C sources have not changed.
  The help still displays the main program version only, to limit the first line size.

### Fixed
- C/include/win32.mak: Use the new asInvoker.manifest by default for WIN95 and WIN32 builds. This allows fixing issues
  with unprivileged accesses to C:\ or %ProgramFiles% silently redirected to %LOCALAPPDATA%\VirtualStore\...
- C/MsvcLibX/include/string.h: Added a prototype for our modified version of strerror(), avoiding the link warning:
  LIBCMT.lib(strerror.obj) : warning LNK4006: _strerror already defined in MsvcLibX.lib(strerror.obj); second definition ignored
- C/SRC/conv.c, detab.c, lessive.c, remplace.c: Added error message for failures to backup or rename the output files.
- C/SRC/cpuid.c, cpuid.mak: Fixed warnings. No functional code change.

## [Unreleased] 2017-05-11
### Added
- C/MsvcLibX/include/msvclibx_version.h: Defines the MSVCLIBX_VERSION string.

### Changed
- C/MsvcLibX/src/dirent.c: Recognize LinuX SubSystem symlinks.
- C/SRC/remplace.c, C/SRC/update.c: The -V option now displays the MsvcLibX library version in DOS & Windows.
  This allows to see if an executable built from an unchanged source contains bug fixes from a newer version of the MsvcLibX library.

### Fixed
- C/MsvcLibX/src/fopen.c: In case of error, fopen() returned a wrong errno in some cases.
- C/MsvcLibX/src/iconv.c: Fixed fputc() for files in binary mode.
- C/MsvcLibX/src/junction.c: Dynamically allocate debug strings in junction(). This prevents stack overflows in debug mode.
- C/SRC/gpt.cpp, sector.cpp: When listing drives, tolerate missing indexes, as one drive may have been recently unplugged.
- C/SRC/remplace.c: Detect the input encoding, and convert the old and new replacement string to that encoding.
  This allows replacing non-ASCII strings, whether they come in from a pipe (using the console code page), or from an ANSI or UTF-8 text file.

## [1.8.1] 2017-04-12
### Fixed
- C/MsvcLibX/src/main.c: Fixed a bug that caused command-line arguments to be lost in some cases.
- C/MsvcLibX/src/iconv.c: Added missing routine puts(). This fixes a last-minute bug in the 1.8 release, that prevented
  the (unreleased) debug versions of the programs from displaying debug output.

## [1.8] 2017-04-05
### Added
- C/BiosLib: A library of routines for writing C programs running in the legacy BIOS. Previously released within HP in the early 2000s as NoDosLib.
- C/LoDosLib: A library of routines for writing MS-DOS drivers and TSRs.
- C/PModeLib: A library of routines for managing the x86 processor protected mode.

### Changed
- C/Files.mak now checks which subdirectories exist, and build those present.
- C/src/tee.c: Changed to a UTF-8 app, to support non-ASCII file names.

### Fixed
- C/*/configure.bat, C/*/make.bat: Avoid defining environment STINCLUDE after exit.
- C/MsvcLibX/include/*.h: Many small compatibility fixes.
- C/MsvcLibX/src/*: Make sure all debug prints are done in UTF-8.
- C/include/debugm.h: The Windows version of debug prints is now thread-safe.
- C/MsvcLibX/src/realpath.c: Fixed resolution for relative paths.

## [unreleased] 2017-03-16
### Added
- C/SRC/codepage.c: A new tool for displaying information about the current and available console code pages.

### Changed
- C/SRC/update.c: Now includes a workaround for the WIN32 incompatibility with pathnames ending with spaces or dots.
- C/SRC/sector.cpp: Added a dirty workaround for Windows' auto-mount feature, which prevented from copying
  a full disk image to a blank disk. (As soon as it detects a valid partition table, Windows mounts it, which locks the drive.)
- C/SRC/chars.c: Added an optional code page argument, for displaying characters for a different Windows code page.

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
- configure.bat and make.bat do not automatically set persistent environment variables with the library paths.
  This did more harm than good, when dealing with multiple copies of the libraries.
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
			Improved routine DnsSearchList, to avoid dependency on twapi in most cases.				    
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
