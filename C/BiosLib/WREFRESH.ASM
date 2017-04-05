	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    WREFRESH.ASM					      ;
;									      ;
;   DESCRIPTION:    Utility routine for speed-independant loops		      ;
;									      ;
;   NOTES:								      ;
;									      ;
;   HISTORY:								      ;
;    1990/11/01 JFL Initial implementation of autil.asm.		      ;
;    1994/05/10 JFL Moved routine wait_refresh from io8042.asm.		      ;
;									      ;
;      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

	.286

;-----------------------------------------------------------------------------;
;									      ;
;   Globally defined constants						      ;
;									      ;
;-----------------------------------------------------------------------------;

include 	adefine.inc	; All assembly language definitions
include 	io8042.inc	; 8042-specific definitions

;-----------------------------------------------------------------------------;
;									      ;
;   Public routines							      ;
;									      ;
;-----------------------------------------------------------------------------;

		.code

;=============================================================================;
;
;  ENTRY POINT NAME:	wait_refresh
;
;  DESCRIPTION: 	Wait for the next refresh cycle
;
;  INPUT PARAMETERS
;	None
;
;  OUTPUT PARAMETERS
;	None
;
;  REGISTERS DESTROYED: AX
;
;  REGISTERS PRESERVED: Others
;
;  DEPENDENCIES
;
;  ALGORITHM USED
;
;  NOTES
;
;	Wait for the next refresh cycle. There is one cycle every 15 us.
;	This allows to make speed independant loops.
;	A call to this routine takes up to 10 us on machines at low speed.
;
;  MODIFICATION HISTORY NOTES
;     JFL 03/28/91  Initial implementation.
;
;=============================================================================;

CFASTPROC	wait_refresh			    ; clocks

		in	al, SPU_CONTROL_PORT	    ; 12
		mov	ah, al			    ; 2   ; Reference value
wait_ref_loop:
		in	al, SPU_CONTROL_PORT	    ; 12
		xor	al, ah			    ; 2   ; Which bits changed?
		test	al, REFRESH_MASK	    ; 2   ; Test refresh only
		jz	wait_ref_loop		    ; 7+2 ; Loop if not changed.

		ret				    ; 10+N

ENDCFASTPROC	wait_refresh

END
