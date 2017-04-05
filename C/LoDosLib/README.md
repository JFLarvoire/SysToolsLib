The LODOS.LIB Library
=====================

Introduction
------------

This library was created to simplify the development of low-level MS-DOS programs:  
MS-DOS device drivers, MS-DOS TSRs, and low level DOS utilities.  
It works in conjunction with BIOS.LIB, described separately.

The library is structured with one module per routine. This means that only the routines actually used by a program will
be linked with it. This ensures a minimal size for executables, without any work from the developer.

There are just a few include files:

Name	 | Description
---------|-------------------------------------------------------------------
lodos.h	 | General definitions and prototypes.
dosdrv.h | Definitions for MS-DOS device drivers.
msdos.h	 | MS-DOS structures and parameters.
dos.inc	 | MS-DOS assembly language macros.

To use LODOS.LIB, a program should add the LODOSLIB directory to the list of include file directories.  
C language modules must include lodos.h.  
Assembler modules must use H2INC to convert the definitions to a suitable format.  
No include file from the standard C library should be used, to avoid the risk of inadvertently dragging in the Microsoft
C libraries.

Structural limitations:

- The library only supports small and tiny C memory models. Other models would require extensive changes to the assembly
  language routines. Sorry. Note that this has never been a limitation so far. Far data segments are supported, provided
  the "far" keyword is used when referencing data in them. To use the tiny model, the following line must be added in
  the startup module after the inclusion of ADEFINE.INC:  
    DGROUP   GROUP   RESID, _TEXT, CONST, _DATA, _BSS, C_COMMON, STACK
- The library was compiled with the /Gr option, that is the register calling convention. This means the first three
  arguments to a function are passed in registers instead of the stack. Extensive testing for the Vectra 386s/20 Setup
  in ROM has shown that this results in the smallest program size. Programs using BIOS.LIB and LODOS.LIB  must also use
  this convention. This is completely transparent for the C modules. But this has impact on assembly language modules,
  which must use the macros CFASTPROC, ENDCFASTPROC and EXTERNCFASTPROC from ADEFINE.INC to define or reference public
  procedures.
- The library uses 286 instructions. Sorry for HP 95LX aficionados, but they can't use code linked with LODOS.LIB in
  their favorite palmtop. Again, this was done for code compactness reasons.

 
Optimizations:

The following switches are used: "/G2rs /Oaes /Zpil". This particular set has been carefully selected after extensive
testing for the Vectra 386s/20. This is the set that produced the smallest executable AFTER compression by PkLite.
It does not produce the smallest executable before compression, but the goal was to fit as much compressed code as
possible in a 32 KB ROM.  
The figures were obtained with Microsoft C 6.0, and may not be valid with C 8.0 (i.e.. Visual C++) anymore.

Startup modules:

Every C program requires an assembly language startup module. This startup module prepares the local heap, the stack,
all segment registers, processes the command line, opens standard files, then calls the C main routine. The startup
module contains the DOS start address, defined by a .STARTUP directive, or by the label listed on the END line. It also
contains the _acrtused dummy public constant, which is referenced be all C object files to force pulling a startup
module from the standard C library.  
The LODOSLIB directory contains modules STARTDEV.ASM and STARTEXE.ASM which can be used as a startup module for .SYS
and .EXE programs respectively. They can be used without change. For .COM programs, use STARTCOM.ASM in BIOSLIB.  
Use module STARTDEV.ASM for .SYS device drivers. In the case of .EXE device drivers, it is necessary to use both
STARTDEV and STARTEXE.  
For .COM programs and all device drivers, the startup module must also be first in the list of object modules linked.
This is important because execution starts at the beginning of the memory image (.COM programs) or because a driver
DeviceHeader structure must be at the beginning of the memory image (device drivers). Also the first module linked
defines the segment ordering, and we define a non-standard RESID segment for code that will remain resident for TSRs
and drivers, that must be ahead of all standard C segments. See the header of ADEFINE.INC for more details.

Tools:

MASM 6.0a or later  
MSC 6.0 or later (Currently using C 8.00c)  
Newer versions of the C and assembler should be OK. The risk is with the segments naming. If this has changed,
ADEFINE.INC will have to be modified, and the whole library recompiled. Then the startup module must be changed
accordingly, and the application recompiled too.


Build Procedure
---------------

First, build BIOS.LIB, and make sure that the environment variable BIOSLIB has been set to point at BIOS.LIB work
directory. (This should be automatically done by the configure.bat script. Check its output.)  
Extract all files from LODOSLIB.ZIP, or from the Source Control server, into to your work directory. Ex: C:\SRC\LODOSLIB.  
Go to that directory and run MAKE.BAT. If MAKE.BAT cannot find your assembler or C compiler, run CONFIGURE.BAT with the
right options to declare their location. Run "CONFIGURE -?" for a list of all options.  
Object files and list files are stored in subdirectories OBJ and LIST respectively.

Significant source files:

Name			| Description
------------------------|-----------------------------------------------------------------------
CONFIGURE.BAT		| Locate your build tools & generate CONFIG.%COMPUTERNAME%.BAT.
CONFIGURE.LODOSLIB.BAT	| Defines LODOS.LIB-specific configuration settings.
DOS.INC			| MS-DOS definitions from successive DOS programmer’s references.
LODOS.H			| C Definition for the subset of the standard C library supported
MAKE.BAT		| Batch file to build LODOS.LIB at the command prompt.
NMakefile		| Rules for building LODOS.LIB with MAKE.BAT.
STARTCOM.ASM		| Sample startup routine for .EXE programs. Not part of LODOS.LIB.
STARTDEV.ASM		| Sample startup routine for device drivers. Not part of LODOS.LIB.
