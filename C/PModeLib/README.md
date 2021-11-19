The PMODE.LIB Library
=====================

Introduction
------------

This library was created to simplify the development of BIOS, DOS, or Windows 95 programs that need to switch to the
80286 and 80386 protected mode.  
This is a complex task because of the complexity of the protected mode of the x86 processors, of the many standards
created over the years for managing it, and also because of the various compatibility problems in the PC world.

Some of its functions depend on the BIOS.LIB library, documented separately.

Build Procedure
---------------

First, build BIOS.LIB, and make sure that the environment variable BIOSLIB has been set to point at BIOS.LIB work
directory. (This should be automatically done by the configure.bat script. Check its output.)  
Extract all files from PMODELIB.ZIP, or from the Source Control server, into to your work directory. Ex: C:\SRC\PMODELIB.  
Go to that directory and run MAKE.BAT. If MAKE.BAT cannot find your assembler or C compiler, run CONFIGURE.BAT with the
right options to declare their location. Run `CONFIGURE -?` for a list of all options.  
Object files and list files are stored in subdirectories OBJ and LIST respectively.

Significant source files:

Name                    | Description
------------------------|-----------------------------------------------------------------------
CONFIGURE.BAT           | Locate your build tools & generate CONFIG.%COMPUTERNAME%.BAT.
CONFIGURE.PMODE.BAT     | Defines PMODE.LIB-specific configuration settings.
MAKE.BAT                | Batch file to build PMODE.LIB at the command prompt.
NMakefile               | Rules for building PMODE.LIB with MAKE.BAT.
PMODE.INC               | Pmode.lib definitions for assembly language sources.
PMODE.H                 | Pmode.lib definitions for C language sources.
VMM16.H                 | VMM Definitions for use by 16 bits programs.
WINDEF.H                | Define common Windows data types.
