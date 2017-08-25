# System Tools Catalog

List of tools released on https://github.com/JFLarvoire/SysToolsLib, grouped by function category.

The OS column lists the operating systems supported. D=DOS, W=Windows, L=Linux.

## Manage directories

The Windows versions of these tools support Unicode pathnames, long paths > 260 characters, symlinks and junctions. 
The output is correct in any code page.
As far as I know, no other Windows port of Unix tools can do all that, if any.

Name		| OS	|Description										| Example
----------------|-------|---------------------------------------------------------------------------------------|-------------
backnum.exe	| DWL	| Make a backup copy of a file, appending a unique number to its name.	 		| `backnum myprogram.c` &:# Back it up before making a risky change
cfdt.tcl	| -WL	| Change files dates and times. Option for using a Jpeg picture internal time.		| `cfdt --i2m *.jpg` &:# Change the files time to image time.
dirc.exe	| DWL	| Compares directories side by side.							| `dirc oldDir newDir` &:# Compare directories based on the files time and size.
dircc.bat	| DW-	| Front end to dirc.exe. Compare file trees recursively, listing only different files.	| `dircc oldDir newDir`
dirdir.bat	| DW-	| List subdirectories. (Non trivial for DOS in the absence of a dir /ad option.)	| `dirdir`
dirsize.exe	| DWL	| Compute the total size used by a directory ot tree.					| `dirsize -s -t` &:# Find which subdirectory uses up all that space.
mcd.bat 	| DW-	| Create a directory, and go there, in a single command.				| `mcd TempDir`
redo.exe	| DWL	| Execute a command recursively in a whole directory tree.				| `redo dirsize -t` &:# Same end result as the above dirsize example.
rhs.bat 	| DW-	| Set all RHS flags for a file. Conversely, -rhs.bat removes them all.			| `-rhs msdos.sys` &:# Often needed in the 1980s.
rxrename.tcl	| -WL	| Rename a series of files, based on a regular expression.		 		| 
trouve.bat	| DWL	| Find files containing a string. Uses WIN32 ports of Unix find and grep tools.		| `trouve "Error 1234"` &:# Did I record this error before?
truename.exe	| DW-	| Display the true name of a file or directory, like old DOS' truename internal command. Resolves links, junctions, etc.							| `truename "C:\Documents and Settings"`
update.exe	| DWL	| Copy files only if newer.								| `update -X *.c X:\backup` &:# Display files which need updating, but don't do it.
zap.bat 	| -W-	| Delete files and directories, displaying the exact list of files deleted.		| `zap *.obj *.lst`
zapbaks.bat	| -W-	| Delete all kinds of backup files (*.bak, *~, etc...), optionally recursively.	 	| `zapbaks -r`

## Manage the PATH

Easily add, remove, or search entries in your PATH variables.

Name		| OS	|Description										| Example
----------------|-------|---------------------------------------------------------------------------------------|-------------
addpath.bat	| -W-	| Manage the local and global path easily.						| `addpath | sort` &:# Display the local path, one entry per line, sorted. Useful to check if a a directory is already in a long %PATH%.
AddPaths.bat	| -W-	| Configure the system path to include my tool boxes					| `addpaths`
which.exe	| -W-	| Check which program will execute by the given name. Supports any executable type, including tcl and ps1. (Contrary to most WIN32 ports of which, which ignore Windows' %PATHEXT% variable, and don't know the different precedence rules of cmd and powershell shells.) | `which which`

## Access the Windows Clipboard

Brings the power of the command line to all Windows GUI applications.

Name		| OS	| Description																							| Example
----------------|-------|-------------------------------------------------------------------------------------------------------|-------------
12.bat		| -W-	| Pipe Windows clipboard contents into a program, then that program output back into the clipboard.	| `12 sort`
1clip.exe	| -W-	| Pipe Windows clipboard contents into a program.							| `1clip | sort`
2clip.exe	| -W-	| Pipe a program output into Windows clipboard.								| `dir | 2clip`

## Convert data

There tools are useful on their own, and even more so when combined in a command pipeline with the clipboard management tools above.

Name		| OS	| Description										| Example
----------------|-------|---------------------------------------------------------------------------------------|-------------
72w.bat		| -W-	| Convert UTF-7 text to Windows ANSI characters.					|
82w.bat		| -W-	| Convert UTF-8 text to Windows ANSI. Useful to decode scrambled emails. Uses conv.exe. | `12 82w`	
a2u.bat		| -W-	| Convert ANSI text to 16-bits Unicode text.						|
b64dec.tcl	| -WL	| Decode base64-encoded data.								| `12 b64dec`
codepage.exe	| -W-	| Get information about the current and usable console code pages.			| `codepage 850` &:# Display a table of code page 850 characters.
conv.exe	| -W-	| Convert from/to various character sets.						| `type winfile.txt | conv w .`
deffeed.exe	| DWL	| Remove tabulations, form-feeds, etc.							|
detab.exe	| DWL	| Remove tabulations, replacing them with spaces.					| `detab this.md -t 4` &:# Type a text file with 4-colums tabulations
dump.exe	| DWL	| Hexadecimal dump.									| `1clip | dump`
lessive.exe	| DWL	| Remove blank characters from the end of lines.					| `12 lessive`
regsub.tcl	|  WL	| Replace a regular expression at the file level.					| `regsub "(\d\d)/(\d\d)/(\d\d)" "19\1-\2-\3" *.c` &:# Change old dates to the ISO format
remplace.exe	| DWL	| Replace any characters by others, including CR and LF.				| `1clip | remplace "; " \r\n | sort` &:# Sort an email distribution list alphabetically
u2a.bat		| -W-	| Convert 16-bits Unicode text to ANSI text.						|
u2w.bat		| -W-	| Convert Unix End-Of-Lines (LF) to Windows End-Of-Lines (CR LF).			| `12 u2w`
w2u.bat		| -W-	| Convert Windows End-Of-Lines (CR LF) to Unix End-Of-Lines (LF).			|

## Manage the Windows system

Name				| OS	|Description										| Example
--------------------------------|-------|---------------------------------------------------------------------------------------|-------------
25.bat				| DW-	| Switches the console to 25 lines x 80 columns. (CGA mode)				| `25`
43.bat				| DW-	| Switches the console to 43 lines x 80 columns. (EGA mode)				| `43`
50.bat				| DW-	| Switches the console to 50 lines x 80 columns. (VGA mode)				| `50`
cascade.tcl			| -W-	| Align a set of similar windows regularly. Uses Twapi.					| `cascade notepad`
Disable-IPv6Components.ps1	| -W-	| Disables Windows IPv6 component. Useful for diagnosing networking issues.		|
driver.exe			| -W-	| Manage Windows 9X and 2000+ drivers.							|
Enable-IPv6Components.ps1	| -W-	| Enables Windows IPv6 component. Useful for diagnosing networking issues.		|
Get-ProductKey.ps1		| -W-	| Display the product key used for activating Windows.					|
hosts.bat			| -W-	| Edit the hosts file with notepad							| `hosts` &:# Must be run as administrator, else the hosts file can't be saved
IESec.ps1			| -W-	| Display Internet Explorer security mode settings.					|
IPcfg.bat			| -W-	| Front end to ipconfig.exe, filtering its output to make it more readable. Numerous options to select the information needed.	| `ipcfg` &:# Display just Ethernet and Wifi interfaces
Is-WindowsActivated.ps1		| -W-	| Test if Windows is activated.								|
Out-ByHost.ps1			| -W-	| Execute PowerShell commands on remote machines in parallel, displaying results like Unix pdsh: That is as pure text lines, prefixed by the remote machine name. Requires installing Out-ByHost.ps1 and Reconnect.ps1 on every target system.		| `Out-ByHost (atlas 1..4) {update S:\tools C:\tools}`
Reconnect.bat			| -W-	| Reconnect network drives. Useful after moving a laptop, or when opening an elevated window.	| `Reconnect S:`
Reconnect.ps1			| -W-	| Reconnect network drives. Useful after moving a laptop, or when opening an elevated window.	| `Reconnect S:`
regx.bat			| -W-	| Manage the registry as if it were a file system. Output formatted as SML.		| `regx dir HKLM\SOFTWARE\Microsoft\Windows`
ShadowCopy.ps1                  | -W-   | Manage volume shadow copies. Includes an option for recyling them like a pool of backup tapes. | `help ShadowCopy.ps1 -detailed`
Test-IPv6Components.ps1		| -W-	| Test if Windows IPv6 component are enabled or disabled. Useful for diagnosing networking issues.	|
Window.ps1			| -W-	| Move and resize windows.								| `Window "Server Manager" -MoveTo 150,150 -Resize 1000,750 -OnTop`

## Manipulate structured trees

XML is good for programs, but hard to read for humans.
JSON is easier to read, but not as powerful as XML.
SML is XML made readable.

Name		| OS	|Description											 | Example
----------------|-------|------------------------------------------------------------------------------------------------|-------------
sml.tcl		| -WL	| Convert XML files to a much simpler structured text format, and back.				 | `type config.xml | sml`
show.tcl	| -WL	| Display files contents, or whole directory trees contents, in a simple structured text format. | `show /proc/fs`
xpath.tcl	| -WL	| Use XPATH to extract data from an XML file.							 | `xpath --dir config.xml /root/display`

## Programmer toolbox

Name		| OS	|Description										| Example
----------------|-------|---------------------------------------------------------------------------------------|-------------
chars.exe	| DWL	| Display a table of ASCII and all 8-bit characters.		 			|
codepage.exe	| -W-	| Get information about the current code pages, or the characters in other ones.	| `codepage 1253` &:# Show characters in code page #1253
echoargs.bat	| DW-	| Display echoargs arguments. Useful to diagnose command-line processing issues.	| `echoargs.bat how" many args "here?`
EchoArgs.ps1	| -W-	| Display echoargs arguments. Useful to diagnose command-line processing issues.	|
errorlev.bat	| D--	| Display last command's errorlevel. (There's no %ERRORLEVEL% variable in DOS.)		|
inicomp.exe	| DWL	| Compare .ini or .reg files. Useful to detect changes in the Windows registry.		| `inicomp server1.reg server2.reg`
MakeZip.bat	| -W-	| Create a zip file, based on a list of files in an input file. Uses 7-zip.		| `makezip tools.lst` &:# Builds tools.zip.
msgbox.exe      | -W-   | Display various types of message boxes, and return answers to the batch.		| `msgbox -x -c "About to erase your disk"`
nlines.tcl      | -WL   | Count lines, and non-commented source lines, in a set of files.                       | `nlines -r` &:# Count recursively in the current dir. and sub-dirs.
PSService.ps1   | -W-   | Sample Windows Service entirely in a PowerShell script.                               | `help PSService.ps1 -detailed` &:# Comprehensive built-in usage doc.
PySetup.bat	| -W-	| Find the Python interpreter even if it's not in the PATH. Configure Windows to run *.py files as command-line scripts. | `pysetup -t` &:# Test if Python is correctly configured
tclsh.bat	| -W-	| Find the Tcl shell even if it's not in the PATH. Configure Windows to run *.tcl files as command-line scripts.	 | `tclsh.bat -t` &:# Test if Tcl is correctly configured
tee.exe 	| DW-	| Duplicate the input from stdin to multiple parallel outputs.				| `dir | tee -a work.log`
TimeX.bat	| -W-	| Time the execution of a command. Similar in spirit to Unix' time command.		| `timex ping -n 1 myserver.mysite.org`
touch.bat       | -W-   | Uses touch.exe if available, else does it in pure batch (slower!).			| `touch myprog.c`
whichinc.exe    | DW-   | Enumerate all include files that a C/C++ source potentially includes.                 | `set INCLUDE=... & whichinc myprog.c`

## Harware and BIOS management

Name		| OS	| Description										| Example
----------------|-------|---------------------------------------------------------------------------------------|-------------
cpuid.exe	| DW-	| Identify the processor, and display its various capabilities.				| `cpuid -v`
gpt.exe         | DW-   | Manage GUID Partition Tables.                                                         | `gpt` &:# Dump the legacy and GUID partition tables
sector.exe      | DW-   | Manage disk sectors. Options for dumping them, or copying them to & from files.       | `sector hd0:` &:# Dump the hard disk 0 boot sector
smbios.exe      | DW-   | Manage the System Management BIOS.                                                    | `smbios -t 0:` &:# Dump SMBIOS table 0
uuid.exe        | DW-   | Manage UUIDs.                                                                         | `uuid -s` &:# Display the system UUID

(More to come!)

## Misc

Name		| OS	|Description										| Example
----------------|-------|---------------------------------------------------------------------------------------|-------------
FixMHT.tcl	| -WL	| Fix relative links in a .mht file. (Works around known bugs in many .mht files.)	|
FlipMails.tcl	| -WL	| Convert mail threads into a single ASCII text file with mails in chronologic order.	| `12 flipmails`
fvlCache.bat	| -W-	| List the most recent files in the Flash Player cache.					|
gcCache.bat	| -W-	| List the most recent files in the Google Chrome cache. Ex: The video you just viewed. |
get.tcl		| -WL	| Get a file from a web server.	Simpler to use than Unix' wget.				| `get "https://cloud.com/picture.jpg`
ie.bat		| -W-	| Start Internet Explorer								|
ieCache.bat	| -W-	| List the most recent files in the Internet Explorer cache.				|
n.bat		| -W-	| Start Notepad										| `n readme.md`

-----------------------------------------------------------------------------------------------------------------------

## Notes

* All Tcl scripts require installing a Tcl interpreter.
This interpreter is standard in Linux, but absent in Windows.
See the README file in SysToolsLib's Tcl directory for instructions on how to install a Tcl interpreter in Windows.

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
