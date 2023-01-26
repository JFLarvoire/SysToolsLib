	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;***************************** :encoding=UTF-8: ******************************;
;									      ;
;   FILE NAME:	    SETCURPO.ASM 					      ;
;									      ;
;   DESCRIPTION:    Routine to set the current text cursor coordinates	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    2023-01-26 JFL Created routine based on getcurpo.asm.  		      ;
;									      ;
;                   © Copyright 2023 Jean-François Larvoire                   ;
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
;  ENTRY POINT NAME:    set_cursor_position
;
;  DESCRIPTION:         Set the cursor coordinates
;
;  INPUT PARAMETERS
;       AX = Column number (Typically 0 to 79)
;       DX = Row number (Typically 0 to 24)
;
;  OUTPUT PARAMETERS
;       None
;
;  REGISTERS DESTROYED: AX, BX, DX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES	Preserves CX for historical reasons.
;
;  MODIFICATION HISTORY NOTES
;    2023-01-26 JFL Initial implementation.
;
;=============================================================================;

CFASTPROC       set_cursor_position

		mov	dh, dl			; Row number
		mov	dl, al			; Column number
		xor	bh, bh			; Page number
                mov     ah, F10_SET_CURPOS      ; Get the cursor position
                int     VIDEO                   ; Size in CX & coords in DX
                ret

ENDCFASTPROC    set_cursor_position

END
