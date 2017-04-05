;*****************************************************************************;
;									      ;
;   FILE NAME:	    memset.asm						      ;
;									      ;
;   DESCRIPTION:    Fill a block of memory				      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1997/12/04 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1997-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.model TINY

.386p

.code

;------------------------------------------------------------------------------
; ANSI-C memset function
; C-interface:
;    void * memset(void *, int, size_t);
; corresponds to:    bx  , ax ,   dx

_memset	   equ	   @memset

_memset    proc    syscall public

           mov     cx, dx      ; get length into cx
           mov     dx, ax      ; temporary store ax
           mov     ax, ds      ; make ES, DS the same
	   mov     es, ax
	   mov     ax, dx      ; restore ax
           mov     dx, di      ; save di for later restoring
           mov     di, bx      ; get address into di for later bit testing
           jcxz    ready
	   mov     ah, al
	   test    di, 1
	   jz      aligned
	   stosb               ; set first byte
	   dec     cx

aligned:   shr     cx, 1       ; divide by 2, but keep first bit in C-flag
           rep stosw
	   adc     cx, cx      ; get back C-flag
	   rep stosb
ready:     mov     di, dx      ; restore di
           xchg    ax, bx      ; return value

           ret

_memset    endp

end

;------------------------------------------------------------------------------
