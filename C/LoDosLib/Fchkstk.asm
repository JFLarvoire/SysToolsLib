	page	,132
	title	Pseudo stack check
	subttl	Fchkstk.asm

;*****************************************************************************;
;									      ;
;   FILE NAME 	    Fchkstk.asm						      ;
;									      ;
;   DESCRIPTION     Replace the far stack checks from the MSVC library	      ;
;									      ;
;   NOTES 	    Only creates the stack frame, but does not actually	      ;
;		    check for stack overflow.				      ;
;		    							      ;
;   HISTORY								      ;
;    2022-11-24 JFL Created this routine				      ;
;									      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	large, C
		.code

		ASSUME	DS:NOTHING, ES:NOTHING, SS:STACK

public		_aFchkstk

_aFchkstk	proc	far

		pop	cx			; The return address offset
		pop	dx			; The return address segment
		sub	sp, ax
		push	dx
		push	cx
		ret

_aFchkstk	endp

		.stack

end
