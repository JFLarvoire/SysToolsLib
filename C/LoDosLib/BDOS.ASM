		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    bdos.asm						      ;
;									      ;
;   DESCRIPTION:    Microsoft C library routine _bdos			      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/23 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE DOS.INC 	; For the DOS call macros

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    _bdos						      ;
;									      ;
;   Description:    Make a call the basic DOS functions 		      ;
;									      ;
;   Parameters:     AX	    Function number				      ;
;		    DX	    DX register value				      ;
;		    BX	    AL register value				      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, ES. 					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/28 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_bdos

		mov	ah, al
		mov	al, bl
		int	21H
		check_error

		ret

ENDCFASTPROC	_bdos

END
