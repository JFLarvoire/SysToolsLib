	page	,132
	title	Pseudo stack check
	subttl	Nchkstk.asm

;*****************************************************************************;
;									      ;
;   FILE NAME 	    Nchkstk.asm						      ;
;									      ;
;   DESCRIPTION     Replace the near stack checks from the MSVC library	      ;
;									      ;
;   NOTES 	    Only creates the stack frame, but does not actually	      ;
;		    check for stack overflow.				      ;
;		    							      ;
;   HISTORY								      ;
;    2022-11-24 JFL Created this routine				      ;
;									      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		.model	small, C
		.code

		ASSUME	DS:NOTHING, ES:NOTHING, SS:STACK

public		_aNchkstk

_aNchkstk	proc	near

		pop	cx			; The return address
		sub	sp, ax
		jmp	cx

_aNchkstk	endp

		.stack

end
