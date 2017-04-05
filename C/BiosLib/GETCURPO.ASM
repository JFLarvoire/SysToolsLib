	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    GETCURPO.ASM 					      ;
;									      ;
;   DESCRIPTION:    Routine to get the current text cursor coordinates	      ;
;									      ;
;   NOTES:	    Uses only BIOS calls to work before DOS is available.     ;
;									      ;
;   HISTORY:								      ;
;    1991/06/14 JFL Created routine in RomSetup's AUTIL.ASM.		      ;
;    1996/02/02 JFL Extracted from AUTIL.ASM.				      ;
;		    Get the current page from the BIOS data area.	      ;
;									      ;
;      (c) Copyright 1991-2017 Hewlett Packard Enterprise Development LP      ;
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
;  ENTRY POINT NAME:    get_cursor_position
;
;  DESCRIPTION:         Get the cursor coordinates
;
;  INPUT PARAMETERS
;       BX = Address of the variable for the column (0 to 79)
;       AX = Address of the variable for the row (0 to 24)
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
;   JFL 06/14/91 Initial implementation.
;   JFL 96/02/02 Get the current page from the BIOS data area.
;
;=============================================================================;

CFASTPROC       get_cursor_position

                push    cx
                push    ax                      ; Save pointer to row
                push    bx                      ; Save pointer to column

		push	ds
		mov	dx, seg BDA
		mov	ds, dx
		ASSUME	ds:BDA
		mov	bh, video_page		; Active video page
		pop	ds
		ASSUME	ds:@data

                mov     ah, F10_GET_CURPOS      ; Get the cursor position
                int     VIDEO                   ; Size in CX & coords in DX

                xor     ah, ah
                pop     bx                      ; Restore pointer to column
                mov     al, dl                  ; Column
                mov     [bx], ax

                pop     bx                      ; Restore pointer to row
                mov     al, dh                  ; Row
                mov     [bx], ax

                pop     cx
                ret

ENDCFASTPROC    get_cursor_position

END
