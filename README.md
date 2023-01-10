System Tools Library
====================

Project Description
-------------------

### System Tools

This Library contains many command-line tools for managing Windows and Unix (Ex: Linux, MacOS) systems.
I've built them over 30 years, both for work at HP then HPE, and for home projects.
Some of these tools have unique capabilities, that I hope Windows and Unix power users will find useful.

Major highlights:

- Directory management tools, to search, compare, weight, update, recurse, etc.
  In Windows, they all support Unicode names in any code page, paths > 260 characters, junctions, file & directory symlinks.
- Windows clipboard content filtering tools. They bring the power of the command line to all GUI apps!
- DOS and Windows system management tools. Manage the hardware, BIOS, disks, drivers, etc.
  The Windows tools work in all versions of Windows since Windows 95 (for the 32-bits versions) and Windows XP/64 (For the 64-bits versions).

For a list of available tools and their description, see [Catalog.md](Docs/Catalog.md).

### Development Libraries

This Library also contains several software libraries that developers could find useful:
As many tools shared common features, I've refactored them many times.
The common code is now in libraries, which can be reused in new programs and scripts.

Major highlights:

- A set of powerful script debugging and logging libraries for (in chronologic creation order) Tcl, Batch, Shell, PowerShell, Python.
  They can be added easily to existing scripts, and make it much easier to debug complex interacting scripts.
  More info [here](Docs/System Script Libraries.md).
- A configure.bat/make.bat system for Microsoft Visual C++, allowing to build multiple versions of C tools from a common source,
  with commands familiar to Unix developers. Target environments: BIOS, DOS, WIN95, WIN32, IA64, WIN64, ARM, ARM64.
  More details in the [MsvcLibX documentation](C/MsvcLibX/README.md).
- A set of debugging macros for C programs, similar in use and effect to those for scripting languages. [debugm.h](C/include/debugm.h)
- MsvcLibX.lib - A Microsoft C library eXtension, implementing many modern Unix C Library functions that Microsoft never provided.
  It supports UTF-8 sources that work in any code page, paths > 260 characters, junctions, file & directory symlinks.
  This makes it easy to write C system management tools that build in both Unix and Windows. More info [here](C/MsvcLibX/README.md).
- SysLib.lib - A set of system management functions, with versions for DOS, Windows, and Unix.
- Bios.lib - A library for writing C or C++ programs using only the legacy BIOS: Option ROMs, OS boot loaders, MS-DOS drivers, or TSRs.
- LoDos.lib - A library for the specific needs of MS-DOS drivers and TSRs.
- PMode.lib - A libray for switching BIOS, DOS or Windows 95 programs to the 80x86 protected mode.

For more details on these C libraries, see [C/README.md](C/README.md), and the README.md files in the C/ subdirectories.

Programming languages:

Old tools were mostly written in C/C++. Then I started using scripting languages more often:
Batch and PowerShell for Windows-only tools; Python and Tcl for cross-OS tools; Shell for Unix-only tools.

Jean-François Larvoire
jf.larvoire@free.fr
2023-01-10


Installation
------------

### Scripts and programs in Windows

You can use the WIN32 release files on all versions of Windows.

* Go to the project release area.
* Download the most recent SysTools.zip release.
* Extract files from that zip file, and put them in a directory in your PATH. For example in C:\Windows.
* The Tcl scripts require installing a Tcl interpreter. See [Tcl/README.md](Tcl/README.md) for details on how to do that.

You can also rebuild all C programs from sources. More details about this further down.

### Scripts and programs in Unix

The C programs need to be rebuilt from source.
As for scripts, only the Shell and Tcl directories contain useful scripts in Unix.

* Download the project sources archive.
* Extract files from that archive, and put them in a new work directory.
* Run `make` in that work directory to rebuild the C programs. (There's no ./configure script.)
* Then run `sudo make install` to install them and the Tcl and Shell scripts.
  If you're on the cautious side, you can first dry-run the installation using `sudo make -n install`.
  Individual scripts and programs can also be installed separately by running `sudo ./install PROGNAME`.

### Development environment

The C development environment is designed so that a Windows and a Unix system (possibly one a VM inside the other)
can share the same sources, and output executables in distinct target-OS-specific subdirectories.

* Download the project sources archive.
* Extract files from that archive, and put them in a new work directory.
* See the [C/README.md](C/README.md) file for more information on how to rebuild C programs for each OS.

#### Files description

See the README.txt or README.md file in each subdirectory for more details about that particular library,
and further subdirectories.

Name             | Description
---------------- | -------------------------------------------------
C/               | Programs and libraries written in C or C++
Batch/           | Scripts and libraries in the Batch language
Docs/            | Project documentation
PowerShell/      | Scripts and libraries in the PowerShell language
Python/          | Scripts and libraries in or for the Python language
Shell/           | Scripts and libraries in the Posix Shell language
Shell/profile.d/ | Posix Shell initialization scripts to install into /etc/profile.d
Tcl/             | Scripts and libraries in or for the Tcl language

Particular files:

Name            | Description
--------------- | ---------------------------------------------------------------
README.md       | This file
NEWS.md         | Project history
LICENSE.txt     | Project license (Apache 2.0)
*.lst           | List of files that go into source or binary releases. Used by MakeZip.bat and `make release`.


Tools Usage
-----------

All tools (both scripts and C programs) support the -? option for help, and most share a few other common options:

Option  | Description
------- | -----------------------------------------------------------------------------
  -?    | Display a help screen. (In Unix shells that swallow ? characters, use the -h alias)
        |  
  -d    | Display debug information: Help the author understand what code is running.
  -v    | Display verbose information: Help users understand what the program is doing.
  -V    | Display the script version and exit.
  -X    | No Exec mode: Display what the program would do, but don't do it.
        |  
  -A    | Force ANSI encoding output. (Windows only)
  -O    | Force OEM encoding output. (Windows only)
  -U    | Force UTF8 encoding output. (Windows only)

All make files support a `make help` target, which displays a help screen with available make variables and targets.


Contributing
------------

Most of the development work was done by Jean-François Larvoire during work hours at HP then HPE.
It is thus now HPE that is the copyright owner of this code.
HPE legal authorized in March 2016 for a first part, October 2016 for a second part, and February 2017 for the last part,
the open-source release of this code, provided that future contributors agree with the following conditions:

- Contributions from independent individuals are welcome under the rules of the Apache 2.0 license.
- Contributions from employees of outside (non-HPE) companies will only be accepted after
  they've signed HPE's Corporate Contributor License Agreement in file [HPE_CCLA.docx](HPE_CCLA.docx).


License
-------

Copyright 2016-2020 Hewlett Packard Enterprise

All files in this distribution are licensed under the Apache License version 2.0.
You may not use any of these files except in compliance with this License.
You may obtain a copy of the Apache License version 2.0 at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under this License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing its permissions and
limitations.
