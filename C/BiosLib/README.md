The BIOS.LIB Library
===================

Introduction
------------

This library was created to simplify the development of low level 16-bits x86 programs that use the PC BIOS, and cannot
use MS-DOS. This is a requirement for Option ROM programs, OS boot loaders, MS-DOS device drivers, MS-DOS TSRs.

These kinds of programs used to be written entirely in assembly language. The problem with C was that most routines in
Microsoft's standard C library do rely on DOS, either directly or indirectly.

With BIOS.LIB, these kinds of program can be written mostly in C, using standard C library routines for most practical
purposes. This makes it easy to port code from the DOS world. It is also easy to test code under DOS, then port it to
run within the BIOS. Note that the MS-DOS device drivers and TSRs do have specific additional needs, addressed in
library LODOS.LIB, described separately.

The oldest routine dates from 1987, but most were developed in 1990 for the HP Vectra 386/16N Setup in ROM. After being
reused and updated in several other projects, they were repackaged as a library called NODOS.LIB in 1993. Finally I
renamed it as BIOS.LIB in 2016 to clarify its main purpose. A benefit of this long history is that the code is heavily
field-tested, and can be fully relied upon.

In the second half of the 1990s a new usage pattern emerged: I needed to shoehorn as many system management tools as
possible into my Universal Boot Disks. These UBDs were floppy disks used to boot PCs for maintenance tasks. They were
also used as boot images for maintenance CDs. Many of these tools had actually no dependancy on DOS, and could be made
significantly smaller by linking them against BIOS.LIB. The concept of MINICOM programs was born.

In the early 2000s, one last use case appeared: OS boot loaders. HP support needed a way to multi boot PCs into various
support environments incompatible with each other. The BIOS.LIB library allowed to build a powerful OS-independant boot
menu that solved the issue. 

Finally in 2015 I noticed that I could merge my MINICOM build directory into the new generic configure.bat/make.bat
system that I had developed for DOS/WIN32/WIN64 targets. By defining a new target "OS" type called BIOS, I could
automatically invoke a bios.mak script, and generate MINICOMs at will. Rebuilding DOS drivers, TSRs, or BIOS option ROMs
this way is also certainly possible, but this has not been tested due to lack of immediate need.


Description
-----------

The library is structured with one module per routine. This means that only the routines actually used by a program will
be linked with it. This ensures a minimal size for executables, without any work from the developer. Having the smallest
possible program size is a critical goal for all target types of programs (Option ROMs, DOS drivers and TSRs, MINICOMs)

There are just a few include files with all the main definitions:

Name	    | Description
------------|-------------------------------------------------------------------
adefine.inc | All assembly language definitions.
clibdef.h   | C definitions for the subset of the standard C library available.
utildef.h   | Other C definitions, mostly BIOS and hardware related.

A few standard C library include files are also provided, but they're just front ends to clibdef.h.  
They allow compiling MINICOM programs using the standard C library either for the BIOS or for DOS, without having to
use conditional compilations to include clibdef.h in one case, or the standard include files in the other.

Recommended:  
- C sources that are intended to run only in the BIOS should only include clibdef.h and/or utildef.h.  
- C sources that are to be built either for the BIOS or for DOS must use only standard include files.  
- Assembler sources must include adefine.inc.

To use BIOS.LIB, a program should add the BIOSLIB directory to the list of include file directories.  
If you do use the standard C library include files, make sure that your INCLUDE path does _not_ include the MSVC INCLUDE
directory when you build for the BIOS. Else there would be a risk that the wrong set of include files is used.  

Structural limitations:

- The library only supports small and tiny C memory models. Other models would require extensive changes to the assembly
  language routines. Sorry. Note that this has never been a limitation so far. Far data segments are supported, provided
  the "far" keyword is used when referencing data in them. To use the tiny model, the following line must be added in
  the startup module after the inclusion of ADEFINE.INC:  
    DGROUP   GROUP   RESID, _TEXT, CONST, _DATA, _BSS, C_COMMON, STACK
- The library was compiled with the /Gr option, that is the register calling convention. This means the first three
  arguments to a function are passed in registers instead of the stack. Extensive testing for the Vectra 386s/20 Setup
  in ROM has shown that this results in a smaller program size. Programs using BIOS.LIB must also use this convention.  
  This is completely transparent for the C modules. But this has impact on assembly language modules, which must use the
  macros CFASTPROC, ENDCFASTPROC and EXTERNCFASTPROC from ADEFINE.INC to define or reference public procedures.
- The library uses 286 instructions. Sorry for HP 95LX aficionados, but they can't use code linked with BIOS.LIB in
  their favorite palmtop. Again, this was done for code compactness reasons.

Main functional limitations:

- The printf and sprintf function support only a subset of the standard format strings.
- The printf function output is limited to 1024 characters.
- The malloc function performs no heap size checking. It may collide into the stack. There is no corresponding free function.
- There are no file functions. Obviously since the goal is to work without DOS...
- The output to the display cannot be redirected to a file when run under DOS. This is a consequence of the previous
  limitation: The output cannot go to stdout, which is a file.

Long integers support:

Additions and subtractions are managed directly by the C compiler. They can be used without restriction.  
All other operations are done through calls to the math library. BIOS.LIB provides functions for multiplication,
division, remainder and left and right shifts for both signed and unsigned operations. The support routines are called
_aNul??? for unsigned long operations, and _aNl??? for signed long operations.  
Be careful when using long integers in resident modules. The _aN????? routines must be manually rewritten to reside in
the resident segment too.  
Warning: 32-bits multiplication, division and remainder are implemented using 80386 instructions. Modify your startup
module to prevent running on a PC with less than a 80386 if needed. ECX and the high halves of EAX and EDX are destroyed.
Other registers are preserved, as expected by the compiler.  
It is recommended that the programs use when possible, the routines regle_de_trois, mul16x16to32 and div32x16to16.
They are small, efficient, and compatible with the 80286.  

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
The BIOSLIB directory contains module STARTCOM.ASM, which can be used as a startup module for .COM programs. It can be
used without change for MINICOMs. Leverage the STARTCOM.ASM module for existing device drivers based on BIOS.LIB if
needed. This usually requires extensive adaptations.  
STARTCOM.ASM (like STARTEXE.ASM and its sister startup modules in LODOSLIB) does not preprocess the command line. It's
BIOS.LIB's routine cmain() that does. If your program does not use arguments, you can save memory by calling your main
routine cmain instead of main.  
For .COM and .SYS programs, the startup module must also be first in the list of object modules linked. This is
important because execution starts at the beginning of the memory image (.COM) or because a driver DeviceHeader
structure must be at the beginning of the memory image (.SYS). Also the first module linked defines the segment ordering,
and we define a non-standard RESID segment for code that will remain resident for TSRs and drivers, that must be ahead
of all standard C segments. See the header of ADEFINE.INC for more details.

Build Tools:

MASM 6.0a or later  
MSC 6.0 or later (Currently using C 8.00c, as part of MSVC 1.52c, the last version with DOS support.)  
Newer versions of the C and assembler should be OK. The risk is with the segments naming. If this has changed,
ADEFINE.INC will have to be modified, and the whole library recompiled. Then the startup module must be changed
accordingly, and the application recompiled too.

C++:

The library has only minimal support for C++ sources:  
Include files do contain the necessary extern "C" directives. So it's possible to include them directly.
But the library does NOT contain the internal routines implicitly called by the compiler for most non-trivial C++
extensions. For example, the C++ "new" keyword generates calls to routines not implemented in BIOS.lib.  
Note: As a makeshift, use the NEW(object) macro defined in clibdef.h instead of the C++ new keyword. It is usable in
both C and C++ sources.

The only meaningful development using C++ so far has been one using EFI structures. This required having a 64-bits
integer type. As far as I know, Visual C++ 1.5 does not have one. A C++ class, called QWORD, has been used successfully
for that.

Build Procedure
---------------

Extract all files from BIOSLIB.ZIP, or from the Source Control server, into to your work directory. Ex: C:\SRC\BIOSLIB  
Go to that directory and run CONFIGURE.BAT then MAKE.BAT. If CONFIGURE.BAT cannot find your assembler or C compiler,
run it with the right options to declare their location. Run `CONFIGURE -?` for a list of all options.  
Object files and list files are stored in subdirectories OBJ and LIST respectively.

Significant source files:

Name			| Description
------------------------|-----------------------------------------------------------------------
ADEFINE.INC		| Assembly language definitions
CLIBDEF.H		| C Definition for the subset of the standard C library supported
CONFIGURE.BAT		| Locate your build tools & generate CONFIG.%COMPUTERNAME%.BAT.
CONFIGURE.BIOSLIB.BAT	| Defines BIOS.LIB-specific configuration settings.
MAKE.BAT		| Batch file to build BIOS.LIB at the command prompt.
NMakefile		| Make file for BIOS.LIB
SPRINTF.C		| The most useful routine of them all
STARTCOM.ASM		| Sample startup routine for .COM programs. Not part of BIOS.LIB.
UTILDEF.H		| All C Definition not in CLIBDEF.H, ie not in the standard C library.
