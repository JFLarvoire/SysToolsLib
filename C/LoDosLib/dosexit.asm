	page	, 132
;*****************************************************************************;
;									      ;
;   FileName:	    dosexit.asm 					      ;
;									      ;
;   Contents:	    Exit to DOS using int 21H				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   History:								      ;
;    1995/02/24 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

INCLUDE ADEFINE.INC

.CODE

;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		_dos_exit					      ;
;									      ;
;	DESCRIPTION:	Exit to DOS using int 21H			      ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	AL = Dos Exit code				      ;
;									      ;
;	    ON EXIT:	N/A						      ;
;									      ;
;	REGS ALTERED:	N/A						      ;
;									      ;
;=============================================================================;

CFASTPROC	_dos_exit

		mov	ah, 4CH
		int	21H

ENDCFASTPROC	_dos_exit

END
