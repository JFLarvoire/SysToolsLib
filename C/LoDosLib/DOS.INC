.XLIST
;*****************************************************************************;
;									      ;
;	Filename:	DOS.INC 			    		      ;
;									      ;
;	Description:	MS-DOS macros, adapted from the Vectra DOS reference  ;
;									      ;
;	Author: 	Jean-Francois Larvoire		    1988/04/05	      ;
;									      ;
;      (c) Copyright 1988-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

FALSE		equ	0	    ; Warning: NOT is an arithmetic operator in
TRUE		equ	1	    ; ASM, not a logical operator like ! in C

DOS		equ	21H

stdin		equ	0	    ; Standard file handles
stdout		equ	1
errout		equ	2
stderr		equ	2
stdaux		equ	3
stdprn		equ	4

;*****************************************************************************;
;		Structures from the DOS 5 programmers reference 	      ;
;*****************************************************************************;

BOOTSECTOR	struc		    ; Boot sector header

    bsJump	    db	3 dup (?)   ; E9 XX XX or EB XX 90
    bsOemName	    db	'????????'  ; OEM name and version
				    ; Start of BIOS parameter block (BPB)
    bsBytesPerSec   dw	?	    ; Bytes per sector
    bsSecPerClust   db	?	    ; Sectors per cluster
    bsResSectors    dw	?	    ; Number of reserved sectors
    bsFATs	    db	?	    ; Number of file allocation tables
    bsRootDirEnts   dw	?	    ; Number of root directory entries
    bsSectors	    dw	?	    ; Total number of sectors
    bsMedia	    db	?	    ; Media descriptor
    bsFATsecs	    dw	?	    ; Number of sectors per FAT
    bsSecPerTrack   dw	?	    ; Sectors per track
    bsHeads	    dw	?	    ; Number of heads
    bsHiddenSecs    dd	?	    ; Number of hidden sectors
    bsHugeSectors   dd	?	    ; Number of sectors if dpSectors = 0
				    ; End of BIOS parameter block (BPB)
    bsDriveNumber   db	?	    ; Drive number (80h)
    bsReserved1     db	?	    ; Reserved
    bsBootSignature db	?	    ; Extended boot signature (29h)
    bsVolumeID	    dd	?	    ; Volume ID number
    bsVolumeLabel   db	11 dup (?)  ; Volume label
    bsFileSysType   db	8 dup (?)   ; File system type

BOOTSECTOR	ends

DEVICEPARAMS	struc		    ; Device Parameters Table

    dpSpecFunc	    db	?	    ; Special functions
    dpDevType	    db	?	    ; Device type
    dpDevAttr	    dw	?	    ; Device attribute
    dpCylinders     dw	?	    ; Number of cylinders
    dpMediaType     db	?	    ; Media type
				    ; Start of BIOS parameter block (BPB)
    dpBytesPerSec   dw	?	    ; Bytes per sector
    dpSecPerClust   db	?	    ; Sectors per cluster
    dpResSectors    dw	?	    ; Number of reserved sectors
    dpFATs	    db	?	    ; Number of file allocation tables
    dpRootDirEnts   dw	?	    ; Number of root directory entries
    dpSectors	    dw	?	    ; Total number of sectors
    dpMedia	    db	?	    ; Media descriptor
    dpFATsecs	    dw	?	    ; Number of sectors per FAT
    dpSecPerTrack   dw	?	    ; Sectors per track
    dpHeads	    dw	?	    ; Number of heads
    dpHiddenSecs    dd	?	    ; Number of hidden sectors
    dpHugeSectors   dd	?	    ; Number of sectors if dpSectors = 0
				    ; End of BIOS parameter block (BPB)
DEVICEPARAMS	ends

EXEHEADER   struc		    ; .EXE file header

    exSignature     dw	5A4DH	    ; .EXE signature
    exExtraBytes    dw	?	    ; Number of bytes in last partial page
    exPages	    dw	?	    ; Number of sectors in file
    exRelocItems    dw	?	    ; Number of pointers in relocation table
    exHeaderSize    dw	?	    ; Size of header in paragraphs
    exMinAlloc	    dw	?	    ; Minimum allocation
    exMaxAlloc	    dw	?	    ; Maximum allocation
    exInitSS	    dw	?	    ; Initial SS value
    exInitSP	    dw	?	    ; Initial SP value
    exCheckSum	    dw	?	    ; Complemented checksum
    exInitIP	    dw	?	    ; Initial IP value
    exInitCS	    dw	?	    ; Initial CS value
    exRelocTable    dw	?	    ; Byte offset to relocation table
    exOverlay	    dw	?	    ; Overlay number

EXEHEADER   ends

FCB		struc		    ; File control block

    fcbDriveID	    db	?	    ; Drive no (0=default, 1=A, etc...)
    fcbFileName     db	'????????'  ; File name
    fcbExtent	    db	'???'	    ; File extension
    fcbCurBlockNo   dw	?	    ; Current block number
    fcbRecSize	    dw	?	    ; Record size
    fcbFileSize     dd	?	    ; File size
    fcbDate	    dw	?	    ; Date last modified
    fcbTime	    dw	?	    ; Time last modified
    fcbReserved     db	8 dup (?)   ; Reserved
    fcbCurRecNo     db	?	    ; Current record number
    fcbRandomRecNo  dd	?	    ; Random record number

FCB		ends

FILEINFO	struc		    ; File information

    fiReserved	    db	21 dup (?)
    fiAttribute     db	?
    fiFileTime	    dw	?
    fiFileDate	    dw	?
    fiSize	    dd	?
    fiFileName	    db	13 dup (?)

FILEINFO	ends

MID		struc		    ; Media ID block

    midInfoLevel    dw	0	    ; Information level
    midSerialNum    dd	?	    ; Serial number
    midVolLabel     db	11 dup (?)  ; ASCII volume label, with trailing spaces
    midFileSysType  db	8 dup (?)   ; File system type

MID		ends

PARTENTRY	STRUC
    peBootable	    db	?	    ; 80H = Bootable, 00H = Non bootable
    peBeginHead     db	?	    ; Beginning head
    peBeginSector   db	?	    ; Beginning sector
    peBeginCylinder db	?	    ; Beginning cylinder
    peFileSystem    db	?	    ; 0=?, 1=FAT12, 4=FAT16<32MB, 6=FAT16>32MB
    peEndHead	    db	?	    ; Ending head
    peEndSector     db	?	    ; Ending sector
    peEndCylinder   db	?	    ; Ending cylinder
    peStartSector   dd	?	    ; Starting sector relative to disk
    peSectors	    dd	?	    ; Number of sectors in partition
PARTENTRY	ENDS

RWBLOCK 	STRUC		    ; IOCtl Read/Write block

    rwSpecFunc	    db	0	    ; Special function (must be 0)
    rwHead	    dw	?	    ; Head to read/write
    rwCylinder	    dw	?	    ; Cylinder to read/write
    rwFirstSector   dw	?	    ; First sector to read/write
    rwSectors	    dw	?	    ; Number of sectors to read/write
    rwBuffer	    dd	?	    ; Address of buffer for read/write data

RWBLOCK 	ENDS


;*****************************************************************************;
;	     Utility macros used throughout the rest of this file	      ;
;*****************************************************************************;

; macro that loads a register with a value (a data or another register)
mov_reg		macro	to, from
		ifdif	<to>, <from>	;; if not already in the good register
		 mov	 to, from
		endif
		endm

; macro that loads a segment register from memory or from another register
mov_seg		macro	to, from
		ifdif	<to>, <from>	;; if not already in the good register
mov_seg_found	 =	 FALSE
		 irp	 s, <cs, ds, es, ss>	;; Check if <from> is a segment
		  ifidn	  <from>, <s>
		   push	   from			;; If it is, the transfer has
		   pop	   to			;; to go through the stack
mov_seg_found	   =	   TRUE
		   exitm			;; and the macro is finished.
		  endif
		 endm
		 if	 mov_seg_found EQ FALSE ;; Don't use NOT which is arith.
		  mov	  to, from		;; Else a simple move will do
		 endif
		endif
		endm

; macro that loads a register with a pointer (a label or another register)
mov_ptr		macro	reg, pointer
		ifdif	<pointer>, <reg> ;; if not already in the good register
mov_ptr_found	 =	 FALSE
		 irp	 reg2, <ax, bx, cx, dx, si, di, bp, sp>	;; test if it's
		  ifidn	  <pointer>, <reg2>	;; a register to register move.
		   mov	   reg, reg2
mov_ptr_found	   =	   TRUE
		   exitm
		  endif
		 endm
		 ife	 mov_ptr_found	;; if not found, then it's a label
		  mov	  reg, offset DGROUP:pointer
		 endif
		endif
		endm

say2		macro	mot1, mot2, mot3, mot4, mot5, mot6, mot7, mot8, mot9
;		 IF2
		 %out	mot1 mot2 mot3 mot4 mot5 mot6 mot7 mot8 mot9
;		 ENDIF
		endm

;*****************************************************************************;
;         Macros adapted from the Vectra MS-Dos programmers reference         ;
;*****************************************************************************;

; All lines using a pointer, such as "mov reg, offset label"
; were replaced by "mov_ptr reg, label" to allow to use register pointers

read_kbd_and_echo macro
		mov	ah, 01h
		int	DOS
		endm

display_char	macro	char
		mov_reg dl, char
		mov	ah, 02h
		int	DOS
		endm

print_char	macro	char
		mov_reg dl, char
		mov	ah, 05h
		int	DOS
		endm

; Direct input/output to console. No control-C check.
; Switch=FF : Read char if any into AL and clear ZF. Else is no char, set ZF.
; Else	    : Switch is a character sent to standard output.

dir_console_io	macro	switch
		mov_reg dl, switch
		mov	ah, 06H
		int	DOS
		endm

; Direct input from console. No control-C check.

dir_console_input macro
		mov	ah, 07H
		int	DOS		; Character in AL
		endm

read_kbd	macro
		mov	ah, 08H
		int	DOS		; Character in AL
		endm

display 	macro	string
		mov_ptr	dx, string
		mov	ah, 09H
		int	DOS		; No error returned
		endm

; Buffered keyboard input
;  ds:dx = Pointer to structure:
;		db	limit		; Buffer length, including room for CR
;		db	datalength	; Output: Data read, not counting CR
;		db	limit dup (?)	; Buffer for data to be read

get_string	macro	string, limit
		mov_ptr dx, string
		IFNB	limit
		push	bx
		mov	bx, dx
		mov_reg ah, limit
		mov	[bx], ah
		pop	bx
		ENDIF
		mov	ah, 0AH
		int	DOS
		endm

check_kbd_status macro
		mov	ah, 0Bh
		int	DOS		; AL=0 (buffer empty), FF (code ready)
		endm

flush_and_read_kbd macro switch 	; Flush keyboard and read without echo
		mov_reg al, switch	; MS-DOS function to call (1/6/7/8/A)
		mov	ah, 0CH
		int	DOS
		endm

reset_disk	macro
		mov	ah, 0Dh
		int	DOS		; AL=0 (buffer empty), FF (code ready)
		endm

select_disk	macro	disk
		mov_reg	dl, disk	; 0=A, 1=B...
		mov	ah, 0EH
		int	DOS		; Returns the number of drives in AL
		endm

open		macro	fcb
		mov_ptr dx, fcb
		mov	ah, 0FH
		int	DOS		; Returns AL = 0:found; FF:Not found
		endm

close		macro	fcb
		mov_ptr dx, fcb
		mov	ah, 10H 	; Returns AL = 0:found; FF:Not found
		int	DOS
		endm

read_seq	macro	fcb
		mov_ptr dx, fcb
		mov	ah, 14H 	; Returns AL = 0:OK, 1:EOF, 2:L>DTA,
		int	DOS		;  3:Data<L
		endm

write_seq	macro	fcb
		mov_ptr dx, fcb
		mov	ah, 15H 	; Returns AL = 0:OK, 1:DskFull, 2:L>DTA
		int	DOS
		endm

create		macro	fcb
		mov_ptr dx, fcb
		mov	ah, 16H
		int	DOS		; Returns AL = 0:done; FF:Not done
		endm

current_disk	macro
		mov	ah, 19h
		int	DOS		; Returns the current drive in AL
		endm

set_dta		macro	buffer
		mov_ptr	dx, buffer	; ds:dx -> disk transfer buffer
		mov	ah, 1AH
		int	DOS		; No error returned
		endm

set_vector	macro	interrupt, handler, segment
		mov_reg	al, interrupt
		SET_VECTOR_PTR_SIZE = 0
		say2	Type de handler : %(TYPE handler)
		IF	(TYPE handler) EQ 0		; Register, constant
		 mov_reg dx, handler
		 SET_VECTOR_PTR_SIZE = 2
		ENDIF
		IF	(TYPE handler) EQ 65535		; Near label
		 mov	 dx, offset handler
		 SET_VECTOR_PTR_SIZE = 2
		ENDIF
		IF	(TYPE handler) EQ 65534		; Far label
		 IFB	 <segment>
		  push	  ds
		  mov	  dx, seg handler
		  mov	  ds, dx
		  SET_VECTOR_PTR_SIZE = 4
		 ELSE
		  SET_VECTOR_PTR_SIZE = 2
		 ENDIF
		 mov	 dx, offset handler
		ENDIF
		IF	(TYPE handler) EQ 2		; Word data
		 mov	 dx, handler
		 SET_VECTOR_PTR_SIZE = 2
		ENDIF
		IF	(TYPE handler) EQ 4		; Dword data
		 push	 ds
		 lds	 dx, handler
		 SET_VECTOR_PTR_SIZE = 4
		ENDIF
		IF	SET_VECTOR_PTR_SIZE EQ 0	; If dx not loaded yet
		 %out	 Invalid pointer argument passed to set_vect
		 .ERR
		ENDIF
		IFNB	<segment>
		 IF	 SET_VECTOR_PTR_SIZE EQ 2
		  push	  ds
		 ENDIF
		 mov_seg ds, segment
		 SET_VECTOR_PTR_SIZE = 4
		ENDIF
		mov	ah, 25H
		int	21H		; No returns
		IF	SET_VECTOR_PTR_SIZE EQ 4
		 pop	 ds
		ENDIF
		endm

get_date	macro
		mov	ah, 2AH
		int	DOS
		endm

; AL = week_day (0=Sunday, 1=monday, etc...)
; CX = Year (1980-2099)
; DH = month (1=Jan, 2=Feb, etc...)
; DL = Day (1 - 31)

set_date	macro	year, month, day
		mov_reg cx, year
		mov_reg dh, month
		mov_reg dl, day
		mov	ah, 2BH
		int	DOS
		endm

get_time	macro
		mov	ah, 2CH
		int	DOS
		endm

; CH = Hour (0 - 23)
; CL = minute (0 - 59)
; DH = Second (0 - 59)
; DL = Hundredth (0 - 99)

set_time	macro	hour, minute, second, hundredth
		mov_reg ch, hour
		mov_reg cl, minute
		mov_reg dh, second
		mov_reg dl, hundredth
		mov	ah, 2DH
		int	DOS
		endm

get_dta 	macro
		mov	ah, 2FH
		int	DOS		; Returns DTA in es:bx
		endm

get_version	macro
		mov	ah, 30H
		int	DOS		; AL=major, AH=minor
		endm			; BH=OEM serial #, BL:CX=24 bits user #

keep_process	macro	paragraphs, return_code
		mov_reg	dx, paragraphs
		ifnb	<return_code>
		 mov_reg al, return_code
		else
		 mov	 al, 0
		endif
		mov	ah, 31H
		int	DOS		; Does not return
		endm

ctrl_c_chk	macro	action, state	; Control-C check
		mov_reg al, action	; AL = 0: Get state; AL = 1: Set state.
		IFNB	<state>
		mov_reg dl, state	; If AL == 1, DL = 0: Off; DL = 1: On.
		ENDIF
		mov	ah, 33H
		int	DOS		; DL = state. 0 = Off; 1 = On.
		endm

get_indos_flag	macro
		mov	ah, 34H
		int	DOS
		endm

get_vector	macro	interrupt
		mov_reg	al, interrupt
		mov	ah, 35H
		int	DOS		; Returns vector in es:bx
		endm

create_dir	macro	buffer
		mov_ptr	dx, buffer
		mov	ah, 39H
		int	DOS
		endm

; If there is an error, the carry is set and the error code is stored in AX:
;  Error 3: The pathname does not exist.
;  Error 5: The directory, or a file with the same name, already exists.

remove_dir	macro	buffer
		mov_ptr	dx, buffer
		mov	ah, 3AH
		int	DOS
		endm

; If there is an error, the carry is set and the error code is stored in AX:
;  Error 3: The pathname does not exist.
;  Error 5: The directory is not empty, or is the root directory.
;  Error 10h: This is the current directory.

change_dir	macro	buffer
		mov_ptr	dx, buffer
		mov	ah, 3BH
		int	DOS
		endm

; If there is an error, the carry is set and the error code is stored in AX:
;  Error 3: The pathname does not exist, or specifies a file but not a dir.

create_handle	macro	path, attrib
		mov_ptr	dx, path
		mov_reg	cx, attrib
		mov	ah, 3CH
		int	DOS
		endm

; If the file does not exist, it is created. If it exists, it is truncated to
; a size of 0.

; File attributes:
; 00H: Normal
; 01H: Read only
; 02H: Hidden
; 04H: System
; 08H: Volume ID
; 10H: Subdirectory
; 20H: Archive bit. Set whenever the file is changed. Reset by Backup.

; Returns a file handle in AX if the carry is not set.
; Else AX contains the following error codes:
;  3: Invalid or inexisting path
;  4: No more handles
;  5: File can't created (directory full, already exists with restrictive
;	 attributes...)

open_handle	macro	path, access
		mov_ptr	dx, path		;; ds:dx -> file pathname
		mov_reg	al, access		;; Access bits. See below.
		mov	ah, 3DH
		int	DOS
		endm

; Description of the access bits: ISSSAAAA

; I = Inherit bit. I=0 ==> File inherited by children. I=1 ==> Not inherited.
; SSS = Sharing mode. Specifies what access the other processes have to the
; same file.
; SSS=000 ==> Compatibility. Any process can open it in this same mode.
; SSS=001 ==> Deny all sharing. Fails if already opened in any mode.
; SSS=010 ==> Deny write. Fails if any other process already has write access.
; SSS=011 ==> Deny read. Fails if any other process already has read access.
; SSS=100 ==> Deny compatibility. Fails if already opened in compatib. mode.
; AAAA = Access code. Specifies how the file will be used.
; AAAA=0000 ==> Open to read. Fails if already opened in deny read or deny all.
; AAAA=0001 ==> Open to write. Fails if already opened in deny write "" "" "".
; AAAA=0010 ==> Open to read and write. Fails if already opened in a deny mode.

; Returns a file handle in AX if the carry is not set.
; Else AX contains the following error codes:
;  1: File sharing TSR not loaded.
;  2: Invalid or inexisting file
;  3: Invalid or inexisting path
;  4: No more handles
;  5: File can't be opened (directory, read only, volume ID, system...)
; 12: Invalid access code

close_handle	macro	handle
		mov_reg	bx, handle
		mov	ah, 3EH
		int	DOS
		endm

; If there is an error, the carry is set and the error code is returned in AX
;  6: Handle not opened or invalid.

read_handle	macro	handle, buffer, bytes
		mov_reg	bx, handle
		mov_ptr	dx, buffer		;; ds:dx -> buffer
		mov_reg	cx, bytes
		mov	ah, 3FH
		int	DOS
		endm

; If there is an error, the carry is set and the error code is returned in AX
;  5: Handle not opened for reading.
;  6: Handle not opened or invalid.

write_handle	macro	handle, buffer, bytes
		mov_reg	bx, handle
		mov_ptr	dx, buffer		;; ds:dx -> buffer
		mov_reg	cx, bytes
		mov	ah, 40H
		int	DOS
		endm

; If there is an error, the carry is set and the error code is returned in AX
;  5: Handle not opened for writing.
;  6: Handle not opened or invalid.

delete_entry	macro	path
		mov_ptr	dx, path		;; ds:dx -> file pathname
		mov	ah, 41H
		int	DOS
		endm

; If there is an error, the carry is set and the error code is returned in AX
;  2: Pathname invalid or file does not exist.
;  5: Pathname specifies a directory or a read only file.

move_pointer	macro	handle, high, low, method
		mov_reg	bx, handle
		mov_reg	cx, high
		mov_reg	dx, low
		mov_reg	al, method	;; 0=absolute, 1= relative, 2=from end
		mov	ah, 42H
		int	DOS
		endm

; If the file pointer has been successfully moved, the carry is cleared,
; and DX:AX contains the new absolute location of the pointer.
; If there is an error, the carry is set and the error code is returned in AX
;  1: AL not 0, 1 or 2.
;  6: Handle not opened or invalid.

change_attrib	macro	path, action, attrib
		mov_ptr	dx, path	;; ds:dx -> pathname
		mov_reg	al, action	;; 0=Get attributes, 1=Set attributes.
		mov_reg	cx, attrib	;; Used if al=1.
		mov	ah, 43H
		int	DOS
		endm

; If successful, the carry is cleared and the attributes are returned in cx.
; Else, the carry is set and the error code is returned in AX
;  1: AL not 0 or 1.
;  3: Invalid or inexisting path
;  5: Attribute can't be changed (directory or volume ID)

ioctl_get_data	macro	handle
		mov_reg	bx, handle
		mov	ax, 4400H
		int	DOS
		endm

; If it a failure (carry set), ax contains the following error codes:
;  1: Invalid function
;  6: Invalid handle
; If the call is successfull (carry not set), dx contains device data.
; If bit 7 of dx is 0, then the handle refers to a file, and the other bits
; mean:
;  bits 15-8: reserved
;  bit 6: 0 if the file has been written
;  bits 5-0: Drive number (0=A, 1=B, etc...)
; If bit 7 of dx is 1, then the handle refers to a device, and the other bits
; mean:
;  bit 15: reserved
;  bit 14: (read only) 1 if the device can process ioctl_send/receive_char.
;  bits 13-8: reserved
;  bit 6: 0 if end of file on input
;  bit 5: If 0, then check for control characters (Ctrl-P, Ctrl-S, Ctrl-Z)
;  bit 4: reserved
;  bit 3: Clock device
;  bit 2: Null device
;  bit 1: Console output device
;  bit 0: Console input device

ioctl_set_data	macro	handle, data
		mov_reg	bx, handle
		mov_reg	dx, data
		sub	dh, dh
		mov	ax, 4401H
		int	DOS
		endm

; See previous function for data format, and error codes.

set_media_id	macro	drive, idstruc
		mov_reg bx, drive	; 0=current, 1=A, 2=B, ... , 26=Z
		mov_ptr dx, idstruc	; DS:DX = Pointer to an MID structure
		mov	cx, 0846H
		mov	ax, 440DH
		int	DOS
		endm

get_media_id	macro	drive, idstruc
		mov_reg bx, drive	; 0=current, 1=A, 2=B, ... , 26=Z
		mov_ptr dx, idstruc	; DS:DX = Pointer to an MID structure
		mov	cx, 0866H
		mov	ax, 440DH
		int	DOS
		endm

xdup		macro	handle
		mov_reg	bx, handle
		mov	ah, 45H
		int	DOS
		endm

; If a new handle has been successfully created, the carry is cleared,
; and the handle is returned in AX
; If there is an error, the carry is set and the error code is returned in AX
;  4: No more handles
;  6: Handle not opened or invalid.

xdup2		macro	handle1, handle2
		mov_reg	bx, handle1
		mov_reg	cx, handle2
		mov	ah, 46H
		int	DOS
		endm

; If the handle in CX now refers to the same file as that in BX, the carry
; is cleared. (If CX refered to an open file, it is closed).
; If there is an error, the carry is set and the error code is returned in AX
;  4: No more handles
;  6: Handle not opened or invalid.

get_dir		macro	drive, buffer
		mov_reg	dl, drive	;; 0=default, 1=A, 2=B...
		mov_ptr	si, buffer	;; DS:SI -> 64 bytes Buffer
		mov	ah, 47H
		int	DOS
		endm

; Returns the pathname of the current directory on the given drive. It does
; not include the drive letter, colon nor initial backslash.

allocate_memory	macro	paragraphs	;; Was bytes in the original macro
		mov_reg	bx, paragraphs
		mov	ah, 48H
		int	DOS
		endm

; If the request fails, the carry is set and bx contains the size in paragraphs
; of the largest allocatable block. In addition, AX contains the error code
;  7: Memory control blocks damaged.
;  8: Not enough memory.

free_memory	macro	segment
		ifdif	<es>, <segment>
		 push	 es
		 mov_seg es, segment
		endif
		mov	ah, 49H
		int	DOS
		ifdif	<es>, <segment>
		 pop	 es
		endif
		endm

; If the request fails, the carry is set and AX contains the error code
;  7: Memory control blocks damaged.
;  9: Not a valid memory address in ES.

set_block	macro	paragraphs, segment	; Was bytes in the original mac
		mov_reg	bx, paragraphs
		ifnb	<segment>
		 push	 es
		 mov_seg es, segment
		endif
		mov	ah, 4AH
		int	DOS
		ifnb	<segment>
		 pop	 es
		endif
		endm

; If the request fails, the carry is set and bx contains the size in paragraphs
; of the largest allocatable block. In addition, AX contains the error code
;  7: Memory control blocks damaged.
;  8: Not enough memory.
;  9: Not a valid memory address in ES.

exec		macro	program, arguments, parms_block, segenv
		mov_ptr	dx, program			; ds:dx
		mov_ptr	bx, parms_block			; es:bx
		IFNB	<segenv>
		mov	word ptr [bx+00H], segenv	; Use given environment
		ELSE
		mov	word ptr [bx+00H], 0		; Use parent's environ.
		ENDIF
		mov_ptr	<word ptr [bx+02H]>, arguments	; dword & of arg. line
		mov	word ptr [bx+04H], cs
		mov	word ptr [bx+06H], 5Ch		; dword & of FCB1
		mov	word ptr [bx+08H], es
		mov	word ptr [bx+0AH], 6CH		; dword & of FCB2
		mov	word ptr [bx+0CH], es
		mov	al, 0
		mov	ah, 4BH
		int	DOS
		endm

; If the request fails, the carry is set and AX contains the error code
;  1: AL not 0 or 3
;  2: Program not found or invalid path
;  8: Not enough memory
; 11: Invalid EXE file structure

end_process	macro	return_code
		mov_reg	al, return_code
		mov	ah, 4CH
		int	DOS
		endm

ret_code	macro
		mov	ah, 4DH
		int	DOS
		endm

find_first_file macro	attrib, name
		mov_reg cx, attrib
		mov_ptr dx, name
		mov	ah, 4EH
		int	DOS
		endm

find_next_file	macro
		mov	ah, 4FH
		int	DOS
		endm

set_current_psp macro	psp
		mov_reg	bx, psp
		mov	ah, 50H
		int	DOS
		endm

get_current_psp macro
		mov	ah, 51H
		int	DOS
		endm

rename_file	macro	old_path, new_path
		mov_ptr dx, old_path		; Relative to DS
		mov_ptr di, new_path		; Relative to ES
		mov	ah, 56H
		int	DOS
		endm

get_file_date	macro	handle
		mov	bx, handle
		mov	ax, 5700H
		int	DOS	    ; Returns CX=time 5H6M5S, DX=Date 7Y4M5D
		endm

commit_file	macro	handle
		mov_reg bx, handle
		mov	ah, 68H
		int	DOS
		endm

;*****************************************************************************;
;			Miscellanneous definitions			      ;
;*****************************************************************************;

; Awfull macro that aligns the program counter to the next paragraph address
PARAGRAPH	macro
		local	temp
		org	$-1
temp:
		org	0
		org	temp + 16 - ((temp-$) mod 16)
		endm

;*****************************************************************************;

; Two macros that ease recovery when DOS returns an error
error		macro	message, restart_label
		display message
		jmp	restart_label
		endm

test_error	macro	message, restart_label
		local	no_error
		jnc	no_error
		error	message, restart_label
no_error:
		endm

check_error	macro
		local	an_error
		jc	an_error
		xor	ax, ax
an_error:
		endm

;*****************************************************************************;
.LIST
