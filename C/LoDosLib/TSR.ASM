		page	,132
;*****************************************************************************;
;									      ;
;   FILE NAME:	    tsr.asm						      ;
;									      ;
;   DESCRIPTION:    Terminate and stay resident 			      ;
;									      ;
;   NOTES:	    For use only by DOS applications (.COM and .EXE).	      ;
;									      ;
;   HISTORY:								      ;
;    1995/08/23 JFL Created this file.					      ;
;    1997/08/22 JFL Restrict the usage of this routine to the tiny memory     ;
;		     model. This is reasonable anyway since we need to	      ;
;		     allocate resident data in the RESID code segment.	      ;
;									      ;
;      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

		INCLUDE ADEFINE.INC	; For the segment definitions
		INCLUDE DOS.INC 	; For the DOS call macros

DGROUP		group	RESID, RESIDEND, _TEXT	  ; Pseudo tiny mode

		.286

;=============================================================================;
;		      Resident code and data segment			      ;
;=============================================================================;

;=============================================================================;
;		      Discardable data segment				      ;
;=============================================================================;

.DATA

extern		_psp:word

;=============================================================================;
;		      Discardable initialization segment		      ;
;=============================================================================;

.CODE

;-----------------------------------------------------------------------------;
;									      ;
;   Function:	    tsr 						      ;
;									      ;
;   Description:    Terminate and stay resident 			      ;
;									      ;
;   Parameters:     int iRetCode	MS-DOS exit code		      ;
;		    WORD wBufSize	Up to 65520 bytes for a scratch buffer;
;									      ;
;   Returns:	    N/A 						      ;
;									      ;
;   Notes:	    Automatically frees the environment segment.	      ;
;									      ;
;		    Optionally allocates a scratch buffer, overlaying the     ;
;		    initialization code and data. Variable top_of_rseg gives  ;
;		    the address of the allocated scratch buffer, if any.      ;
;									      ;
;		    This routine is very similar to the Microsoft C library   ;
;		    routine _dos_keep, except that the latter requires the    ;
;		    caller to compute the size of the resident code.	      ;
;									      ;
;   Regs altered:   N/A 						      ;
;									      ;
;   History:								      ;
;									      ;
;    1995/08/23 JFL Created this routine				      ;
;									      ;
;-----------------------------------------------------------------------------;

CFASTPROC	tsr

		; Free the environment segment

		push	ax

		mov	es, _psp
		xor	ax, ax
		xchg	ax, word ptr es:[2CH]
		test	ax, ax		    ; Just in case it was already done,
		jz	@F		    ;  jump to prevent an error.
		free_memory ax
@@:
		pop	ax

		; Compute the size of the resident segment

		add	dx, 15		    ; Safety against conversion below
		add	dx, offset DGROUP:RESIDEND ; Resident size (0 to 128 K)
		rcr	dx, 1		    ; Convert it to paragraphs
		shr	dx, 1
		shr	dx, 1
		shr	dx, 1

		mov	cx, cs
		sub	cx, _psp	    ; Offset of RESID relative to PSP
		add	dx, cx

		keep_process dx 	    ; al already has exit code

ENDCFASTPROC	tsr

END
