	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    MALLOC.ASM						      ;
;									      ;
;   DESCRIPTION:    C library emulation routines in assembly language	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1990/11/02 JFL Initial implementation of alibc.asm.		      ;
;    1991/11/13 JFL Added malloc().					      ;
;    1991/04/11 JFL Changed the way the beginning of the local heap is	      ;
;		    initialized to use the newly defined stack segment	      ;
;		    base. This was necessary because the linker does not      ;
;		    define the handy _END label for EXE files.		      ;
;    1993/10/06 JFL Made malloc_base public.				      ;
;    1993/10/07 JFL Separated from alibc.c.				      ;
;		    							      ;
;      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

	.286

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include		adefine.inc	; All assembly language definitions

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

;=============================================================================;
;
;  ENTRY POINT NAME:	malloc
;
;  DESCRIPTION: 	Allocate memory from the near heap
;
;			Warning: No error checking! May overwrite stack!
;
;  INPUT PARAMETERS
;	AX	Number of bytes to allocate
;
;  OUTPUT PARAMETERS
;	Near pointer to the block allocated
;
;  REGISTERS DESTROYED: AX, DX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;  MODIFICATION HISTORY NOTES
;   JFL 11/13/90 Initial implementation.
;
;=============================================================================;

.data

		public	malloc_base
malloc_base	dw	offset DGROUP:STACK

.code

CFASTPROC	malloc

		xchg	ax, malloc_base 	; Current top of memory
		add	malloc_base, ax 	; Save new top of memory

		ret

ENDCFASTPROC	malloc

END
