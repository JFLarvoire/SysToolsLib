# System Tools Catalog

List of tools released on https://github.com/JFLarvoire/SysToolsLib, grouped by function category.

The OS column lists the operating systems supported. D=DOS, W=Windows, L=Linux.

Windows executables usually are available in a WIN32 and WIN64 version.
The WIN32 version runs in all versions of Windows from Windows 95 to Windows 10.
The WIN64 version runs in XP/64 an later versions of 64-bits Windows.

The Linux version of scripts and executables have no extension.

The Windows versions of these tools support Unicode pathnames, long paths > 260 characters, symlinks and junctions. (When the OS supports it.)
The output to the console is Unicode. (It's correct in any code page, provided the console font supports the Unicode characters used.)
The output piped to other apps is correct in any code page. (When the code page supports the Unicode characters used.)
As far as I know, no other Windows port of Unix tools can do all of that, and most can't do any.

## Manage directories

Name            | OS  | Description                                                                            | Example
----------------|-----|---------------------------------------------------------------------------------------|-------------
ag.exe          | -WL | A port for Windows of 'The Silver Searcher', a _very fast_ file searcher, with full regexp support.   | `ag --cc myvariable` &:# Find all references to myvariable in C sources in the current directory tree.
backnum.exe     | DWL | Make a backup copy of a file, appending a unique number to its name.                  | `backnum myprogram.c` &:# Back it up before making a risky change
cfdt.tcl        | -WL | Change files dates and times. Option for using a JPEG picture internal time.          | `cfdt --i2m *.jpg` &:# Change the files time to image time.
dirc.exe        | DWL | Compares directories side by side.                                                    | `dirc oldDir newDir` &:# Compare directories based on the files time and size.
dircc.bat       | DW- | Front end to dirc.exe. Compare file trees recursively, listing only different files.  | `dircc oldDir newDir`
dirdir.bat      | DW- | List subdirectories. (Non trivial for DOS in the absence of a dir /ad option.)        | `dirdir`
dirsize.exe     | DWL | Compute the total size used by a directory or a directory tree.                       | `dirsize -s -t` &:# Find which subdirectory uses up all that space.
in.exe          | DWL | Execute a command in a given directory, then come back.                               | `in .. cd` &:# Display the full name of the parent directory
mcd             | --L | Create a directory, and go there, in a single command.                                | `mcd TempDir`
mcd.bat         | DW- | Create a directory, and go there, in a single command.                                | `mcd TempDir`
md.exe          | DWL | Create a directory, all its parents, and don't complain if any exists.                | `"md.exe" -v a\b\c` &:# Displays a\ then a\b\ then a\b\c\ the first time; Displays nothing if repeated.
rd.exe          | DWL | Remove a directory. Force mode to remove all contents. Don't complain if absent.      | `"rd.exe" -v a` &:# Displays a\b\c\ then a\b\ then a\ the first time; Displays nothing if repeated.
redo.exe        | DWL | Execute a command recursively in a whole directory tree.                              | `redo dirsize -t` &:# Same end result as the above dirsize example.
rhs.bat         | DW- | Set all RHS flags for a file. Conversely, -rhs.bat removes them all.                  | `-rhs msdos.sys` &:# Often needed in the 1980s.
rxrename.tcl    | -WL | Rename a series of files, based on a regular expression.                              |
trouve.bat      | DWL | Find files containing a string. Uses WIN32 ports of Unix find and grep tools.         | `trouve "Error 1234"` &:# Did I record this error before?
truename        | --L | Alias of Unix' realpath, for old programmers used to DOS' truename.                   | `truename /var/run`
truename.exe    | DW- | Display the true name of a file or directory, like old DOS' truename internal command. Resolves links, junctions, etc. | `truename "C:\Documents and Settings"`
update.exe      | DWL | Copy files only if newer.                                                             | `update -X *.c X:\backup` &:# Display files which need updating, but don't do it.
zap.bat         | -W- | Delete files and directories, displaying the exact list of files deleted.             | `zap *.obj *.lst`
zap.exe         | DWL | Delete files and directories, displaying the exact list of files deleted.             | `zap *.obj *.lst` &:# Supersedes zap.bat, and zapbaks.bat with the zap -b option
zapbaks.bat     | -W- | Delete all kinds of backup files (*.bak, *~, #*#), optionally recursively.            | `zapbaks -r`

## Manage the PATH

Easily add, remove, or search entries in your PATH variables.

Name            | OS  | Description                                                                           | Example
----------------|-----|---------------------------------------------------------------------------------------|-------------
AddPaths.bat    | -W- | Configure the system path to include my tool boxes                                    | `addpaths`
paths           | --L | Manage the local and global path easily.                                              | `paths \| sort` &:# Display the local path, one entry per line, sorted. Useful to check if a a directory is already in a long %PATH%.
paths.bat       | -W- | Manage the local and global path easily.                                              | `paths \| sort` &:# Display the local path, one entry per line, sorted. Useful to check if a a directory is already in a long %PATH%.
which.exe       | -W- | Check which program will execute by the given name. Supports any executable type, including tcl and ps1. (Contrary to most WIN32 ports of which, which ignore Windows' %PATHEXT% variable, and don't know the different precedence rules of cmd and powershell shells.)       | `which which`

## Pipes and the Windows Clipboard

Brings the power of the command line to all Windows GUI applications.

Name            | OS  | Description                                                                                                                                                                                   | Example
----------------|-----|-------------------------------------------------------------------------------------------------------|-------------
12.bat          | -W- | Pipe Windows clipboard contents into a program, then that program output back into the clipboard.     | `12 sort`
1clip.exe       | -W- | Pipe Windows clipboard contents into a program.                                                       | `1clip \| sort`
2clip.exe       | -W- | Pipe a program output into Windows clipboard.                                                         | `dir \| 2clip`
2note.exe       | -W- | Pipe a program output into a new instance of Windows Notepad.                                         | `dir \| 2note`
2note2.bat      | -W- | Pipe a program output into a new instance of [Notepad2](http://www.flos-freeware.ch/notepad2.html).   | `dir \| 2note2`
Get-Console.bat | -W- | Front-end to Get-Console.ps1, for use in cmd.exe shells.                                              | `Get-Console`
Get-Console.ps1 | -W- | Capture the console window as HTML (or optionally RTF or plain Text), and send it to the clipboard.   | `Get-Console`

## Convert data

There tools are useful on their own, and even more so when combined in a command pipeline with the clipboard management tools above.

Name            | OS  | Description                                                                           | Example
----------------|-----|---------------------------------------------------------------------------------------|-------------
72w.bat         | -W- | Convert UTF-7 text to Windows ANSI characters.                                        |
82w.bat         | -W- | Convert UTF-8 text to Windows ANSI. Useful to decode scrambled emails. Uses conv.exe. | `12 82w`
a2u.bat         | -W- | Convert ANSI text to 16-bits Unicode text.                                            |
b64dec.tcl      | -WL | Decode base64-encoded data.                                                           | `12 b64dec`
b64enc.tcl      | -WL | Encode data into base64 text.                                                         | `12 b64enc`
camel.tcl       | -WL | Convert text to Camel Case. (i.e. capitalize each word.)                              | `12 camel`
codepage.exe    | -W- | Get information about the current and usable console code pages.                      | `codepage 850` &:# Display a table of code page 850 characters.
conv.exe        | -W- | Convert from/to various character sets.                                               | `type winfile.txt \| conv w .`
deffeed.exe     | DWL | Remove tabulations, form-feeds, etc.                                                  |
detab.exe       | DWL | Remove tabulations, replacing them with spaces.                                       | `detab this.md -t 4` &:# Type a text file with 4-colums tabulations
dump.exe        | DWL | Hexadecimal dump.                                                                     | `1clip \| dump`
encoding.exe    | -W- | Display the text encoding for one or more files. Ex: Windows \| UTF-8 \| UTF-16 ...   | `encoding *.txt`
halve.bat       | -W- | Halve the number of new lines. (To cleanup text with double interline.)               | `12 halve`
htmldec.tcl     | -WL | Decode strings that contain HTML entities.                                            | `12 htmldec`
htmlenc.tcl     | -WL | Encode text strings with the 3 basic HTML entities for & < >.                         | `12 htmlenc` or `htmlenc "R&D"`
lower.tcl       | -WL | Convert text to lower case.                                                           | `12 lower`
md2h.bat        | -W- | Convert MarkDown to HTML, and display it in a Web browser.                            | `md2h catalog.md` &:# Display this catalog
regsub.tcl      |  WL | Replace a regular expression at the file level.                                       | `regsub "(\d\d)/(\d\d)/(\d\d)" "19\1-\2-\3" *.c` &:# Change old dates to the ISO format
remplace.exe    | DWL | Replace any characters by others, including CR and LF.                                | `1clip \| remplace "; " \r\n \| sort` &:# Sort an email distribution list alphabetically
trim.exe        | DWL | Remove blank characters from the end of lines.                                        | `12 trim`
u2a.bat         | -W- | Convert 16-bits Unicode text to ANSI text.                                            |
u2w.bat         | -W- | Convert Unix End-Of-Lines (LF) to Windows End-Of-Lines (CR LF).                       | `u2w UnixProg.c`
upper.tcl       | -WL | Convert text to UPPER CASE.                                                           | `12 upper`
urldecode.tcl   | -WL | Decode URLs encoded with %XX hexadecimal codes.                                       | `urldecode "http://dot.com/A%20cryptic%20name.htm"`
w2u.bat         | -W- | Convert Windows End-Of-Lines (CR LF) to Unix End-Of-Lines (LF).                       | `w2u WindowsProg.c`

## Manage the Windows system

Name                        | OS  | Description                                                                           | Example
----------------------------|-----|---------------------------------------------------------------------------------------|-------------
25.bat                      | DW- | Switches the console to 25 lines x 80 columns. (CGA mode)                             | `25`
43.bat                      | DW- | Switches the console to 43 lines x 80 columns. (EGA mode)                             | `43`
50.bat                      | DW- | Switches the console to 50 lines x 80 columns. (VGA mode)                             | `50`
AutoRun.cmd                 | -W- | Manage multiple cmd.exe AutoRun scripts. See [AutoRun README](../Batch/AutoRun.cmd.d/README.md) | `AutoRun -i`
cascade.tcl                 | -W- | Align a set of similar windows regularly. Uses Twapi.                                 | `cascade notepad`
Disable-IPv6Components.ps1  | -W- | Disables Windows IPv6 component. Useful for diagnosing networking issues.             |
driver.exe                  | -W- | Manage Windows 9X and 2000+ drivers.                                                  |
DumpLink.bat                | -W- | Display the contents of *.lnk shortcuts (current directory, and target program)       | `DumpLink "%APPDATA%\Microsoft\Windows\Start Menu\Programs\System Tools\Command Prompt.lnk"`
Enable-IPv6Components.ps1   | -W- | Enables Windows IPv6 component. Useful for diagnosing networking issues.              |
font.exe                    | -W- | Display information about available fonts.                                            | `font -f` &:# Enumerate fixed-width fonts usable in the console
Get-ProductKey.ps1          | -W- | Display the product key used for activating Windows.                                  |
history                     | -W- | Display the list of all previous commands entered at the cmd prompt. (See [AutoRun macros](../Batch/AutoRun.cmd.d/)) | `history \| findstr /i make`
hosts.bat                   | -W- | Edit the hosts file with notepad                                                      | `hosts` &:# Must be run as administrator, else the hosts file can't be saved
IESec.ps1                   | -W- | Display Internet Explorer security mode settings.                                     |
IPcfg.bat                   | -W- | Front end to ipconfig.exe, filtering its output to make it more readable. Numerous options to select the information needed.  | `ipcfg` &:# Display just Ethernet and Wifi interfaces
Is-WindowsActivated.ps1     | -W- | Test if Windows is activated.                                                         |
junction.exe                | -W- | Manage NTFS junctions as if they were symbolic links. Otherwise like SysInternal's.   | `junction bin ..\bin` &:# Create a relative junction
Out-ByHost.ps1              | -W- | Execute PowerShell commands on remote machines in parallel, displaying results like Unix pdsh: That is as pure text lines, prefixed by the remote machine name. Requires installing Out-ByHost.ps1 and Reconnect.ps1 on every target system.          | `Out-ByHost (atlas 1..4) {update S:\tools C:\tools}`
Reconnect.bat               | -W- | Reconnect disconnected network drives. Useful after moving a laptop, or when opening an elevated window.      | `Reconnect S:`
Reconnect.ps1               | -W- | Reconnect disconnected network drives. Useful in remote PS sessions, which have drives disconnected by default.       | `Reconnect S:`
regx.bat                    | -W- | Manage the registry as if it were a file system, with keys=dirs & values=files. Output formatted as SML.      | `regx dir HKLM\SOFTWARE\Microsoft\Windows`
ShadowCopy.ps1              | -W- | Manage volume shadow copies. Options for listing previous versions of files. Option for recyling shadow copies like a pool of backup tapes.   | `help ShadowCopy.ps1 -detailed`
ShareInfo.exe               | -W- | Get information about a shared folder on a remote server.                             | `ShareInfo \\server\share`
ShellApp.ps1                | -W- | List Windows Shell Application Windows, i.e. File Explorers and Control Panels.       | `shellapp -fe -h | window -moveto 400,200`
Test-IPv6Components.ps1     | -W- | Test if Windows IPv6 component are enabled or disabled. Useful for diagnosing networking issues.      |
Window.ps1                  | -W- | Move and resize windows.                                                              | `Window "Server Manager" -MoveTo 150,150 -Resize 1000,750 -OnTop`

## Manipulate structured trees

XML is good for programs, but hard to read for humans.
JSON is easier to read, but not as powerful as XML.
SML is XML made readable. It looks like C. It's strictly equivalent to XML: Any XML file can be converted to SML and back with no loss.

Name            | OS  | Description                                                                                           | Example
----------------|-----|-------------------------------------------------------------------------------------------------------|-------------
sml.tcl         | -WL | Convert XML files to a much simpler structured text format, and back.                                 | `type config.xml \| sml`
sml2.exe        | -WL | A rewrite of sml.tcl in C, based on libxml2. Options to reformat and indent the output.               | `type config.xml \| sml2 -f`
show.tcl        | -WL | Display files contents, or whole directory trees contents, in a simple structured text format.        | `show /proc/fs`
xpath.tcl       | -WL | Use XPATH to extract data from an XML file.                                                           | `xpath -i config.xml dir /root/display`

## Programmer toolbox

Name            | OS  | Description                                                                           | Example
----------------|-----|---------------------------------------------------------------------------------------|-------------
chars.exe       | DWL | Display a table of ASCII and all 8-bit characters.                                    |
CheckEOL.bat    | -W- | Check the line ending type for a set of files.                                        | `CheckEOL *.c`
codepage.exe    | -W- | Get information about the current code pages, or the characters in other ones.        | `codepage 1253` &:# Show characters in code page #1253
echoargs        | --L | Display echoargs arguments. Useful to diagnose command-line processing issues.        | `echoargs how" many args "here?`
echoargs.bat    | DW- | Display echoargs arguments. Useful to diagnose command-line processing issues.        | `echoargs how" many args "here?`
EchoArgs.ps1    | -W- | Display echoargs arguments. Useful to diagnose command-line processing issues.        | `echoargs how" many args "here?`
errorlev.bat    | D-- | Display last command's errorlevel. (There's no %ERRORLEVEL% variable in DOS.)         |
inicomp.exe     | DWL | Compare .ini or .reg files. Useful to detect changes in the Windows registry.         | `inicomp server1.reg server2.reg`
MakeZip.bat     | -W- | Create a zip file, based on a list of files in an input file. Uses 7-zip.             | `makezip tools.lst` &:# Builds tools.zip.
mountw.bat      | -W- | Mount a .wim Windows disk Image using a Unix-like command.                            | `mountw boot.wim` &:# Mounts the image at the default C:\mnt\wim
msgbox.exe      | -W- | Display various types of message boxes, and return answers to the batch.              | `msgbox -x -c "About to erase your disk"`
nlines.tcl      | -WL | Count lines, and non-commented source lines, in a set of files.                       | `nlines -r` &:# Count recursively in the current dir. and sub-dirs.
PSService.ps1   | -W- | Sample Windows Service entirely in a PowerShell script.                               | `help PSService.ps1 -detailed` &:# Comprehensive built-in usage doc.
PySetup.bat     | -W- | Find the Python interpreter even if it's not in the PATH. Configure Windows to run *.py files as command-line scripts.        | `pysetup` &:# Test if Python is correctly configured
subcmd.bat      | -W- | Start a sub cmd shell, changing the prompt to show the shell depth level and modes.   | `subcmd /V:on` &:# Enable delayed expansion in the sub shell
subsh           | --L | Start a sub Linux shell, changing the prompt to show the shell depth level.           | `subsh`
TclSetup.bat    | -W- | Configure Windows to run *.tcl files as command-line scripts, and *.tk as windowed scripts.   | `tclsetup` &:# Test if Tcl is correctly configured
tclsh.bat       | -W- | Find the Tcl shell even if it's not in the PATH.                                      | `tclsh.bat -t` &:# Test if Tcl is correctly configured
tee.exe         | DW- | Duplicate the input from stdin to multiple parallel outputs.                          | `dir \| tee -a work.log`
TimeX.bat       | -W- | Time the eXecution of a command. Similar in spirit to Unix' time command.             | `timex ping -n 1 myserver.mysite.org`
touch.bat       | -W- | Uses touch.exe if available, else does it in pure batch (slower!).                    | `touch myprog.c`
unixtime.tcl    | -WL | Convert a Unix Epoch time (# of seconds since Jan 1970) to an ISO 8601 date/time.     | `unixtime 1515063609`
umountw.bat     | -W- | Unmount a .wim Windows disk Image using a Unix-like command.                          | `umountw` &:# Unmounts the image at the default C:\mnt\wim
vcvars.bat      | -W- | Run vcvarsall.bat for the latest Visual C++ installed.                                | `subcmd`   `vcvars`
whichinc.exe    | DW- | Enumerate all include files that a C/C++ source potentially includes.                 | `set INCLUDE=... & whichinc myprog.c`
with.exe        | DW- | Run a command with specific environment variables.                                    | `with DEBUG=1 build` &:# Build the debug version of your application
wm.bat          | -W- | Invoke WinMerge, even if it's not in the PATH.                                        | `wm old_version new_version`

## Harware and BIOS management

Name            | OS  | Description                                                                           | Example
----------------|-----|---------------------------------------------------------------------------------------|-------------
cpuid.exe       | DW- | Identify the processor, and display its various capabilities.                         | `cpuid -v`
gpt.exe         | DW- | Manage GUID Partition Tables.                                                         | `gpt` &:# Dump the legacy and GUID partition tables
sector.exe      | DW- | Manage disk sectors. Options for dumping them, or copying them to & from files.       | `sector hd0:` &:# Dump the hard disk 0 boot sector
smbios.exe      | DW- | Manage the System Management BIOS.                                                    | `smbios -t 0:` &:# Dump SMBIOS table 0
uuid.exe        | DW- | Manage UUIDs.                                                                         | `uuid -s` &:# Display the system UUID

## Misc

Name            | OS  | Description                                                                           | Example
----------------|-----|---------------------------------------------------------------------------------------|-------------
distrib         | --L | Display the Linux distribution name, major.minor version, and target processor        |
FixMHT.tcl      | -WL | Fix relative links in a .mht file. (Works around known bugs in many .mht files.)      |
FlipMails.tcl   | -WL | Convert mail threads into a single ASCII text file with mails in chronologic order.   | `12 flipmails`
fvlCache.bat    | -W- | List the most recent files in the Flash Player cache.                                 |
gcCache.bat     | -W- | List the most recent files in the Google Chrome cache. Ex: The video you just viewed. |
get.tcl         | -WL | Get a file from a web server. Simpler to use than Unix' wget.                         | `get "https://cloud.com/picture.jpg`
ie.bat          | -W- | Start Internet Explorer                                                               |
ieCache.bat     | -W- | List the most recent files in the Internet Explorer cache.                            |
n.bat           | -W- | Start Notepad                                                                         | `n readme.md`
search.bat      | -W- | Query the Windows Search service, and display results in Windows Explorer.            | `search moon AND sun`

-----------------------------------------------------------------------------------------------------------------------

## Notes

* All Tcl scripts require installing a Tcl interpreter.
  This interpreter is standard in Linux, but absent in Windows.
  See the [README](../Tcl/README.md) file in SysToolsLib's Tcl directory for instructions on how to install a Tcl interpreter in Windows.

* All tools (both scripts and C programs) support the -? option for help, and most share a few other common options:

Option | Description
------ | -----------
  -?   | Display a help screen.
       |    
  -d   | Display debug information: Help the author understand what code is running. (In the debug build only)
  -v   | Display verbose information: Help the users understand what the program is doing.
  -V   | Display the script or program version and exit.
  -X   | No-eXec mode: Display what the program would do, but don't do it.
       |    
  -A   | Force ANSI encoding (aka. Windows System Code Page) output. (Windows only)
  -O   | Force OEM encoding (aka. DOS Code Page) output. (Windows only)
  -U   | Force UTF8 encoding output. (Windows only)
