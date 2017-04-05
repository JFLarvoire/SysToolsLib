		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    absdiskr.asm					      ;
;									      ;
;   DESCRIPTION:    Absolute disk read					      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/28 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    AbsDiskRead 					      ;
;									      ;
;   Description:    Absolute disk read					      ;
;									      ;
;   Parameters:     On stack:						      ;
;		    BYTE bDrive 	DOS Drive number. 0=A, 1=B, etc...    ;
;		    DWORD dwFirstSeg	First sector number		      ;
;		    WORD wNumSeg	Number of sectors		      ;
;		    char far *lpBuf	Buffer				      ;
;									      ;
;   Returns:	    The MS-DOS error code. 0=Success.			      ;
;									      ;
;   Notes:	    The arguments on the stack are in the same order as the   ;
;		    contents of the DOS 4 DISKIO structure.		      ;
;									      ;
;   Regs altered:   AX, BX, CX, DX					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/28 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

AbsDiskRead	proc	near public uses ds, \
			bDrive:BYTE, dwFirstSeg:DWORD, wNumSeg:WORD, \
			lpBuf:FAR PTR BYTE


		lds	dx, dwFirstSeg
		mov	ax, ds		    ; AX:DX = Number of 1st sector
		mov	cx, wNumSeg	    ; CX = Number of sectors to read
		lds	bx, lpBuf	    ; DS:BX = Buffer address

		test	ax, ax		    ; More than 64KB sectors?
;		 jz	 @F		     ; Jump if not

		mov	cx, -1		    ; Use the DISKIO structure at DS:BX
		push	ss
		pop	ds
		lea	bx, WORD PTR dwFirstSeg
@@:
		mov	al, bDrive
		int	25H
		jc	@F
		xor	ax, ax
@@:
		popf
		ret

AbsDiskRead	endp

END
