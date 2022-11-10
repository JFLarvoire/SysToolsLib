	page	, 132
;*****************************************************************************;
;									      ;
;   FileName:	    cpuid.asm						      ;
;									      ;
;   Contents:	    Identify the processor				      ;
;									      ;
;   Notes:								      ;
;									      ;
;   History:								      ;
;    1993-12-17 JFL Jean-Francois Larvoire created this file.		      ;
;    1995-02-17 JFL Extracted the routine from the Pike BIOS.		      ;
;    1995-09-04 JFL Removed an unnecessary instruction. Updated comments.     ;
;    2022-11-10 JFL Added the extended family calculation.                    ;
;									      ;
;      (c) Copyright 1993-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.286

.model small, C

.code

PAGE
;===FUNCTION HEADER===========================================================;
;									      ;
;	NAME:		identify_processor				      ;
;									      ;
;	DESCRIPTION:	Tell which generation of processor we're running on   ;
;									      ;
;	PARAMETERS:							      ;
;									      ;
;	    ON ENTRY:	None						      ;
;									      ;
;	    ON EXIT:	AX = Processor generation:			      ;
;                           0 = 8086					      ;
;                           1 = 80186					      ;
;                           2 = 80286					      ;
;                           3 = 80386					      ;
;                           4 = 80486					      ;
;                           5 = Pentium					      ;
;                           6 = Pentium Pro, P2, P3, and all later Core procs.;
;                           7 = Itanium					      ;
;                           15 = Pentium 4				      ;
;                           16 = Itanium 2 early steppings		      ;
;                           17 = Itanium 2 later steppings		      ;
;									      ;
;	REGS ALTERED:	AX, BX, CX, DX, ES				      ;
;									      ;
;	HISTORY:							      ;
;	  1993-12-17 JFL Created for Pike BIOS				      ;
;	  1995-09-04 JFL Removed an unnecessary instruction. Updated comments.;
;         2022-11-10 JFL Added the extended family calculation.               ;
;									      ;
;=============================================================================;

.386

is_486_or_more  proc    near

                push    bp
                mov     bp, sp                  ; Save the initial stack pointer
                and     sp, 0FFFCH              ; Make sure SP is DWORD-aligned

                push    eax
                pushfd                          ; Save initial AC & DI flags
                cli                             ; Prevent ints while AC is on

                pushfd                          ; Copy EFlags to EAX
                pop     eax
                bts     eax, 18                 ; Set the Alignment Check flag
                push    eax                     ; Copy EAX to EFlags
                popfd
                pushfd                          ; Copy EFlags to EAX
                pop     eax
                bt      eax, 18                 ; Test the Alignment Check flag
                sbb     ax, ax                  ; CY->FFFF; NC->0000

                popfd                           ; Restore the initial AC & DI f.
                add     al, 1                   ; CY on 486; NC on 386
                pop     eax

                mov     sp, bp
                pop     bp
                ret

is_486_or_more  endp

is_586_or_more  proc    near                    ; Carry set if at least a 586

                push    ebx
                push    eax

                pushfd                          ; Copy EFlags to EAX
                pop     eax
                btc     eax, 21                 ; Complement the ID flag
                push    eax                     ; Copy EAX to EFlags
                popfd
                pushfd                          ; Copy EFlags to EAX
                pop     ebx
                xor     eax, ebx
                bt      eax, 21                 ; Test the ID flag
                cmc

                pop     eax
                pop     ebx
                ret

is_586_or_more  endp

.8086

identify_processor  proc    public

                push    bx
                push    cx
                push    dx
                xor     dx, dx                  ; Assume 8086 class

                ; Is this a 80186 or better?

                mov     al, -1
                mov     cl, 41H                 ; The 8086 shifts using cl,
                shr     al, cl                  ;  newer ÊP use cl modulo 32
                jz      SHORT identified        ; It's a 8086

                inc     dx                      ; It's at least a 186

                ; Is this a 80286 or better?

                push    sp                      ; The 80286 pushes SP
                pop     ax                      ; The older ÊP push SP-2
                sub     ax, sp                  ; 0 on a 80286; -2 on older ÊP.

                jnz     SHORT identified        ; It's a 186

                inc     dx                      ; This is at least a 286

                ; Is this a 80386 or better?

                mov     ax, 7000H
                push    ax
                popf
                pushf
                pop     bx
                and     ax, bx                  ; On the 286 flags 14:12 are
                jz      SHORT identified        ; always 0. Jump if it's a 286

                inc     dx                      ; This is at least a 386

                ; Is this a 80486 or better?

                call    is_486_or_more
                jnc     SHORT identified        ; It's a 386

                inc     dx                      ; This is at least a 486

                ; Is this a Pentium or better?

                call    is_586_or_more
		jnc	SHORT identified	; It's a 486 DX, SX, DX2, SX2

		; ~~jfl 950904 Don't inc dx now ; This is at least a 486 DX4

		; What is it really?

		.386				; We know it's at least a 486

                mov     eax, 1                  ; Request the Family/Model/Step
                db      0FH, 0A2H               ; CPUID instruction
		mov	dl, ah			; Family in EAX[11:8]
		and	dx, 0FH 		; DX = Family

		; Add the extended family if present

		cmp	dx, 0FH			; Do we have an extended family?
		jne	SHORT identified
		shr	eax, 20			; Extended family in EAX[27:20]
		and	ax, 0FFH		; AX = Extended family
		add	dx, ax			; DX = Family + Extended family

		.8086
identified:
                mov     ax, dx
                pop     dx
                pop     cx
                pop     bx
                ret

identify_processor  endp

end
