		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    freemem.asm 					      ;
;									      ;
;   DESCRIPTION:    Microsoft C library routine _dos_freemem		      ;
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
;   Function:	    _dos_freemem					      ;
;									      ;
;   Description:    Free a block of DOS memory				      ;
;									      ;
;   Parameters:     AX	    Segment to free				      ;
;									      ;
;   Returns:	    AX	    MS-DOS error code				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX, BX, ES. 					      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/23 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	_dos_freemem

		free_memory ax
		check_error

		ret

ENDCFASTPROC	_dos_freemem

END
