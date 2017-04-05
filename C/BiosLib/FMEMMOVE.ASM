		page	,132
		TITLE	C library emulation, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    fmemmove.asm 					      ;
;									      ;
;   DESCRIPTION:    Copy a far block of memory, managing overlap	      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   History:								      ;
;    2001/08/02 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions

		.286

		.code

EXTERNCFASTPROC	canonicalize

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _fmemmove						      ;
;									      ;
;   Description:    Copy a far block of memory, managing overlap	      ;
;									      ;
;   Parameters:     ARG1    Far pointer to the destination buffer	      ;
;		    ARG2    Far pointer to the source buffer		      ;
;		    ARG3    Number of characters to copy		      ;
;									      ;
;   Returns:	    DX:AX   The destination pointer			      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, CX, ES						      ;
;									      ;
;   History:								      ;
;    2001/08/02 JFL Created this routine, adapted from _fmemcpy().	      ;
;									      ;
;-----------------------------------------------------------------------------;

_fmemmove	proc	C public, lpDEST:DWORD, lpSrc:DWORD, wCount:WORD

		push	si
		push	di
		push	ds

		; Get canonical source pointer
		mov	ax, WORD ptr lpSrc
		mov	dx, WORD ptr lpSrc+2
		call	canonicalize
		mov	si, ax
		mov	ds, dx
		mov	cx, dx			; Save temp source segment copy

		; Get canonical destination pointer
		mov	ax, WORD ptr lpDest
		mov	dx, WORD ptr lpDest+2
		call	canonicalize
		mov	di, ax
		mov	es, dx

		; Are we moving up or down?
		cmp	dx, cx			; Compare dest and source segments
		mov	cx, wCount		; Count in CX
		jb	copy_down
		ja	copy_up
		cmp	di, si			; Segments equal. Compare offsets.
		jb	copy_down
		; ja	copy_up
		; jmp	done
		je	done

copy_up:	; Must start copying at the end to prevent overlap.
		add	si, cx
		dec	si			; Last byte to copy
		add	di, cx
		dec	di			; Last byte of destination buffer
		std				; Change from standard C direction

copy_down:	; Must start copying at the beg to prevent overlap.
		rep	movsb

done:
		mov	ax, WORD ptr lpDest	; Restore the uncanonical destination pointer
		mov	dx, WORD ptr lpDest+2
		cld				; Restore the standard C direction flag
		pop	ds
		pop	di
		pop	si
		ret

_fmemmove	endp

END
