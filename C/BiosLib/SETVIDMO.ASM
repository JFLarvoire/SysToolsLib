		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   File Name:	    SetVidMo.asm					      ;
;									      ;
;   Description:    Set Video Mode					      ;
;									      ;
;   Notes:								      ;
;									      ;
;   History:								      ;
;    1996/08/19 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

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

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    set_video_mode					      ;
;									      ;
;   Description:    Set the video mode					      ;
;									      ;
;   Parameters:     AX	    Mode number.				      ;
;									      ;
;   Returns:	    AX	    Video BIOS return code.			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX						      ;
;									      ;
;   History:								      ;
;    1992/05/15 MM  Initial implementation.				      ;
;    1992/06/17 MM  Set the video mode only for the QUASAR family.	      ;
;		    Do nothing for the following models such as PULSAR,       ;
;		    detectable with their hp_id "HP" in another location      ;
;		    in F000:0102h.					      ;
;    1996/06/26 JFL Removed the pulsar-specific code.			      ;
;		    Added support for VESA modes.			      ;
;    1997/02/13 JFL Use VESA extensions only for modes > 255.		      ;
;    1997/03/18 JFL Do not revert to VGA in case of VESA failure. Stupid.     ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	set_video_mode

		test	ah, ah
		jz	@F

		; Use VESA extensions for modes > 255
		mov	bx, ax	    ; <15>=!Clear; <14>=flat; <8:0>=Mode
		mov	ax, 4F02H   ; Set VBE mode
		int	VIDEO	    ; Preserves all registers but AX
		jmp	done
@@:
		mov	ah, F10_SET_MODE
		int	VIDEO
		xor	ah, ah
done:
                ret

ENDCFASTPROC	set_video_mode

END
