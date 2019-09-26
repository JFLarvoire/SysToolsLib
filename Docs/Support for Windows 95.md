Support for Windows 95/98/ME
---------------------------------

The text below mentions Windows 95, but is applicable to Windows 98 and Windows ME as well.

The System Tools Library C programs can be built as WIN32 executables for Windows 95.  
For documentation about how to build them for Windows 95, please see [../C/SRC/README.txt](../C/SRC/README.txt)  
These executables end up in a bin\WIN95 subdirectory.

But before using them on a Windows 95 system, it's necessary to apply the instructions below.

Note that the DOS version of the tools can also be used in Windows 95.  
But this is not recommended, as the DOS tools currently do not support long file names.

### The problem

These WIN95 executables uses Unicode internally for strings, like all other SysToolsLib builds for Windows.

But Windows 95 only has a very limited support for Unicode built in.  

So most of these WIN95 executables will not work by default in Windows 95.

### The solution

To allow them to work, it is necessary to download from Microsoft a "Microsoft
Layer for Unicode on Windows 95/98/ME Systems" (MSLU for short), and install it
on the Windows 95 system.  

See the following links for details:

- https://en.wikipedia.org/wiki/Microsoft_Layer_for_Unicode
- https://msdn.microsoft.com/en-us/goglobal/bb688166.aspx

MSLU installation procedure:

- Download the MSLU redistributable setup (unicows.exe) from:  
  http://go.microsoft.com/fwlink/?LinkId=14851
- Extract unicows.dll from the unicows.exe archive.
- Copy that unicows.dll to the Windows 95 system, into %windir%\System.

## Other recommendations

### Testing WIN95 executables in a Windows 95 VM

The difficulty is how to transfer files from the host to the guest Windows 95 in the VM.

#### Using VMware Player

VMware Player does not have Windows 95 drivers for the HGFS file system.  
This prevents accessing the host's files directly as network files, as is
usually done for Windows XP and later versions of Windows.  
It is not possible to use network shares either, as Windows 95 only supports
the SMB 1 protocol, which is actively blocked by Windows Vista and later hosts.

It is possible, but inconvenient, to transit through a web server, and download
the files in the Windows 95 VM using Internet Explorer 4.

Another possible solution is to transit through a floppy or CD image, and
mount that image in the VMware Player virtual floppy or CD player.  
Many tools can create CD images.  
A very convenient shareware called [WinImage](http://www.winimage.com/download.htm)
allows to create floppy images.

But the best solution as of 2019 is to use an HGFS driver for DOS.  
Try using VMSMOUNT for DOS:

What               | Where
------------------ | ----------------------------------------------
Download page      | https://www.eduardocasino.es/files/vmsmount/
Alternate download | https://sourceforge.net/projects/vmsmount/
Home page          | https://www.eduardocasino.es/code/

Installation: Add this line to your autoexec.bat and reboot the VM:  
(Changing the path as needed, and the host drive letter (U: here) as desired.)

    C:\Tools\VMSMOUNT\vmsmount.exe /L:U /LFN

Known limitation: Long File Names on the host are only partially supported.
But this is just a minor annoyance, compared to the ease of moving files to and from the host directly.
