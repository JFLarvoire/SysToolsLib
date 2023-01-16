		page	,132
;*****************************************************************************;
;									      ;
;   File name	    isatty.asm						      ;
;									      ;
;   Description     Microsoft C library routine isatty			      ;
;									      ;
;   Notes								      ;
;									      ;
;   History								      ;
;    2023-01-16 JFL Created this file.					      ;
;									      ;
;                  (c) Copyright 2023 Jean-Francois Larvoire                  ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function	    isatty						      ;
;									      ;
;   Description     Standard C library routine - Test if handle is for a TTY  ;
;									      ;
;   Parameters      AX	    DOS file number				      ;
;									      ;
;   Returns	    AX	    1=Yes, 0=No					      ;
;									      ;
;   Notes	    Does not support serial consoles, etc.		      ;
;									      ;
;   Regs altered    AX, BX, DX. 					      ;
;									      ;
;   History:								      ;
;    2023-01-16 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	isatty

		mov	bx, ax
		mov	ax, 440AH
		int	21H
		jc	dos_error

		test	dl, 80H
		jz	no_tty		; Not a device
		test	dl, 02H
		jz	no_tty		; Not the console device

		mov	ax, 1		; Yes, it's a TTY
		ret
dos_error:	; AX can be 1=Unsupported function or 6=Invalid handle
		; TODO: If error==1 (very old DOS), assume handle 1 is always the console?
                ; TODO: Set _doserrno and errno in case of error
		; mov	errno, ax
no_tty:
		xor	ax, ax
		ret

ENDCFASTPROC	isatty

END
