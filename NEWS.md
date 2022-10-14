# Change Log

Major changes for the System Tools Library are recorded here.

For more details about changes in a particular area, see the README.txt and/or NEWS.txt file in each subdirectory.

## [Unreleased] 2022-10-13
### Changed
- cfdt.tcl: Added options --pf and --ps, to better control the --m2n option effect.
- dirc.exe: Separate new option -I (ignore <=2s) from -i (ignore TZ).

## [Unreleased] 2022-08-08
### Fixed
- update.exe: Fixed errors "Failed to create directory PATHNAME. File exists." when updating files below a junction on
  a network server.

## [Unreleased] 2022-07-10
### Changed
- 1clip.exe:  
  Added option -s to get the clipboard data size.  
  Added option -L to get the clipboard text locale.  
  Added a workaround for an Excel bug, where putting more than 16 KB (8K WCHARs) into the clipboard returned only 8K Unicode chars.

## [Unreleased] 2022-06-27
### Changed
- 2clip.exe: Added option -N to remove the final CRLF.
- C/include/configure.bat: Changed the STINCLUDE presence test file from debugm.h to all.mak, in preparation for the
  split of the NMaker system, and the C debug library.
- Batch/Library.bat:
  Added routines :SaveErrorLevel and :RetrieveErrorLevel, to allow testing the exit code from the left half of a pipe.  
  Added routine :GetUserEmail.  
  Improved routine :RunAsAdmin.

### Fixed
- 2note2.bat: Fixed the issue with the extra CRLF appended to the text.

## [1.21.1] 2022-02-26
### Fixed
- C/MsvcLibX/include/sys/types.h, C/MsvcLibX/include/wchar.h, C/SRC/junction.c:  
  Corrected the definitions for MSVC's own _dev_t and _ino_t types, used in its stat* structures.
- 2clip.exe, 2note.exe, conv.exe, detab.exe, dump.exe, remplace.exe, trim.exe:  
  Fixed the detection of the input (file or pipe) type.
- 2note.exe: Added support for the new Windows 11 22H1 notepad.exe.

## [1.21] 2022-02-20
### New
- Shell/subsh: Start a sub Linux shell, changing the prompt to show the shell depth level.

## [Unreleased] 2022-02-18
### Changed
- Moved SysLib/WalkDirTree.c:SafeRealloc() to Include\debugm.h:ShrinkBuf(), and use it in many sources. (Could be used in many more.)
- Rewrote option -1 to record the junction themselves in a binary tree. Renamed the old -1 as -o. The new way is faster.

### Fixed
- Include/tree.h, dict.h: Updated macros, so that multiple kinds of trees can be used in the same program.

## [Unreleased] 2022-02-08
### Changed
- zap.exe: Added option -- to force end of switches.

### Fixed
- update.exe: Fixed option -- to force end of switches.

## [Unreleased] 2022-02-01
### Changed
- smbios.exe: Recognize table 43 "TPM Device".
- C/SRC/smbios_defs.c: Added table 43 name.
- C/SRC/smbios.mak: Rebuild smbios.exe if one of its dependencies changes.
- C/SRC/smbios.c: Improved the interface to the optional HPE tables decoder.

### Fixed
- chars.exe: Prevent a misalignment in the new Windows Terminal, which handles ASCII control characters differently
  from the old Windows Console.

## [Unreleased] 2022-01-26
### New
- Shell/Library.sh: A sourceable library of useful Bourne Shell routines

### Changed
- Shell/Library.bash: Added routines ReadSecret(), Info(), Warning(), Error().  
  Check whether the script was sourced or executed directly.
- dirsize.exe: Added option -f to follow links to directories.
- junction.exe: Added a dummy -nobanner option. Added option -l to list junctions non-recursively.

### Fixed
- Various makefiles: Fixed Linux builds
- C/MsvcLibX/src/GetShareBasePath.c, readlink.c: Fixed a bug preventing to read relative junction targets on network drives.

## [Unreleased] 2022-01-12
### Changed
- C/include/dict.h, tree.h: Moved from C/SRC.
- junction.exe, C/SRC/junction.c, C/SysLib/WalkDirTree.c:  
  Optionally make sure to visit folders only once.  
  More consistent error handling & better statistics.  
  Clarified the code, and fixed many bugs.

### Fixed
- C/MsvcLibX/readlink.c: Fixed a bug introduced in the last change in MlxResolveTailLinks(),
  which caused it to return an incorrect target in cases like: bin -> ..\bin -> ..\bin

## [Unreleased] 2021-12-27
### New
- C/SysLib/WalkDirTree.c: Added function GetFileID() for Windows
- junction.exe: Added a test of function GetFileID().

## [Unreleased] 2021-12-27
### Fixed
- junction.exe, C/MsvcLibX/readlink.c, C/SysLib/WalkDirTree.c: Fixed WalkDirTree() abort when encountering a Linux symlink

## [Unreleased] 2021-12-26
### Changed
- junction.exe: Added option -f to follow symlinks and junctions.  
  Detect link loops and danglink links, report them as errors (except in quiet mode), and avoid entering them.  
  The verbose flag now always reports both the link and its target.

### Fixed
- C/MsvcLibX/readlink.c: Added link loop detection to MlxResolveTailLinks().
- C/SysLib/WalkDirTree.c: Added link loop and dangling link detection to WalkDirTree().

## [Unreleased] 2021-12-15
### New
- junction.exe: Manage NTFS junctions as if they were relative symbolic links.
  Otherwise same features and options as SysInternal's junction.exe.
- ShareInfo.exe: Get information about a shared folder on a remote server. Useful to configure network shares,
  so that junction.exe, update.exe, etc, can properly manage relative junctions over there.
- C/SysLib/WalkDirTree.c, JoinPaths.c, pferror.c, pathnames.h, mainutil.h:
  Added routines WalkDirTree(), JoinPaths.c(), pferror()

## [Unreleased] 2021-12-13
### New
- C/MsvcLibX/include/errno.h: New include file extension, regrouping all errno-related definitions previously spread
  across several other include files.

### Fixed
- C/include/win32.mak, dos.mak, bios.mak: Fixed `make clean` & `make help` when used in the C/SRC subdirectory.
- C/MsvcLibX/include/msvclibx.h, unistd.h: There were 2 distinct (and incoherent) definitions of error ELOOP.

## [Unreleased] 2021-12-07
### New
- Batch/search.bat: Query the Windows Search service indexed files, using the Advanced Query syntax.

### Changed
- update.exe: Improved the explanations in the help screen about junctions on network drives.
- encoding.exe: Updated the help screen, documenting option -r for recursion.
- C/MsvLibX/src/mb2wpath.c: Detect pathnames that WIN32 APIs would "normalize", ex: "end space " or "nul",
  and prefix them with \\\\?\\ to avoid that.

### Fixed
- update.exe, zap.exe: Can now handle any NTFS file name, including those like "end space " or "nul".
  To distiguish files like "nul" from their homonym DOS devices, always use a path for the file. Ex: ".\nul"

## [Unreleased] 2021-11-29
### New
- C/MsvLibX/src/GetShareBasePath.c: Merge the heuristics for getting network junctions base paths, shared between
  readlink() and junction(), into a new fixed and improved common routine.

### Changed
- C/MsvcLibX/include/unistd.h and several sources: Renamed MsvcLibX-specific WIN32 routines with an Mlx prefix.  
  This was done to avoid confusions, like when I posted sample code containing MsvcLibX-specific calls on StackOverflow.

### Fixed
- update.exe: Better handling of junctions on network shares. See the build-in help screen for details.  
  Option -c can now delete dangling junctions.

## [Unreleased] 2021-11-19
### Changed
- regsub.tcl: Don't display the file names in quiet mode
- *.md: Reformated markdown documents in UTF-8, without tabs; Updated some sub-projects history notes.

## [Unreleased] 2021-11-18
### Fixed
- Batch/Library.bat: Fixed :IsAdmin to work even invoked in a 32-bits cmd.exe in a 64-bits OS.

## [Unreleased] 2021-11-16
### New
- C/include/configure.bat: Added support for Visual Studio 17/2022.

## [Unreleased] 2021-11-10
### New
- C/MsvcLibX/src/dirent.c, C/MsvcLibX/include/dirent.h: Added standard C library routine dirfd()

## [Unreleased] 2021-11-09
### New
- C/MsvcLibX/include/limits.h, stdint.h: Added TS 18661-1:2014 integer types widths macros
- C/SysLib/oprintf.*: Added 9 and 10 argument versions of oprintf()

### Fixed
- C/SysLib/qword.*, oprintf.*, makefile: Make sure QWORDs and oprintf() work together correctly in all operating systems.

## [Unreleased] 2021-10-28
### Changed
- ShellApp.ps1:  
  More consistent output object fields names, matching the originals.  
  Added option -Object to add a ComObject property, allowing to get all original fields if desired. Drawback: It's much slower.

## [Unreleased] 2021-10-27
### Fixed
- remplace.exe:  
  Increased the max string size from 80 to 255.  
  Fixed warnings in remplace.c compilation.

## [Unreleased] 2021-10-20
### Changed
- sector.exe:  
  Added option -lp to just list the available partitions.  
  In verbose mode, report the free space between partitions.  
  Display the correct partition size when it's 0xFFFFFFFF.
- gpt.exe:  
  Display the correct partition size when it's 0xFFFFFFFF.
- Batch/Library.bat: Added routine :GetEchoState.

### Fixed
- sector.exe:  
  Fixed the -ld option to report drives with no media inside.

## [Unreleased] 2021-10-11
### New
- ShellApp.ps1: Enumerate shell applications, i.e. File Explorers and Control Panels

### Changed
- Window.ps1: Major restructuration with...  
  Removed the code dealing with shell applications, moved to the new ShellApp.ps1.  
  A much faster enumeration by default (Rewritten in C#), and more complete.  
  New fields in the Window object: Visible, RealClass, hTopWnd, hRootWnd, ThreadId, StartTime  
  New options: -v, -Popups

### Fixed
- Window.ps1: Several bug fixes, like the -OnTop operation.

## [Unreleased] 2021-09-15
### Changed
- Batch/Library.bat:  
  Rewrote the variables preparation for passing back through endlocal/return.  
  New %^1!% ... %^6!% expand to (2^n)-1 hats before a !.  
  Renamed character entities from DEBUG.entity to @entity.  
  Added routine :ConvertEntitiesNoDebug & use it for -c, -C.  

### Fixed
- Batch/Library.bat: Fixed the :Return routine in Windows XP.

## [Unreleased] 2021-09-09
### Changed
- Shell/Library.bash: Improved the logging. Always indent the output to the log file.

## [Unreleased] 2021-09-07
### Changed
- FlipMails.tcl: Improved the mail separator detection.

### Fixed
- C/include/make.bat: Avoid counting constants like NO_WARNINGS as warnings

## [Unreleased] 2021-09-06
### Changed
- dirsize.exe:  
  Continue by default for all recursive operations.  
  Report the number of inaccessible directories if any error was ignored.

### Fixed
- dirsize.exe: Fixed "out of directory handles" errors with the -i option to ignore access errors.

## [Unreleased] 2021-07-16
### Changed
- GetConsole.ps1: Improved the HTML output, to get a better rendering in Chrome.

## [Unreleased] 2021-06-02
### New
- encoding.exe: New tool displaying the text encoding for one or more files.
- C/MsvcLibx/src/GetEncoding.c: Heuristics for detecting the most common text encodings (Binary/ASCII/Windows/UTF-8/UTF-16/UTF-32).

### Changed
- conv.exe:  
  Added support for conversions from/to UTF-16 & UTF-32.  
  Added option -F to _not_ use best fit characters (Ex: ç -> c) when the correct ones are missing.     
  Removed support for the obsolete Symbol code page.  
  Added a workaround for Windows Terminal limitations on displaying Unicode characters beyond \U10000.
- C/MsvcLibx/src/iconv.c: Added support for conversions from/to UTF-16 & UTF-32 in ConvertBuf() and associated routines.  
- C/MsvcLibx/src/iconv.c, C/MsvcLibx/include/iconv.h: Added a third argument to ConvertBuf() etc;  
  Renamed them with an Ex suffix; And added macros with the old name without the extra three arguments added recently.                       

### Fixed
- conv.exe: Fixed the default encoding when writing to the console.
- zap.exe: Fixed the recursion into linked subdirectories, and the recursive deletion of fixed names.

## [Unreleased] 2021-05-21
### Changed
- codepage.exe:  
  Fixed the output of C0 & C1 control codes in MS Terminal.  
  Added option -l as an alias to -i to list installed CPs.  
  Corrected typos and errors in the help.

## [Unreleased] 2021-05-03
### Changed
- conv.exe, detab.exe, trim.exe: If no change was made to the data, use the input file timestamp for the output.
  (Or preserve it when using the -= switch.)
- deffeed.exe:  
  Removed the -filter option, and use the standard - to specify the input comes from stdin.  
  Add -= and -same as equivalents of -self.  
  Add an optional output file name.
- Window.ps1: Avoid generating errors when modern apps (?) don't report a path.
- Batch/Library.bat: Added function :compare_versions.

## [Unreleased] 2021-04-18
### Changed
- remplace.exe: If no change was made to the data, use the input file timestamp for the output. (Or preserve it when using the -= switch.)

## [1.20] 2021-03-28
### Fixed
- C/SRC/Makefile, C/SysLib/Makefile : Use the correct processor in the MacOSX builds.

## [Unreleased] 2021-03-22
### Changed
- Renamed the Bash directory as Shell, as it's now intended to contain Posix Shell scripts, and not just Bash scripts.
- Changed all references to that directory in docs and makefiles.

### Fixed
- Makefile: Fixed an installation error on FreeBSD and MacOSX.

## [Unreleased] 2021-03-22
### Fixed
- Tcl/regsub.tcl: Fixed the input and output encoding in Windows, which was breaking non-ASCII characters.

## [Unreleased] 2021-03-12
### New
- .gitignore: List all the files that can safely be ignored.

### Changed
- C/SRC/dirc.c:  
  Optionally display the compression ratio in Windows.  
  Display more readable sizes, with thousands separators.
- C/SRC/sector.cpp:  
  Added option -X as an alias for option -ro.  
  Switched the order of the origin & number arguments.  
  Changed ":" to mean dump, and removed the -D switch.

### Fixed
- C/SysLib/FileW32.cpp: Fixed FileW32Read() and FileW32Write(), which hung on errors, causing `sector.exe -z` to hang.

## [Unreleased] 2021-03-11
### Fixed
- Batch/regx.bat:  
  Fixed a bug with the open command breaking the console. (Title left changed; Command history corrupt; exit not working; etc)  
  Fixed a bug when setting a value ending with a "\".

## [Unreleased] 2021-03-04
### Changed
- Python/PySetup.bat:  
  Added support for configuring py.exe as version 0.  
  Allow selecting an instance by python version.  
  Added option -r to register instances not found by py.exe.  
  Work around issues with installations in C:\PROGRA~1\...  
  In the end, make sure that both 'python' and 'python.exe' start the selected python instance.
- Python/python.bat:  
  Restructured the instances enumeration to output the same list as PySetup.bat.
- Python/pip.bat:  
  Restructured the instances enumeration to output the same list as PySetup.bat.  
  List the pip instance corresponding to py.exe instances.  
  Use Scripts\pip.bat or Scripts\pip3.bat in the absence of Scripts\pip.exe.
- Batch/Library.bat:  
  Use the non-instrumented condquote2 as the default condquote version.  
  Added routine :GetLongPathname.  
  Updated :GetRegistryValue to not output error messages, and return an exit code instead.

### Fixed
- Batch/Library.bat: Fixed %ECHO.V% and %ECHO.D% too agressive optimization.

## [Unreleased] 2021-03-01
### Fixed
- C/SRC/dirsize.c: Fixed another issue with Unix readdir() and d_type, which caused it to report size 0 on XFS file systems.
- C/SysLib/SysLib.mak: Compile R0Ios.c, Ring0.c, VxDCall.c only for WIN95, to avoid a build error with Win10 SDK >= 10.0.18362.0.
- C/Include/WIN32.mak: Removed a misleading log message "Environment variable PROGRAM_ not defined".

## [Unreleased] 2021-02-25
### Changed
- Python/PySetup.bat, Python/python.bat Python/pip.bat:  
  Make sure all 3 python batch scripts share the same python instance enumeration algorithm:  
  Removed a dependency on my VMs host drive configuration.  
  Always list the default instance first, as index #0.  
  Use short names to compare instances reliably.

## [Unreleased] 2021-02-16
### Changed
- Batch/regx.bat: Added options -se and -ue to easily manage System and User Environment variables.
- Python/PySetup.bat:  
  Option -l displays the index and version of each instance.  
  Also search for python.exe in "%LOCALAPPDATA%\Programs".  
  Options -s and -t can now specify an index, like "#3".

## [Unreleased] 2021-01-16
### New
- /Bash/profile.d/mcd.sh: A Posix Shell initialization script defining an mcd function, doing the same as Batch/mcd.bat.

### Changed
- Makefile: Install /Bash/profile.d/*.sh scripts into /etc/profile.d/.

## [Unreleased] 2021-01-08
### Changed
- C/SRC/exe.bat: Allow building multiple programs in a single command.

### Fixed
- C/SRC/exe.bat: Delete all local variables before running make.bat, which sometimes did confuse it, and caused it to fail.

## [Unreleased] 2021-01-06
### New
- Batch/TimeX.bat: Return the child task exit code.

### Changed
- Renamed the lessive program as trim. This affects C sources, make files, the documentation, etc.

### Fixed
- C/SRC/tee.c: Fixed the exit code for the help screen.
- C/SRC/Which.c: Don't report App Exec Link targets by default, until we know how to reliably use that target with the other link parameters.

## [1.19] 2020-12-17

## [Unreleased] 2020-12-16
### New
- NMakeFile, Files.mak: Allow building for Windows and generating a release from the project root directory.
- C/include/configure.bat, make.bat: Added option to link the OUTDIR directory to another one, if LINK_OUTDIR is defined, and creating links is authorized.
- C/config.outdir.bat, C/SRC/config.outdir.bat: Define LINK_OUTDIR=.. to output all .exe in the project root directory OUTDIR if possible.

### Changed
- C/SRC/Which.c: Added pwsh.exe as an alias for PowerShell.

### Fixed
- C/include/All.mak: Added a dependency on NUL for all pseudo-targets, to ensure that they run, even if a file with that name exists. Ex: `clean`

## [Unreleased] 2020-12-15
### New
- C/MsvcLibX/SRC/readlink.c, dirent.c, lstat.c: Added support for reading WSL v2 symlinks.
- C/SRC/dirc.c: Report Linux Subsystem Symlinks and UWP App. Exec. links.

### Changed
- configure.bat, Files.mak: Allow building the Windows version from the project root, like for Unix.
- C/SRC/dirc.c: Use the whole screen width if listing a single directory.
- C/SRC/Which.c: Finalized the AppExecLink reparse points support. 
- C/SRC/exe.bat: Accept either program or program.exe as target arguments.

## [Unreleased] 2020-12-12
### New
- C/MsvcLibX/SRC/readlink.c: Added support for AppExecLink reparse points, and function ReadAppExecLink() to read them.
- C/MsvcLibX/include/reparsept.h: Added support for AppExecLink reparse points.
- C/MsvcLibX/include/unistd.h: Added prototype for function ReadAppExecLink().

### Changed
- C/SRC/Which.c: The -l option now also displays the file length. Added support for AppExecLink reparse points. 
- C/SRC/conv.c: Added option -= as a synonym for -same.
- C/include/BatProxy.bat: Search for batchs in [.|..|..\..]\[.|WIN32|C]\include.

## [Unreleased] 2020-12-10
### New
- Tcl/htmldec.tcl: Decode strings that contain HTML entities.
- Tcl/htmlenc.tcl: Encode text strings with the 3 basic HTML entities & < >.

### Changed
- Batch/TimeX.bat: Added support for durations > 1 day.
- Batch/Library.bat: Function :Time.Delta now has optional support for dates, and durations > 1 day. Also the output variables base name can be changed.

## [Unreleased] 2020-12-03
### New
- C/Bash/truename: Added command-line arguments processing, and a pure Shell implementation for old systems that don't have realpath.

### Changed
- C/*/Makefile: Improved compatibility with old systems, by using our Bash/truename instead of realpath, and avoiding to use the sed -E argument.

## [Unreleased] 2020-12-01
### Changed
- Tcl/cascade.tcl: Implemented the -X/--noexec option, really.

## [Unreleased] 2020-11-26
### New
- C/Bash/distrib: New script identifying the OS distribution and version. Works in any Posix shell, and in any Unix derivative.

### Fixed
- C/Bash/*: Use a shebang with the '/usr/bin/env bash' command, as bash is not installed in /bin on all systems.

## [Unreleased] 2020-11-22
### New
- C/SysLib/copydate.c: copydate() routine, factored out of several C SRCs. Copies the timestamp and flags from one file to another.
  Added a version with ns resolution, and gave it priority when APIs are available.
  Added support for LLVM and FreeBSD.
- C/SRC/clean: Clean output files for a single program. Linux equivalent of the existing clean.bat.

### Changed
- C/SRC/backnum.c:
- C/SRC/update.c:
  Use the new factored out copydate() routine.
- C/SysLib/Makefile:
- C/SRC/Makefile:
  Avoid displaying entering/leaving directory for the same directory.
  Disable diagnostics carets for gcc and clang versions that have it. (To allow outputing readable compilation pragma messages.)

### Fixed
- Batch/Paths.bat:
- Python/PySetup.bat:
- Tcl/TclSetup.bat:
  Fixed an unlikely issue with the PowerShell call when there's a "'" in the script path.

## [Unreleased] 2020-10-15
### New
- Batch/n.bat: Added command-line arguments -? and -X when running in NT cmd.exe.
  Added the ability to pipe names of the files to open via stdin. Ex: dir /b *.c \| n
- Batch/ipcfg.bat: Also search the adapter name in the description field.
  Added the ability to define alias names and types, and defined "VPN" as the alias for Pulse Secure VPN.

### Fixed
- Batch/ipcfg.bat: Fixed the virtual adapter detection for English.
- Batch/path.bat: The PATH was truncated if it was > 1KB.
- Batch/pysetup.bat: The PATH was truncated if it was > 1KB.

### Changed
- Batch/pysetup.bat: Allow running as non-administrator, to be able to at least update local settings.
- Batch/paths.bat, pysetup.bat, tclsetup.bat:
  Use setx.exe to set global variables < 1KB, or reg.exe for those >= 1KB.
  Use either setx.exe (preferred) or PowerShell to broadcast a WM_SETTINGCHANGE message in the end.
- Library.bat: Changed :Echo.Color final string to a valid PowerShell comment ##- , for compatibility with mixed Batch+PowerShell scripts.

## [Unreleased] 2020-08-31
### New
- C/SRC/conv.c: Added the automatic detection of UTF-8 and UTF-16 input without BOM.
  Changed the default output file encoding to UTF-8 on Windows 10 >= 2019.
  Added type * to get the input encoding from IMultiLanguage2::DetectInputCodepage().
- C/SRC/2clip.c, 2note.c: Added the automatic detection of UTF-8 and UTF-16 input, both with and without BOM.
  Added options -8 and -16 to force decoding the input as UTF-8 or UTF-16.
- C/SRC/1clip.c: Added options -8 and -16 to encode the output as UTF-8 or UTF-16.
- C/SRC/update.c, dirc.c: Added the 2nd argument D:= , refencing the same path as the first argument, but on another drive.
- Tcl/FlipMails.tcl: Also reformat Yammer threads.

### Fixed
- C/SRC/backnum.c: The debug version no longer outputs double CRs at the end of Win32 error messages.
- C/MsvcLibX/src/err2errno.c: The Win32 error messages no longer contain a CR character.
- C/SRC/1clip.c, 2clip.c, 2note.c, driver.c: Non ASCII characters in Win32 error messages are now displayed correctly.
- C/SRC/conv.c, msgbox.c: Fixed a memory allocation bug causing a debug mode crash.

## [Unreleased] 2020-07-29
### New
- C/MsvcLibX/src/snprintf.c: Added fixed versions of snprintf() and vsnprintf() for old versions of MSVC.
- C/MsvcLibX/src/asprintf.c: Added asprintf() and vasprintf() routines emulating those from the GNU C library.
- C/MsvcLibX/src/aswprintf.c: Added wide character derivatives of asprintf() and vasprintf().
- C/MsvcLibX/src/dasprintf.c: Added asprintf() and vasprintf() derivatives used for MsvcLibX debugging.
- C/MsvcLibX/src/daswprintf.c: Added aswprintf() and vaswprintf() derivatives used for MsvcLibX debugging.

### Changed
- C/MsvcLibX/include/stdio.h: Define all the above.

### Fixed
- C/Include/debugm.h: Rewrote debug_printf() to use the standard asprintf() if available; Else use the fixed emulations.
  This should fix occasional crashes of programs, directly or indirectly using broken versions of MSVC's _snprintf().

## [Unreleased] 2020-07-02
### New
- Batch/12.bat: Added options -h, -n.
- Batch/md2h.bat: Added options -c, -p, -s to use specific github servers.
  Added support for per-server authentication tokens, and option -t to use a specific token.

## [Unreleased] 2020-06-26
### New
- MsvcLibX\src\asprintf.c: Added GNU C library routine asprintf().

## [Unreleased] 2020-06-05
### New
- Batch/Library.bat: Added a new version of :basename.
  Split :noww into :Now.wmic and :GetWeekDay.
  Added a :return routine, for lightweight debugging.

## [1.18] 2020-05-20
### New
- Batch/Library.bat: Added routine :AbsDirName.

## [Unreleased] 2020-05-16
### New
- Batch/AddPaths.bat: Added option -S to _not_ set the system variables.
- Batch/Library.bat: Added routines :EchoStrings* and macro %ECHOSTRINGS*%.

### Changed
- Batch/Library.bat: Renamed EchoVal* & ECHOVAL* as EchoVals* & ECHOVALS* resp.
- C/SRC/conv.c: Test IMultiLanguage2::DetectInputCodepage() in debug mode.

### Fixed
- Batch/Library.bat: Fixed the %LCALL% mechanism in the absence of any arg.

## [Unreleased] 2020-04-25
### Fixed
- C/SRC/1clip.c, 2clip.c, 2note.c: Fixed incorrectly formatted error messages.

## [Unreleased] 2020-04-20
### New
- C/*: Added support for building, installing, and running tools in MacOS.

## [Unreleased] 2020-04-13
### Changed
- */Makefile: Redesigned the `make install` command to use inference rules. This allows using `make -n install` to dry-run it.  
  Moved the scripts installation to the root Makefile.  
  Added a `make uninstall` command, also using inference rules.

## [Unreleased] 2020-04-11
### New
- C/MsvcLibX/src/mkstemp.c: Implemented mkdtemp() for WIN32.
- C/MsvcLibX/include/stdlib.h: Export the DOS and WIN32 versions of mkdtemp().

## [Unreleased] 2020-04-07
### Changed
- C/SRC/dirc.c: Added option -B|--nobak to skip backup and temporary files.

## [Unreleased] 2020-04-06
### Changed
- C/SRC/detab.c, lessive.c, remplace.c: Generalized -bak to backup any existing output file.  
  Added options -b|--bak as synonyms for -bak.  
  Added options -=|--same as synonyms for -same.

## [Unreleased] 2020-04-02
### Changed
- Batch/TimeX.bat: Changed the default to displaying start and end times, and added option -T to revert to the old behaviour.

### Fixed
- Bash/Library.bash: Fixed cd & export in Exec().
- install: Fixed cd & export in Exec(). (No impact on this script though.)

## [Unreleased] 2020-03-30
### Changed
- C/SRC/detab.c: Use mkstemp() instead of tempnam() to avoid security warnings in Unix.    
  Fixed a bug when the backup file does not exist initially.  
  Copy the input file mode flags to the output file.

### New
- C/MsvcLibX/src/mkstemp.c: Implemented mkstemp() for WIN32.
- C/MsvcLibX/include/stdlib.h: Export the DOS and WIN32 versions of mkstemp().

## [Unreleased] 2020-03-26
### Changed
- C/SRC/update.c: Numerous improvements, including some incompatible switch name changes. => Version 4.0.  
  Renamed switches -e|--erase as -c|--clean. (By analogy with `make clean`.)  
  Added switches -D|--makedirs, independent of -E|--noempty. (As the 2018-05-31 change linking the two did more harm than good.)  
  Renamed options -T|-resettime as -R|-resettime, -D|--makedirs as -T|--tree.
  Renamed options -S|--showdest as -D|--dest, and added -S|--source to explicit the default behaviour.  
  Added options -C|--command to display the equivalent shell commands. (And thus display both the source and dest. files.)  
  Fixed issues with copying a link on a file, or vice-versa.  
  Added option -B|--nobak to skip backup and temporary files.

## [Unreleased] 2020-03-23
### New
- install: New Unix script for installing select tools. (Contrary to `make install`, which installs everything.)

## [Unreleased] 2020-03-22
### Fixed
- C/SRC/which.c: Fixed wildcards search in Unix.

## [Unreleased] 2020-03-19
### New
- Makefile: New Unix makefile in the project root, to allow running make from the root.

### Fixed
- C/Makefile: Fixed recursion; Changed the default install dir to /usr/bin if /usr/local/bin is not in the PATH.
- C/SysLib/dirx.c, dirx.h: Fixed operation in a 32-bits OS, like Raspbian on a Raspberry Pi 2.

## [Unreleased] 2020-03-17
### Fixed
- C/SRC/backnum.c, dirc.c, dirsize.c, rd.c, redo.c, update.c, zap.c:
  Fixed serious issues with the use of the Unix readdir() function, which sometimes caused failures in Unix.

## [Unreleased] 2020-03-12
### New
- Batch/halve.bat: New filtering function for use with 12.bat.
- C/SysLib/dirx.c, dirx.h: Unix-specific directory access routines eXtensions.

### Fixed
- C/SRC/update.c: Fixed a serious usability issue when the target is a link to a directory.

## [Unreleased] 2020-03-02
### Changed
- C/SRC/cpuid.c: Display the CPUID index for every set of feature flags.
  Corrected typos and errors about MTRR registers.

## [1.17] 2020-02-29
### Changed
- PowerShell/ShadowCopy.ps1: Performance improvement: Let -Previous skip shadow copies more recent than the last file found.  
  Added switch -Exhaustive to revert to searching all previous shadow copies.
- Python/p*.bat: Also search for Python instances in "%ProgramFiles%\Python\python*\\".
- Batch/AddPaths.bat: Use the new paths.bat, instead of the old addpath.bat.
- Batch/md2h.bat: Display which server was used. Display an error message if curl failed.
- Tcl/Cascade.tcl: Improved the Windows Explorer management. List apps if -l is specified without an app.

## [Unreleased] 2020-02-26
### Changed
- C/SRC/cpuid.c: Decode cpuid(7, 0) output.  
  Added option -m to experiment with reading MSRs.
  Added option -w to experiment with reading WMI props.
  Output a few WMI props, including SLAT, in Windows.
  Skip head spaces in brand string, if any.

## [Unreleased] 2020-02-06
### Changed
- C/SRC/zap.c: Added the ability to delete directories using wild cards.   
  Fixed and improved the error reporting.  
  Make sure never to delete a root directory.  
  Add support for / in Windows paths.  
  Add support for C: bare drive names.  
  Remove extra ./ components in paths.
- C/MsvcLibX/src/main: In BreakArgLine(), don't escape the final " in the last arg. Ex, allow: "C:\"

## [Unreleased] 2020-01-30
### Fixed
- Batch/umountw.bat: Old fixes that got forgotten on a lab system:
  Fixed bug with default /commit or /discard.
  Fixed bug if the mount point is relative or has no drive.

## [Unreleased] 2020-01-29
### Changed
- C/Include.make.bat: In the end of the make process, count warnings, and open the log if any warning found.

### Fixed
- C/SRC/zap.c: Fixed FLAG_NOCASE default initialization.

## [Unreleased] 2020-01-28
### Fixed
- C/SRC/update.c: Fixed issue with "D:myFile" input files, where the path was set to "." instead of "D:.".

## [Unreleased] 2020-02-17
### Changed
- Batch/Library.bat: Added routines :is_empty_dir, :has_files, :has_dirs, :test_errorlevel.
- C/SRC/inicomp.c: Added option -f to allow free lines without an =value.  
  Removed the incorrect code handling homonym sections. The standard is to merge multiple parts into one single section.

## [Unreleased] 2020-01-19
### Fixed
- C/SRC/1clip.c: Fixed the default HTML output code page (with option -h) to be UTF8 in any console code page.

## [Unreleased] 2020-01-14
### New
- Batch/Autorun.cmd: `AutoRun -i` now installs the extension scripts from `AutoRun.d\` listed in `AutoRun.d\default.lst`.

### Fixed
- C/SRC/MsgBox.c: Fixed a regression due to a change in MsvcLibX's version of BreakArgLine():
  Remove C escape sequences, like \n \t \xXX, from the string.

## [Unreleased] 2020-01-09
### New
- C/Makefile: Also install Bash scripts.  
  Added option NOEXEC=1 to display files to install, without doing it.

### Changed
- Batch/Library.bat: Added several new routines: :strchr, :streq, :strstr, and :PopCArg.  
  Added macros %+INDENT% and %-INDENT%.

## [Unreleased] 2019-12-19
### Changed
- Batch/regx.bat: Always force creating keys without confirmation.

## [Unreleased] 2019-12-11
### Fixed
- Batch/paths.bat: Avoid displaying empty lines when PATH ends with a ';',

## [Unreleased] 2019-12-05
### New
- Batch/md2h.bat: Convert Markdown to HTML, and display it in a Web browser.

## [1.16] 2019-11-21
### New
- Batch/2note2.bat: New script to pipe text into a new instance of [Notepad2](http://www.flos-freeware.ch/notepad2.html).
- Batch/Get-Console.bat: Front-end to Get-Console.ps1, for use in cmd.exe shells.
- C/include/versions.h: Most OS, program, and library version strings initially defined in stversion.h. Usable in SysToolsLib-independent projects.
### Fixed
- C/include/win32.mak: Fix alignment errors in ARM64 builds. (Still not tested due to lack of an Windows 10/ARM64 test system)

## [Unreleased] 2019-11-11
### Changed
- Tcl/cfdt.tcl: Keep scanning files, even if one of them fails.  
  Skip directories when scanning zip files contents dates.  
  In verbose mode, display the old and new time changed.  
  Also allow time arguments formatted as 01h02m03s.

## [Unreleased] 2019-11-02
### Changed
- C/SRC/1clip.c, 2clip.c, conv.c, dump.c: Added -z and/or -Z options, for handling a Ctrl-Z as EOF respectively on input and output.

## [Unreleased] 2019-10-05
### Changed
- Batch/Library.bat:* Added macros ECHOSVARS, ECHOSVARS.V, ECHOSVARS.D.  
  Added routines :EscapeCmdString & :TestEscapeCmdString, and added option -te.  
  Fixed the passing of ^ ! arguments in options -c, -C, -M.  
  Rewrote (the broken) routine :convert_entities, and renamed it as :ConvertEntities.

## [Unreleased] 2019-10-02
### New
- Python/python.bat: Run the python interpreter, even if it's not in the PATH. Allows choosing one instance if there are multiple Python instances installed.
- Python/pip.bat: Run Python's pip.exe, even if it's not in the PATH. Allows choosing one instance if there are multiple Python instances installed.

## [Unreleased] 2019-09-25
### New
- C/SRC/which.c: Added the ability to search program names with wildcards;
  Added a verbose message about case-independent matches in Unix.
### Changed
- Batch/Library.bat: Added variable SNAME. Useful for writing generic help messages.
### Fixed
- C/include/debugm.h: Fixed Linux crash in debug_vsprintf() using new try_vsnprintf() wrapper around _vsnprintf().

## [Unreleased] 2019-09-18
### New
- Batch/paths.bat: A major redesign of the old ADDPATHS.BAT script.
- Bash/paths: A PATH manager for Unix, eventually equivalent to paths.bat for Windows.
- Bash/truename: A front end Unix' realpath, for old programmers used to DOS' truename.
### Changed
- Batch/vcvars.bat: Added support for Visual Studio 2019; Added option -l; Allow passing multiple arguments to vcvarsall.bat.
- C/src/which.c: Avoid searching twice in the same directory, if it appears twice in the PATH.
### Fixed
- Tcl/cfdt.tcl: Fixed bug when adding multiple names on the command line.

## [Unreleased] 2019-06-15
### New
- Updated the C make system so that every executable includes program properties, with a description, version, product infos, etc

## [1.15] 2019-04-28
### Changed
- Extras/ag.exe [The silver searcher](https://github.com/JFLarvoire/the_silver_searcher) now expands wildcards in command-line pathnames.
### New
- Batch/AutoRun.cmd.d/which.cmd: Automatically use (which.exe -i) to search for cmd.exe internal commands and doskey macros.

## [Unreleased] 2019-04-25
### Fixed
- Batch/n.bat: Work around trailing spaces issue in Windows 10 v 2019-03.

## [Unreleased] 2019-04-19
### New
- C/include/stversion.bat: New file defining version strings to display with -? and -V options.
### Changed
- C/SRC/*.c and *.cpp: Consistently use the version strings generated in stversion.h.  

## [Unreleased] 2019-04-16
### New
- C/include/BatProxy.bat: Proxy script for configure.bat and make.bat, automatically installed by configure.bat in all
  work directories to avoid having to manually fetch configure.bat and make.bat in the %STINCLUDE% directory.
### Changed
- C/include/configure.bat: Create local configure.bat and make.bat proxies.  
  Added option -nodos. Fixed option -vs, and split it into options -vsp and -vsn.
- C/include/debugm.h: Changed the debug puts() routine to an fputs-based routine which does not implicitely outputs an \n in the end.  
  Likewise, renamed SET_DEBUG_PUTS() as SET_DEBUG_PUT().
### Removed
- All make.bat and configure.bat proxies (but not the originals in C/include), as the proxies are now all recreated by configure.bat.  
### Fixed
- C/SRC/update.c: Implemented a fullpath() routine for Linux. This fixes a minor bug in the Linux version, displaying the link targets instead of the links themselves.

## [Unreleased] 2019-04-08
### Changed
- C/Batch/tcl*.bat moved to the Tcl directory. This is to reflect the fact that these batch scripts are only useful
  when using Tcl.
- C/Batch/PySetup.bat moved to the new Python directory. Same reason: This batch script is only useful when using Python.
  And the Python directory will be there for future Python scripts in the library.

## [Unreleased] 2019-04-02
### Fixed
- C/SRC/font.c: Fixed the font setting, which did not work well with TrueType fonts.
### New
- C/SRC/font.c: Added an optional weight argument.
- C/include/configure.bat: Added the ability to disable MASM and MSVC search.

## [Unreleased] 2019-04-02
### Fixed
- Tcl\cfdt.tcl: Fixed bug in the --i2n option, which moved files to the current directory.

## [Unreleased] 2019-03-19
### Fixed
- C/include/win32.mak: Fixed the DOS stub location, based on OUTDIR.

## [Unreleased] 2019-03-02
### New
- C/src/which.c: Rewrote the -i option to get the built-in list (cmd) and alias & function lists (PowerShell) from stdin.

### Fixed
- C/MsvcLibX/include/strings.h: Fixed this file, which did not work at all, as there's actually no MS equivalent to include.

## [Unreleased] 2019-02-11
### New
- C/include/arm64.mak: New make file to build ARM64 executables.

### Changed
- C/include/configure.bat: Added support for Visual Studio 2017 and 2019 previews.
  Fixed the detection of ARM, and added that of ARM64 tools.
- C/.../\*mak\*: Fixed the support for ARM, and added support for ARM64 tools.

## [Unreleased] 2019-02-06
### New
- Tcl/urldecode.tcl: An old script for decoding URLs encoded with %XX hexadecimal codes.
- Batch/AutoRun.cmd.d/pid.bat: AutoRun script defining a %PID% Process ID variable in cmd.exe, like the $$ variable in bash.

## [Unreleased] 2019-02-05
### New
- Batch/AutoRun.cmd: Manage multiple cmd.exe AutoRun scripts.
- Batch/DumpLink.bat: Display the contents of Windows Explorer *.lnk shortcuts.

### Changed
- Batch/AutoRun.cmd.d/history.bat: Does not run 'doskey /history' directly, but instead create a doskey macro that does.
- Batch/AddPath.bat: Added option -q for a quiet mode.
- Batch/AddPaths.bat: Only add WIN64 paths when running on an AMD64 processor.  
  Conversely add DOS path when running on an x86 processor.  
  Display the result path list just once in the end.  
  Added option -r to remove all my personal paths.

## [Unreleased] 2019-01-31
### Fixed
- Batch/regx.bat: Fixed the dir command when the pathname contains spaces.

## [Unreleased] 2019-01-20
### Changed
- C/include/*:
  * Added a mechanism to declare multiple PROGRAMS sources in Files.mak.
  * Automate the installation of configure.bat and make.bat proxies in all build directories.

## [Unreleased] 2019-01-16
### Changed
- C/SRC/which.c: Added option -- to stop processing switches.
- C/SRC/chars.c: Avoid outputing bytes \x80-\xFF by default for UTF-8 CPs.
- C/Include/All.mak:
  * Added macros defining standard extensions for Windows. Useful for OS-independent Files.mak that work for Unix too.
  * Exclude *.bak, *~, *# from the source file distribution.

## [Unreleased] 2019-01-14
### Changed
- C/SRC/dirc.c:
  * Added option -ct to report equal files with != times.
- C/SRC/update.c:
  * Added option -T to reset the time of identical files.
  * Fixed 2018-12-18 bug causing Error: Not enough arguments
- C/SRC/chars.c:
  * Added option -u to display Unicode characters or ranges.
  * Improved error reporting when switching code pages.
  * Added option -v to display verbose information.

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
- C/SRC/zap.c: Remove files, without complaining if already absent.

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
- PowerShell/IESec.ps1                  Test Internet if Explorer Enhanced Security is enabled
- PowerShell/Rename-Networks.ps1        Rename networks consistently on HP servers with many NICs
- PowerShell/Window.ps1                 Move and resize windows
- PowerShell/PSService.ps1              A template for a Windows service written in pure PowerShell

### Changed
- Tcl/cfdt.tcl          Added the --from option to copy the time of another file
- Tcl/ilo.tcl           Allow specifying the list of systems in an @inputfile.  
                        Improved routine DnsSearchList, to avoid dependency on twapi in most cases.                                 
                        Improved heuristics to distinguish system and ilo names.
- C/SRC/configure.bat   Fix the detection of the Microsoft Assembler

## [1.2] - 2014-12-11
### Changed
- ScriptLibs.zip        New name for SourceLibs.zip, with numerous improvements
- Scripts.zip           Added a collection of scripts in these same languages
- C Tools               Main changes:
                        - An improved make system.
                        - An improved mechanism for adding changes to existing .h files.
                        - Updated all routines to support for WIN32 pathnames >= 260 characters.
                        - A few new routines.
                        Detailed change Log: See ReadMe.txt in the C subdirectory.

## [1.1] - 2014-04-01
### Added
- MsvcLibX.zip  Microsoft Standard C library extensions
- ToolsSRC.zip  C/C++ tools sources
- Tools.zip     Win32 executables

## [1.0] - 2013-12-11
Initial release internally within HP of my scripting libraries

### Added
- SourceLibs.zip        A library of functions for various script languages
