		.XLIST
;*****************************************************************************;
;									      ;
;   FILE NAME:	    ADEFINE.INC 					      ;
;									      ;
;   DESCRIPTION:    All assembler constants and macros for BIOS.LIB	      ;
;									      ;
;   NOTES:	    This file must be included in the startup module of all   ;
;		    programs using BIOS.LIB. This guaranties the right	      ;
;		    segment ordering.					      ;
;									      ;
;		    Segment ordering primer:				      ;
;									      ;
;		    A program is a succession of segments.		      ;
;		    The C compiler makes assumptions about the order of       ;
;		    segments in memory.					      ;
;		    Defining once all segments ensures that the linker will   ;
;		    store them in the given order in the EXE file. This is    ;
;		    done below.						      ;
;		    All other segment the linker encounters are stored	      ;
;		    afterwars.						      ;
;		    Exception: If an unknown segment has a known class, it is ;
;		    inserted right after the last known segment of that class.;
;		    Defining segments that are otherwise unused has no ill    ;
;		    effects.						      ;
;		    Segments can be grouped by the linker. This means they're ;
;		    merged into a single segment. This is tricky though,      ;
;		    because if a .ASM module doesn't know about the group,    ;
;		    it will generate adresses relative to the beginning of    ;
;		    the segments it knows, and not relative to the beginning  ;
;		    of the group as it should.				      ;
;									      ;
;		    A non standard segment called RESID has been added.	      ;
;		    It is intended to contain all resident code and data in   ;
;		    TSRs and device drivers.				      ;
;		    It is defined before all others, so that when a program   ;
;		    stays resident, all other segments can be easily discarted.
;									      ;
;   History:								      ;
;    1990-11-02 JFL Created this file.					      ;
;    1991-01-30	JFL Added Date and time definitions			      ;
;		    Added HP BIOS definitions				      ;
;		    Added macros for C fast call routines		      ;
;    1991-03-29	JFL Added definitions of labels with no extra leading	      ;
;		    character in macros CPROC and CFASTPROC.		      ;
;    1991-04-11	JFL Added support for the small memory model in addition      ;
;		    to the tiny model supported before.			      ;
;    1993-05-10	JFL Added more standard C segments to guaranty the correct    ;
;		    ordering of far segments in big EXE files.		      ;
;		    Added various BIOS equates to match that in RomSetup.     ;
;    1993-07-15	JFL Removed assume CS directives to satisfy MASM 6.x.	      ;
;    1993-10-06	JFL Adapted to C TSRs and device drivers.		      ;
;    1993-10-07	JFL Added the BIOS data area definition.		      ;
;		    Added the .model small, C directive.		      ;
;    1994-05-05	JFL Minor syntax changes to prevent conflicts with DOS.INC    ;
;    1995-02-22 JFL Added external references to routines that are likely     ;
;		     to be called from assembly language modules.	      ;
;    1995-02-23	JFL Oops, fixed segment ordering bug introduced yesterday.    ;
;    1997-08-22	JFL Added RESIDEND segment.				      ;
;    2001-08-07	JFL Changed RESID alignment to PARA. Necessary to allow       ;
;		     reordering RESID behind the code segment.		      ;
;    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              ;
;									      ;
;      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

FALSE		=	0
TRUE		=	1

;-----------------------------------------------------------------------------;
;									      ;
;   BIOS definitions							      ;
;									      ;
;-----------------------------------------------------------------------------;

VIDEO		equ	10H		; Video DSR
F10_SET_MODE	equ	00H		; Set video mode
F10_SET_CURSIZE equ	01H		; Set cursor size
F10_SET_CURPOS	equ	02H		; Set cursor position
F10_GET_CURPOS	equ	03H		; Get cursor position
F10_SET_PAGE	equ	05H		; Set the active display page
F10_SCROLL_UP	equ	06H		; Scroll window up
F10_SCROLL_DOWN equ	07H		; Scroll window down
F10_WR_CHARATR	equ	09H		; Write character and attribute
F10_WR_CHARTEL	equ	0EH		; Write character to teletype
F10_GET_MODE	equ	0FH		; Get the current video adapter state
F10_SET_PALETTE equ	10H		; Set palette register
F10_SET_BORDER  equ     01H             ; Set border color register 
F10_SET_BLINKING equ	03H		; Set intensify/blinking bit
F10_GET_BORDER  equ     08H             ; Get border color register 


KEYBOARD	equ	16H		; Keyboard DSR
F16_GET_KEY	equ	00H		; Return next PC keycode
F16_STATUS	equ	01H		; Return the PC keyboard buffer status
F16_PUT_KEY	equ	05H		; Put scancode in keyboard buffer
F16_GET_EXT_KEY	equ	10H		; Return next AT2 keycode
F16_EXT_STATUS	equ	11H		; Return the AT2 keyboard buffer status

DATE_TIME	equ	1AH		; Date and Time DSR
F1A_READ_TIME	equ	02H		; Read time from RTC
F1A_WRITE_TIME	equ	03H		; Write time into RTC
F1A_READ_DATE	equ	04H		; Read date from RTC
F1A_WRITE_DATE	equ	05H		; Write date into RTC

DOS_INT         equ     21H             ; DOS interrupt
SET_VECTOR_FCT	equ	25H		; Set interrupt vector function
GET_VECTOR_FCT	equ	35H		; Get interrupt vector function
CTL_BRK_VEC     equ     23H             ; DOS <Ctrl> break address
CRIT_ERR_VEC	equ	24H		; DOS critical error

;-----------------------------------------------------------------------------;

SYSTEM_BIOS	segment at 0F000H	; System BIOS segment

		org	0E0H
hp_private_entry label	far		; HP private entry point

		org	0F0H
capab_marker2	label	byte		; HP Capability marker extension

		org	0F8H
hp_id_bytes	label	word		; HP Id bytes: 'H' then 'P'

		org	0FBH
capab_marker	label	byte		; HP Capability marker

		org	102H
hp_id_pulsar	label	word		; HP Id bytes: 'H' then 'P' in this
					; specific address from PULSAR

		org	0FFF0H		; Micro processor reset address
reset_vector	label	far

		org	0FFFFH		; Just make the map look good

SYSTEM_BIOS	ends

;-----------------------------------------------------------------------------;

BDA		segment at 40H		; BIOS data area

		org	49H
video_mode	db	?		; Current video mode
video_colums	dw	?		; Number of screen columns
video_screen_size dw	?		; Length of screen in bytes
video_screen_addr dw	?		; Address of current display page
video_cursors	db	10H dup (?)	; Cursor coords (orw, col) for 8 pages
video_cursor_size db	2 dup (?)	; Cursor mode settings
video_page	db	?		; Current display page
video_crtc_io	dw	?		; CRTC base I/O port address
video_mode_reg	db	?		; Mode select register copy
video_color_reg db	?		; Color palette register copy

BDA		ends

;-----------------------------------------------------------------------------;

RS_SUCCESFUL	equ	0		; HP BIOS standard return code
RS_UNSUPPORTED	equ	2
RS_FAIL 	equ	-2

;-----------------------------------------------------------------------------;
;									      ;
;   C interface								      ;
;									      ;
;-----------------------------------------------------------------------------;

; Parameter passing

ARG1		equ	4		; Offset of the first C argument
ARG2		equ	6		; Offset of the second C argument
ARG3		equ	8		; Offset of the third C argument
ARG4		equ	10D		; Offset of the fourth C argument
ARG5		equ	12D		; Offset of the fifth C argument
ARG6		equ	14D		; Offset of the sixth C argument

; Segment macro definitions

PSEG		macro	; Obsolete macro. Use .code instead.
_TEXT		SEGMENT WORD PUBLIC 'CODE'	; Code segment
		assume	ds:DGROUP, ss:DGROUP
		endm

ENDPS		macro	; Obsolete macro. Useless if .code was used
_TEXT		ENDS				; End of the code segment
		endm

DSEG		macro	; Obsolete macro. Use .data instead.
_DATA		SEGMENT WORD PUBLIC 'DATA'	; Initialized variables
		endm

ENDDS		macro	; Obsolete macro. Useless if .data was used
_DATA		ENDS				; End of the data segment
		endm

RSEG		macro	; Not obsolete at all! There's no simplified equivalent
RESID		SEGMENT PARA PUBLIC 'CODE'	; Resident segment for
		endm				;  TSRs & device drivers

ENDRS		macro
RESID		ENDS				; End of the resident segment
		endm

;-----------------------------------------------------------------------------;

; Public procedures to be called from C

;		C PROCs are those using the standard C calling convention,
;		passing parameters on the stack.

CPROC		macro	name			;; Procedure definition

name		proc	public

		endm


ENDCPROC	macro	name			;; Procedure end

name		endp

		endm

;-----------------------------------------------------------------------------;

;		C FAST PROCs are those using the Microsoft C 6.0 "Fast calling
;		convention" passing parameters in AX, DX, BX then the stack.

CFASTPROC	macro	name			;; Procedure definition

name		equ	@&name

@&name		proc	syscall public

		endm


ENDCFASTPROC	macro	name			;; Procedure end

@&name		endp

		endm


EXTERNCFASTPROC macro	name

name		equ	@&name

@&name		proto	syscall

		endm

;-----------------------------------------------------------------------------;

; Segment ordering necessary for C. All but the first segment are standard.

RESID		SEGMENT PARA PUBLIC 'CODE'	; Resident Code segment
RESID		ENDS
RESIDEND	SEGMENT WORD PUBLIC 'CODE'	; Reserved to compute RESID size
RESIDEND	ENDS
_TEXT		SEGMENT WORD PUBLIC 'CODE'	; Discardable Code segment

		; Routines in BIOS.LIB that are useful from assembly language
EXTERNCFASTPROC disable_kbd_n_mouse
EXTERNCFASTPROC display_field
EXTERNCFASTPROC download_user_pw
EXTERNCFASTPROC find_tsr
EXTERNCFASTPROC malloc
EXTERNCFASTPROC restore_kbd_n_mouse
EXTERNCFASTPROC read_8042
EXTERNCFASTPROC wait_8042_ibe
EXTERNCFASTPROC wait_8042_obf
EXTERNCFASTPROC wait_refresh
EXTERNCFASTPROC write_8042

_TEXT		ENDS
C_ETEXT 	SEGMENT PARA PUBLIC 'ENDCODE'	; Marker for the end of code
C_ETEXT 	ENDS
FAR_DATA	SEGMENT PARA PUBLIC 'FAR_DATA'	; All far data segments
FAR_DATA	ENDS
NULL		SEGMENT PARA PUBLIC 'BEGDATA'	; What NULL pointers address
NULL		ENDS
CONST		SEGMENT WORD PUBLIC 'CONST'	; Constants
CONST		ENDS
_DATA		SEGMENT WORD PUBLIC 'DATA'	; Initialized variables
_DATA		ENDS
_BSS		SEGMENT WORD PUBLIC 'BSS'	; Unintialized variables
_BSS		ENDS
C_COMMON	SEGMENT WORD PUBLIC 'BSS'	; Unintialized variables
C_COMMON	ENDS
STACK		SEGMENT WORD STACK 'STACK'	; Stack
STACK		ENDS


; ~~jfl 95/02/23 The .model directive must be located behind the segment
; definitions to make sure the RESID segment is ordered ahead of _TEXT.

		.model	small, C	; To use the tiny memory model, just
					; add the _TEXT and RESID segments to
					; group DGROUP in your startup module.

DGROUP		GROUP	NULL, CONST, _DATA, _BSS, C_COMMON, STACK


		.LIST
