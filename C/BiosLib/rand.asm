		page	,132
;*****************************************************************************;
;									      ;
;   File name	    rand.asm						      ;
;									      ;
;   Description     Generate a pseudo-random number			      ;
;									      ;
;   Notes	    							      ;
;									      ;
;   History								      ;
;    2001/12/21 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 2001-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

INCLUDE		ADEFINE.INC

.DATA

seed		dd	?

.CODE

.386

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    rand						      ;
;									      ;
;   Description:    Generate a pseudo-random number			      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    AX = random number					      ;
;									      ;
;   Notes:	    							      ;
;									      ;
;   Regs altered:   EAX, EDX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    2001/12/21 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	srand

		movzx	eax, ax
		mov	seed, eax
		ret

ENDCFASTPROC	srand			

CFASTPROC	rand

		mov	eax, seed
		imul	eax, 11111117	; Big prime number
		xor	eax, edx	; Mix 64 bits into 32
		mov	seed, eax
		shr	eax, 17		; Make sure result in [0-32767].
		ret

ENDCFASTPROC	rand

end
