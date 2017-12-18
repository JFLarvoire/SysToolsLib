		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    resetdrv.asm					      ;
;									      ;
;   DESCRIPTION:    Reset a specific DOS drive using the MS-DOS 7 method      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/09/05 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    ResetDrive						      ;
;									      ;
;   Description:    Reset a specific DOS drive using the MS-DOS 7 method      ;
;									      ;
;   Parameters:     AX iDrive	    Drive number. 0=A, 1=B, 2=C, etc...       ;
;		    DX iFlushFlag   0 = Reset drive and flush buffers	      ;
;				    1 = Idem + invalidate the cache	      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code. 0=Success.		      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, CX, DX. 					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/09/04 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	ResetDrive

		mov	cx, dx		    ; iFlushFlag
		mov	dx, ax		    ; iDrive
		inc	dx		    ; Call expects 1-based value
		mov	ax, 710DH
		int	21H
		check_error		    ; Clear AX if no carry

		ret

ENDCFASTPROC	ResetDrive

END
