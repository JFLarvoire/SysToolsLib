###############################################################################
#									      #
#   File name:	    bios.mak						      #
#									      #
#   Description:    A NMake makefile to build BIOS and MINICOM programs.      #
#									      #
#   Notes:	    Use with make.bat, which defines the necessary variables. #
#		    Usage: make -f bios.mak [definitions] [targets]	      #
#									      #
#		    MINICOM programs are DOS .com programs that _only_ use    #
#		    BIOS function calls, defined in the BiosLib library.      #
#		    This allows testing _in DOS_ programs intended to	      #
#		    eventually reside in BIOS option ROMs.		      #
#		    This also allows creating minimized DOS programs (Hence   #
#		    the name MINICOM), much smaller than the ones linked with #
#		    MSVC's standard C library. This can be very useful on     #
#		    systems with extremely limited storage space, like JFL's  #
#		    Universal Boot Disks (UBD).				      #
#		    							      #
#		    The LoDosLib library allows building hybrid programs,     #
#		    using mostly BIOS calls, and a few selected DOS calls.    #
#		    It's useful for building DOS drivers and TSRs, that need  #
#		    to be as small as possible, and can call only a few       #
#                   limited DOS functions.                                    #
#		    							      #
#		    The PMode library allows building BIOS & DOS programs     #
#		    that switch between the x86 real, v86, and protected mode.#
#		    							      #
#		    The SysLib library defines a set of utility routines      #
#		    usable in all environments.				      #
#		    							      #
#		    Targets:						      #
#		    clean	    Erase all files in the BIOS subdirectory. #
#		    {prog}.com	    Build BIOS[\Debug]\{prog}.com.	      #
#		    {prog}.obj	    Build BIOS[\Debug]\OBJ\{prog}.obj.	      #
#		    BIOS\{prog}.com       Build the BIOS release version.     #
#		    BIOS\Debug\{prog}.com Build the BIOS debug version.       #
#		    BIOS\OBJ\{prog}.obj       Compile the BIOS release version.
#		    BIOS\Debug\OBJ\{prog}.obj Compile the BIOS debug version. #
#									      #
#		    Command-line definitions:				      #
#		    DEBUG=0	 Build the release ver. (<=> program in BIOS) #
#		    DEBUG=1	 Build the debug ver. (<=> pgm in BIOS\DEBUG) #
#		    MEM=T	 Build the tiny ver.  (<=> objects in OBJ\T)  #
#		    MEM=S	 Build the small ver. (<=> objects in OBJ\S)  #
#		    MEM=L	 Build the large ver. (<=> objects in OBJ\L)  #
#		    OUTDIR=path  Output to path\BIOS\. Default: To bin\BIOS\  #
#		    PROGRAM=name Set the output file base name		      #
#									      #
#		    The MEM variable is left to minimize differences with     #
#		    the DOS.MAK make file. But the default value T should     #
#		    work in all cases here.				      #
#		    							      #
#		    Likewise, rules for building .exe targets are left in.    #
#		    They should never be needed either.			      #
#		    							      #
#		    If a specific target [path\]{prog}.com is specified,      #
#		    includes the corresponding {prog}.mak if it exists.       #
#		    This make file, defines the files to use beyond the       #
#		    default {prog}.c/{prog}.obj; Compiler options; etc.       #
#		    SOURCES	Source files to compile.		      #
#		    OBJECTS	Object files to link. Optional.		      #
#		    PROGRAM	The node name of the program to build. Opt.   #
#		    EXENAME	The file name of the program to build. Opt.   #
#		    SKIP_THIS	Message explaining why NOT to build. Opt.     #
#									      #
#		    In the absence of a {prog}.mak file, or if one of the     #
#		    generic targets is used, then the default Files.mak is    #
#		    used instead. Same definitions.			      #
#									      #
#		    Note that these sub-make files are designed to be	      #
#		    OS-independant. The goal is to reuse them to build	      #
#		    the same program under Unix/Linux too. So for example,    #
#		    all paths must contain forward slashes.		      #
#									      #
#		    Another design goal is to use that same bios.mak	      #
#		    in complex 1-project environments (One Files.mak defines  #
#		    all project components); And in simple multiple-project   #
#		    environments (No Files.mak; Most programs have a single   #
#		    source file, and use default compiler options).	      #
#									      #
#		    The following macros / environment variables must be      #
#		    predefined. This allows to use the same makefile on       #
#		    machines with various locations for the build tools.      #
#									      #
#		    AS	    	16-bits Assembler			      #
#		    CC16    	16-bits C compiler			      #
#		    INCPATH16  	16-bits include files paths		      #
#		    LINK16  	16-bits Linker				      #
#		    LIBPATH16   16-bits libraries paths			      #
#		    LIB16   	16-bits librarian     			      #
#		    MAPSYM	16-bits Linker .map file to .sym converter    #
#		    TMP	    	Temporary directory	 		      #
#									      #
#  History:								      #
#    2015-10-23 JFL Adapted from DOS.mak.                                     #
#    2015-11-03 JFL Added rules to build a library from a .mak file.          #
#    2015-11-13 JFL Use new generic environment definition variables.         #
#    2015-12-07 JFL Added support for a base output directory other than .\   #
#    2016-04-01 JFL Do not change the PROGRAM value, once it has been set.    #
#		    Added an inference rule for compiling resident C modules. #
#    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              #
#    2016-04-14 JFL Forward HAS_<lib> flags to the C compiler.		      #
#    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      #
#    2016-09-02 JFL Added scripts for removing the UTF-8 BOM from C sources.  #
#    2016-09-21 JFL Fixed an issue that caused double definition warnings.    #
#    2016-09-28 JFL Display FAILED messages when compilation or link fails.   #
#		    Avoid having the word "Error" in the log unnecessarily.   #
#    2016-10-04 JFL Use the shell PID to generate unique temp file names.     #
#		    Display messages only if variable MESSAGES is defined.    #
#    2016-10-11 JFL Adapted for use in SysToolsLib global C include dir.      #
#    2017-03-02 JFL Fixed src2objs.bat and use it indirectly via src2objs.mak.#
#    2017-08-29 JFL Bugfix: The help target did output a "1 file copied" msg. #
#    2017-10-22 JFL Changed OUTDIR default to the bin subdirectory.           #
#    2018-01-12 JFL Added $(LIBRARIES) to the $(PROGRAM).* dependency list,   #
#		    so that it is relinked if one of its libraries changes.   #
#		    The LIBRARIES variable is generated by lib2libs.mak/.bat  #
#		    based on the LIB and LIBS variables.		      #
#    2018-02-28 JFL Added $(LSX) Library SuffiX definition.		      #
#    2018-03-02 JFL Added variable SKIP_THIS, to prevent building specific    #
#		    versions.						      #
#    2022-11-25 JFL Catch up on numerous changes and bug fixes.               #
#    2022-12-03 JFL Removed all inference rules, and use DOS.MAK's instead.  #
#		    							      #
#      © Copyright 2016-2018 Hewlett Packard Enterprise Development LP        #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF !DEFINED(T)
T=BIOS				# Target OS
!ENDIF

!IF !DEFINED(T_VARS)
T_VARS=1	# Make sure OS-type-specific variables are defined only once

T_DEFS=/D_BIOS /DMINICOMS	# Tell sources what environment they're built for

# Memory model for 16-bit C compilation (T|S|C|D|L|H)
!IF !DEFINED(MEM)
MEM=T				# Memory model for C compilation
MEM_ORIG=default
!ELSEIF !DEFINED(MEM_ORIG)
MEM_ORIG=user-defined
!ENDIF

EXE=com				# Default program extension

STARTCOM=$(BIOSLIB)\$(OUTDIR)\OBJ\startcom.obj
STARTEXE=$(LODOSLIB)\$(OUTDIR)\OBJ\startexe.obj

CODEPAGE=cp437			# All ROM BIOSs use that code page

# Tools and options
CGFLAGS=/G2rs /Oaes /Zpil	# C code generation flags

LFLAGSX=/nod			# Extra linker flags

INCPATH=$(BIOSLIB)
LIBPATH=$(BIOSLIB)\$(OUTDIR)
LIBS=bios.lib
!IF DEFINED(LODOSLIB) && 1 # We should not use this one when generating BIOS apps!
INCPATH=$(INCPATH);$(LODOSLIB)
LIBPATH=$(LIBPATH);$(LODOSLIB)\$(OUTDIR)
LIBS=$(LIBS) + lodos.lib
!ENDIF
!IF DEFINED(PMODELIB)
INCPATH=$(INCPATH);$(PMODELIB)
LIBPATH=$(LIBPATH);$(PMODELIB)\$(OUTDIR)
LIBS=$(LIBS) + pmode.lib
!ENDIF
!IF DEFINED(SYSLIB)
INCPATH=$(INCPATH);$(SYSLIB)
!IF 0 # Use the syslibXXX.lib copy of syslib.lib in $(OUTDIR)\LIB
LIBPATH=$(LIBPATH);$(SYSLIB)\$(OUTDIR)\LIB
LIBS=$(LIBS) + syslib$(LSX).lib
!ELSE # Use the initial $(T)-specific syslib.lib in $(OUTDIR)\$(T)$(DS)\BIN\T
LIBPATH=$(LIBPATH);$(SYSLIB)\$(OUTDIR)\$(T)$(DS)\T
LIBS=$(LIBS) + syslib.lib
!ENDIF
!ENDIF
!IF DEFINED(GNUEFI)
INCPATH=$(INCPATH);$(GNUEFI)\INC
!ENDIF

# Library SuffiX. For storing multiple versions of the same library in a single directory.
LSX=b

!ENDIF # !DEFINED(T_VARS)

###############################################################################
#									      #
#		      End of OS-type-specific definitions		      #
#									      #
###############################################################################

!IF !DEFINED(DOS_INFERENCE_RULES)
!INCLUDE "DOS.mak"
!ENDIF
