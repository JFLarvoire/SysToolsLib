###############################################################################
#									      #
#   File name:	    lodos.mak						      #
#									      #
#   Description:    A NMake makefile to build LODOS programs		      #
#									      #
#   Notes:	    Use with make.bat, which defines the necessary variables. #
#		    Usage: make -f lodos.mak [definitions] [targets]	      #
#									      #
#		    LODOS programs are DOS programs that _only_ use low DOS   #
#		    and BIOS function calls, defined in the LoDosLib and      #
#		    BiosLib libraries.					      #
#		    This also allows creating tiny DOS programs, much smaller #
#		    than the ones linked with MSVC's standard C library.      #
#		    This can be very useful on systems with extremely limited #
#		    storage space, like JFL's Universal Boot Disks (UBD).     #
#		    Compared to MINICOM programs (See BIOS.mak), LODOS .com   #
#		    programs are barely bigger, and their output can be	      #
#		    redirected, which is often convenient.		      #
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
#		    clean	    Erase all files in the LODOS subdirectory.#
#		    {prog}.com	    Build LODOS[\Debug]\{prog}.com.	      #
#		    {prog}.obj	    Build LODOS[\Debug]\OBJ\{prog}.obj.	      #
#		    LODOS\{prog}.com       Build the BIOS release version.    #
#		    LODOS\Debug\{prog}.com Build the BIOS debug version.      #
#		    LODOS\OBJ\{prog}.obj   Compile the LODOS release version. #
#		    LODOS\Debug\OBJ\{prog}.obj Compile the LODOS debug version.
#									      #
#		    Command-line definitions:				      #
#		    DEBUG=0	 Build the release ver. (<=> program in LODOS)#
#		    DEBUG=1	 Build the debug ver. (<=> pgm in LODOS\DEBUG)#
#		    MEM=T	 Build the tiny ver.  (<=> objects in OBJ\T)  #
#		    MEM=S	 Build the small ver. (<=> objects in OBJ\S)  #
#		    MEM=L	 Build the large ver. (<=> objects in OBJ\L)  #
#		    OUTDIR=path  Output to path\LODOS\. Default: To bin\LODOS\#
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
#    2022-11-25 JFL Adapted from BIOS.mak.                                    #
#    2022-12-03 JFL Removed all inference rules, and use DOS.MAK's instead.   #
#		    							      #
#                   © Copyright 2022 Jean-François Larvoire                   #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF !DEFINED(T)
T=LODOS				# Target OS
!ENDIF

!IF !DEFINED(T_VARS)
T_VARS=1	# Make sure OS-type-specific variables are defined only once

T_DEFS=/D_LODOS /DMINICOMS	# Tell sources what environment they're built for

# Memory model for 16-bit C compilation (T|S|C|D|L|H)
!IF !DEFINED(MEM)
MEM=T				# Memory model for C compilation
!ENDIF

EXE=com				# Default program extension

STARTCOM=$(BIOSLIB)\$(OUTDIR)\OBJ\startcom.obj
STARTEXE=$(LODOSLIB)\$(OUTDIR)\OBJ\startexe.obj

CODEPAGE=$(DOS_CS)		# Use the user-defined code page

# Tools and options
CGFLAGS=/G2rs /Oaes /Zpil	# C code generation flags

LFLAGSX=/nod			# Extra linker flags

INCPATH=$(LODOSLIB)
LIBPATH=$(LODOSLIB)\$(OUTDIR)
LIBS=lodos.lib
!IF DEFINED(BIOSLIB)
INCPATH=$(INCPATH);$(BIOSLIB)
LIBPATH=$(LIBPATH);$(BIOSLIB)\$(OUTDIR)
LIBS=$(LIBS) + bios.lib
!ENDIF
!IF DEFINED(PMODELIB)
INCPATH=$(INCPATH);$(PMODELIB)
LIBPATH=$(LIBPATH);$(PMODELIB)\$(OUTDIR)
LIBS=$(LIBS) + pmode.lib
!ENDIF
!IF DEFINED(SYSLIB)
INCPATH=$(INCPATH);$(SYSLIB)
LIBPATH=$(LIBPATH);$(SYSLIB)\$(OUTDIR)\LIB
LIBS=$(LIBS) + syslib$(LSX).lib
!ENDIF
!IF DEFINED(GNUEFI)
INCPATH=$(INCPATH);$(GNUEFI)\INC
!ENDIF

# Library SuffiX. For storing multiple versions of the same library in a single directory.
LSX=l

!ENDIF # !DEFINED(T_VARS)

###############################################################################
#									      #
#		      End of OS-type-specific definitions		      #
#									      #
###############################################################################

!IF !DEFINED(DOS_INFERENCE_RULES)
!INCLUDE "DOS.mak"
!ENDIF
