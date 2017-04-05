	PAGE	,132
	TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    KBHIT.ASM						      ;
;									      ;
;   DESCRIPTION:    C library emulation routines in assembly language	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1990/11/02 JFL Initial implementation of alibc.asm.		      ;
;    1991/05/31 LA  Enhanced "kbhit" routine to avoid the crash if a CTRL-    ;
;		     <break> key is pressed.				      ;
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
;  ENTRY POINT NAME:	kbhit
;
;  DESCRIPTION:		Test is a key has been pressed. C standard library.
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	AX	0 = No character waiting.
;		other = One is waiting.
;
;  REGISTERS DESTROYED: AX
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
;   JFL 11/02/90 Initial implementation.
;   LA 05/31/91  Enhanced "kbhit" routine to make CTRL_<break> "equivalent"   								      ;
;		  to CTRL_C. This avoids a crash.
;
;=============================================================================;

CFASTPROC	kbhit

		mov	ah, F16_EXT_STATUS
		int	KEYBOARD
		jnz	fix_ascii
		sub	ax, ax
		ret
fix_ascii:
                cmp     ax, 0                   ; Is CTRL pressed ?
                jne     is_it_ascii             ;  no:  check if it is an ASCII
                mov     ax, 3                   ;  yes: Make CTRL_BREAK a CTRL_C
is_it_ascii:
                test	al, al			; ASCII?
		jz	got_non_ascii		; If not, return scan code in AH
		sub	ah, ah			; Else clear scan code in AH
got_non_ascii:
		ret

ENDCFASTPROC	kbhit

END
