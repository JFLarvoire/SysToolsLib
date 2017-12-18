		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    critsect.asm					      ;
;									      ;
;   DESCRIPTION:    Critical section management routines under Windows VMM    ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/25 JFL Created this file.					      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    BeginCriticalSection				      ;
;									      ;
;   Description:    Prevent VMM from switching virtual machines 	      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    None						      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/02 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	BeginCriticalSection

		mov   ax, 1681H
		int   2FH

		ret

ENDCFASTPROC	BeginCriticalSection

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    EndCriticalSection					      ;
;									      ;
;   Description:    Authorize VMM to switch virtual machines again	      ;
;									      ;
;   Parameters:     None						      ;
;									      ;
;   Returns:	    None						      ;
;									      ;
;   Notes:								      ;
;									      ;
;   Regs altered:   AX. 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/02 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	EndCriticalSection

		mov   ax, 1682H
		int   2FH

		ret

ENDCFASTPROC	EndCriticalSection

END
