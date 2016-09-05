# System Tools Library

## Project Description

### System Tools

This Library contains many command-line tools for managing Windows and Linux systems.  
I've built them over 30 years, both for work and for home projects.  
Some of these tools have unique capabilities, that I hope Windows and Linux power users will find useful.

Major highlights:

- Directory management tools, to search, compare, weight, update, recurse, etc.  
  They all support Unicode names in any code page, paths > 260 characters, junctions, file & directory symlinks.
- Windows clipboard content filtering tools. They bring the power of the command line to all GUI apps!
- System management tools. Manage the hardware, BIOS, hard disks, drivers, etc.

More info [here](Docs/Catalog.md).

### Development Libraries

This Library also contains several software libraries that developers should find useful:  
As many tools shared common features, I've refactored them many times.  
The common code is now in libraries, which can be reused in new programs and scripts.  

Major highlights:

- A set of powerful script debugging and logging libraries for (in chronologic creation order) Tcl, Batch, Bash, PowerShell.  
  They can be added easily to existing scripts, and make it much easier to debug complex interacting scripts.  
  More info [here](Docs/System Script Libraries.md).
- A configure.bat/make.bat system for Microsoft Visual C++, allowing to build multiple versions of C tools from a common source, 
  with commands familiar to Unix developers. Targets: DOS, WIN95, WIN32, IA64, WIN64, ARM. More details in the MsvcLibX documentation.
- MsvcLibX.lib - A Microsoft C library eXtension, implementing many Unix C Library functions that Microsoft never provided.  
  It supports UTF-8 sources that work in any code page, paths > 260 characters, junctions, file & directory symlinks.  
  This makes it easy to write C system management tools that build in both Unix and Windows. More info [here](C/MsvcLibX/README.txt).

Programming languages:

Old tools were mostly written in C/C++. Then I started using scripting languages more often:  
Batch and PowerShell for Windows-only tools; Tcl for cross-OS tools; Bash for Linux-only tools.

Jean-François Larvoire
jf.larvoire@hpe.com
2016-09-05


## Installation

### Scripts and programs in Windows

You can use the WIN32 release files on all versions of Windows.

* Go to the project release area.
* Download the most recent SysTools.zip release.
* Extract files from that zip file, and put them in a directory in your PATH. For example in C:\Windows.
* The Tcl scripts require installing a Tcl interpreter. See [Tcl/README.txt](Tcl/README.txt) for details on how to do that.

### Scripts and programs in Linux

The C programs need to be rebuilt from source.
As for scripts, only the Tcl directory contains useful scripts for Linux.

* Download the project sources archive.
* Extract files from that archive, and put them in a new work directory.
* Make sure the scripts in the Bash and Tcl subdirectories are executable, using the chmod +x command.
* Copy the Tcl scripts into a directory in your PATH. For example in /usr/local/bin.
* Remove the .tcl extension on all these tcl scripts (except Library.tcl), so that they can be invoked with just their base name.
* Go to the C/SRC subdirectory, and run the following commands to rebuild the C programs:

        cd $WORKDIR/C/SRC
        chmod +x configure
        ./configure
        make

* Copy the executable files from the output subdirectory, into the same /usr/local/bin.

### Development environment

The C development environment is designed so that a Windows and a Linux system (possibly one a VM inside the other) 
can share the same sources, and output executables in distinct target-OS-specific subdirectories.

* Download the project sources archive.
* Extract files from that archive, and put them in a new work directory.
* See the (C/README.txt)[C/README.txt] file on how to rebuild C programs for each OS.

#### Files description

See the README.txt file in each subdirectory for more details about that particular library, and further subdirectories.

Name            | Description
--------------- | -------------------------------------------------
C/		| Programs and libraries written in C or C++
Bash/		| Scripts and libraries in the Bash language
Batch/		| Scripts and libraries in the Batch language
Docs/		| Project documentation
PowerShell/	| Scripts and libraries in the PowerShell language
Tcl/		| Scripts and libraries in the Tcl language

Particular files:

Name            | Description
--------------- | ---------------------------------------------------------------
README.md	| This file
NEWS.txt	| Project history
LICENSE.txt	| Project license (Apache 2.0)
*.lst		| MakeZip.bat input files for generating partial source releases. (Deprecated)


## Tools Usage

All tools (both scripts and C programs) support the -? option for help, and most share a few other common options:

Option | Description
------ | -----------
  -?   | Display a help screen.
       |
  -d   | Display debug information: Help the author understand what code is running.
  -v   | Display verbose information: Help users understand what the program is doing.
  -V   | Display the script version and exit.
  -X   | No Exec mode: Display what the program would do, but don't do it.
       |
  -A   | Force ANSI encoding output. (Windows only)
  -O   | Force OEM encoding output. (Windows only)
  -U   | Force UTF8 encoding output. (Windows only)


## Contributing

Most of the development work was done by Jean-François Larvoire during work hours at HP then HPE.
It's thus HPE that is the copyright owner of this code.  
HPE legal authorized in March 2016 the open-source release of this code, 
provided that future contributors agree with the following conditions:

- Contributions from independent individuals are welcome under the rules of the Apache 2.0 license.
- Contributions from employees of outside (non-HPE) companies will only be accepted after
  they've signed HPE's Corporate Contributor License Agreement in file HPE_CCLA.docx.


## License

Copyright 2016 Hewlett Packard Enterprise

All files in this distribution are licensed under the Apache License version 2.0.
You may not use any of these files except in compliance with this License.
You may obtain a copy of the Apache License version 2.0 at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under this License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing its permissions and
limitations.
