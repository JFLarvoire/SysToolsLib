cmd.exe AutoRun scripts management
==================================

The cmd.exe shell allows defining an optional AutoRun script, that is executed
automatically every time cmd.exe starts.  
See `cmd /?` for information about that capability.

This AutoRun capability has some limitations though:

* Only one AutoRun script may be defined.
* These scripts have undocumented limitations:
   - They must not display anything.
   - They must not change the current directory.
   - They must run very fast.

Not respecting the first two limitations causes hard to debug problems when child cmd.exe shells
are started, for example with a `for /f %%l in ('my_simple_program') do ...` command.  
Not respecting the third severely slows down the other scripts that use lots of sub shells.

AutoRun.cmd is a script that manages cmd.exe AutoRun initializations,
in an easily extensible way inspired by Unix standards.

* It allows using multiple AutoRun scripts.
* It makes these AutoRun scripts independent of each other, so that they can be
  installed or removed in any order, without having to deal with the AutoRun registry key.
* As a side feature, it also defines installation directories, as specified in
  https://www.gnu.org/prep/standards/html_node/Directory-Variables.html


AutoRun.cmd Installation
------------------------

To install AutoRun.cmd, copy it to your favorite cmd scripts directory
(A directory that's in your PATH); Then run as Administrator: `AutoRun -i`  
If you first want to know what this installation would do, run: `AutoRun -X -i`  
In the case that another unrelated AutoRun script is already present, this installation
will detect it, and refuse to run. It's possible to override that by using the -f option.
In that case the previous AutoRun script is moved to one of the AutoRun.cmd.d directories. (See below)


Adding your own AutoRun scripts
-------------------------------

AutoRun.cmd does as little as possible, to run as fast as possible.  
Do not customize it. Instead, put your own AutoRun scripts...

* in `"%ALLUSERSPROFILE%\AutoRun.cmd.d\"` for all users,
* or in `"%USERPROFILE%\AutoRun.cmd.d\"` for yourself only.

AutoRun.cmd will scan these directories, and run all the \*.bat and \*.cmd files there.  
This mechanisn is inspired from the way Linux bashrc and login scripts are
organized, and how they can be extended.  
The advantage is that any number of AutoRun scripts can be installed
independently of each other.

Important: Do not add these AutoRun.cmd.d directories in your PATH.
The scripts there are NOT intended to be run manually.


AutoRun scripts available in SysToolsLib
----------------------------------------

This directory provides a few scripts that may be used as AutoRun extension scripts.

Prerequisite: Install AutoRun.cmd as explained above.

Copy the AutoRun scripts you want to `"%ALLUSERSPROFILE%\AutoRun.cmd.d\"`.

Caution:  
Each AutoRun script slows down cmd.exe startup a little bit.
Only install the AutoRun scripts that you really need!

| Script        | Description										| Example			|
| ------------- | ------------------------------------------------------------------------------------- | ----------------------------- |
| history.bat	| Defines a history macro displaying the command history. Allows piping it to another program. | `history \| findstr /i make` |
| pid.bat	| Defines a PID environment variable, containing the Process ID of the current cmd.exe shell instance. | `echo %PID%` |
| which.cmd	| Redefine the which command, so that it uses `which -i` by default to detect cmd.exe macros.  | `which history` |

Note: `AutoRun -i` installs the autorun extension scripts listed in the `AutoRun.cmd.d\default.lst` file.  
This file contains only the extension scripts that are particularly useful, and that initialize quickly.
For example, the pid.bat command is not included by default, because it noticeably slows down the cmd.exe startup.
