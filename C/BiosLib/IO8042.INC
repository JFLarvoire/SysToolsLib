	PAGE	,132
	TITLE	System utilities, not relying on MS-DOS.
;*****************************************************************************;
;									      ;
;   FILE NAME:	    IO8042.INC						      ;
;									      ;
;   DESCRIPTION:    All constants related to 8042, keyboard, and mouse.	      ;
;									      ;
;   NOTES:	    Most constants are named like in the BIOS.		      ;
;									      ;
;   HISTORY:								      ;
;    1992/01/27 JFL Adapted from the BIOS file IO8041.EQU.		      ;
;		    Replaced everywhere "8041" by "8042".		      ;
;    1994/05/10 JFL Added updates from recent versions of IO8041.EQU.	      ;
;		    							      ;
;      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      ;
; Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 ;
;*****************************************************************************;

.XLIST

;=============================================================================;
;				System ports				      ;
;=============================================================================;

SPU_CONTROL_PORT	equ	61H
REFRESH_MASK		equ	10H

;=============================================================================;
;				8042 Commands				      ;
;=============================================================================;

IO_8042_DATA		equ  60h	; 8042 data port (read/write)

IO_8042_STATUS		equ  64h	; 8042 status port (read)
					; The next 5 constants are obsolete
I8042_PARITY_ERROR	equ  10000000b	; Last byte had even parity - error
I8042_REC_TIMEOUT	equ  01000000b	; Transmission started, but not complete
I8042_TRANS_TIMEOUT	equ  00100000b	; Transmission from keyboard not completed
I8042_KBD_INHIBIT	equ  00010000b	; Keyboard NOT inhibited
I8042_CMD_DATA		equ  00001000b	; 1 = command byte; 0 = data byte
I8042_SYSTEM_FLAG	equ  00000100b	; System flag - set to 0 after power on reset
I8042_IBF		equ  00000010b	; Input buffer full flag
I8042_OBF		equ  00000001b	; Output buffer full flag
AUX_OUT_BUFFER_FULL 	equ  00100000b  ; For 8042 aux. output buffer full status.

IO_8042_CMD		equ  64h	; 8042 command port (write)
READ_CMD_BYTE		equ  020h	; 8042 command to read command byte
I8042_WRITE_CMD 	equ  060h	; Write KBD controller command byte (1st byte)
I8042_INIT_CMD		equ   01100101b ; Initial Command byte (2nd byte) ~~tt 8/17/89
					;   bit 7: Reserved = 0
					;   bit 6: Keyboard translate mode = 1
					;   bit 5: Disable AUX device = 1
					;   bit 4: Disable keyboard = 1
					;   bit 3: Override inhibit switch = 0
					;   bit 2: System flag = 1
					;   bit 1: Enable AUX interrupt = 1
					;   bit 0: Enable OBF interrupt = 1
DIS_KBD_N_MOUSE 	equ   00110000b ; disable kbd and mouse ~~jwy 9/15/89
TEST_MOUSE_IRQ		equ   00110010b ; Idem, but allow mouse IRQs ~jfl 3/28/91

CHECK_PW_CMD		equ  0A4h	; 8042 command to test if there is a pwd
LOAD_PW_CMD		equ  0A5h	; 8042 command to load password
ENABLE_SECURITY 	equ  0A6h	; 8042 command to lock the keyboard
AUX_DISABLE_DEVICE	equ  0A7h	; 8042 command to disable aux device
AUX_ENABLE_DEVICE	equ  0A8h	; 8042 command to enable aux device
AUX_INTERFACE_TEST	equ  0A9h	; 8042 command to test aux. device interface
AUX_ST_COMPLETE 	equ   0AAh	; Aux. reply to aux. reset command

I8042_RESET_CMD 	equ  0AAh	; 8042 self-test/reset command
I8042_RESET_OK		equ   055h	; Successful response to reset command

I8042_INTRF_TEST_CMD	equ  0ABH	;KBD interface test command
I8042_INTRF_OK		equ   00H	;Successful response
I8042_INTRF_RESULT_MASK equ   0FH	;``JKL (1/31/87) extract kbd
					;intrf test error.
;I8042_KBD_DIAG_DUMP	equ  0ACh	; Keyboard diagnostic dump (Unsupported)
I8042_DISABLE_KBD_INTRF equ  0ADh	; Command to disable the keyboard
I8042_ENABLE_KBD_INTRF	equ  0AEh	; Command to enable the keyboard

I8042_READ_INPUT	equ  0C0h	; Read 8042 input port
INPUT_PORT_PWD_ENABLE	equ   10000000b ; 1 = Password enabled; 0 = disabled
VID_MONO_DISPLAY	equ   01000000b ; 1 = Primary display attached to MDA
INPUT_PORT_RESERVED	equ   00100000b ; Reserved (Manufacturing jumper not installed)
ENABLE_SYS_BOARD_MEM	equ   00010000b ; 1 = 2nd 256K on system board enabled
INPUT_PORT_KEY_CLICK	equ   00001000b ; Key click control
I8042_KBD_FUSE		equ   00000100b ; 1 = Power sense line fuse OK
INPUT_PORT_AUX_DATA	equ   00000010b ; Auxiliary input data line
INPUT_PORT_KBD_DATA	equ   00000001b ; Keyboard input data line

I8042_READ_OUTPUT	equ  0D0h	; read the 8042 output port
I8042_WRITE_OUTPUT	equ  0D1h	; write the 8042 output port
I8042_KBD_DATA		equ   10000000b ; keyboard data out line
I8042_KBD_CLOCK 	equ   01000000b ; keyboard clock out line
I8042_AUX_INT		equ   00100000b ; aux interrupt
I8042_KBD_INT		equ   00010000b ; keyboard interrupt
I8042_AUX_CLOCK 	equ   00001000b ; aux clock out line
I8042_AUX_DATA		equ   00000100b ; aux data out line
I8042_A20_GATE		equ   00000010b ; A20 gate status
I8042_RESET_BIT 	equ   00000001b ; system reset

WRITE_KDB_BUFFER	equ  0D2H	; Write keyboard output buffer
WRITE_AUX_BUFFER	equ  0D3H	; Write auxiliary device output buffer

AUX_WRITE_DEVICE	equ  0D4h	; 8042 command to write next byte to aux. device

RST_A20_COMMAND 	equ  0DDh	; 1 byte command to reset GATE A20
SET_A20_COMMAND 	equ  0DFh	; 1 byte command to set GATE A20

KC_GET_READY		equ  0DEh	; 8042 command to get ready for HP private command
I8042_HP_PW_BUFFER	equ   3Fh	; Password buffer in 8042 RAM
KC_VOL_COMMAND		equ   80h	; The volume command
KC_DEFAULT_CLICK_VOL	equ    08h	; Default volume is medium
DEFAULT_RATE_DELAY	equ    00101011b; default rate and delay when HP CMOS
					; is bad - 500ms delay & 10.9 cps rate
KC_BAST_SPEED_HIGH	equ   90h	; Bastille only: Switch to high speed
KC_BAST_SPEED_LOW	equ   91h	; Bastille only: Switch to low speed
KC_LOAD_ADMIN		equ   92h	; Quasar/Pulsar: Load admin password byte.
I8042_HP_PW_LOCK_CMD	equ   92h	; Synonym for the above
KC_TEST_ADMIN		equ   93h	; Quasar/Pulsar: Test admin password.
I8042_HP_PW_UNLOCK_CMD	equ   93h	; Synonym for the above
I8042_DELAY_CMD 	equ   94h	; Delay the 8042
KC_GET_VERSION		equ   0FEH	; Return the 3 byte HP ID.  Example:
					; for rev B.12.34 the 3 data bytes
					; returned would be 42H, 12H, 34H.

I8042_READ_TEST_INPUT	equ  0E0h	; Read Test inputs
					;  bit 0 = Keyboard clock input
					;  bit 1 = Keyboard data input

I8042_SYSTEM_RESET	equ  0FEh	; 8042 system reset command
I8042_NULL_CMD		equ  0FFh	; 8042 null command

;=============================================================================;
;			      Keyboard Commands 			      ;
;=============================================================================;

; Acknowledgements sent by the keyboard after receiving any byte
KBD_REP_OVERRUN 	equ   000H	; 16 bytes buffer overrun
KBD_ACK 		equ   0FAH	; Acknowlege commands except EE & FE.
I8042_KBD_DIAG_FAIL	equ   0FDH	; means keyboard failed selftest

I8041_GET_KYB_ID_CMD	equ  0D2h	; Get old vectra keyboards ID

KBD_SET_LEDS		equ  0EDH	; Set keyboard indicator LEDs
KBD_LED_CAPS_LOCK	equ   00000100b
KBD_LED_NUM_LOCK	equ   00000010b
KBD_LED_SCROLL_LOCK	equ   00000001b

KBD_ECHO		equ  0EEH	; Echo command. Returns itself.

KBD_NOP 		equ  0EFH	; No op

KBD_SET_SCANCODE_SET	equ  0F0H	; Set alternate scancodes sets 1/2/3

KBD_ID_READ		equ  0F2H	; Read enhanced keyboard ID (2 bytes)

KBD_SET_TYPEMATIC	equ  0F3H	; Set typematic rate/delay

KBD_ENABLE		equ  0F4H
KBD_DISABLE		equ  0F5H
KBD_SET_DEFAULTS	equ  0F6H

I8042_RESEND_CMD	equ  0FEH	; keyboard resend command

I8042_KBD_TEST_CMD	equ  0FFH	;cmd to invoke keyboard selftest
I8042_KBD_TEST_ACK	equ   0FAH	 ;ACK from the keybaord selftest cmd
I8042_KBD_TEST_OK	equ   0AAH	 ;means keyboard pass selftest
I8042_KBD_TEST_FAIL	equ   0FCH	 ;means keyboard failed selftest

I8042_INIT_KBD_TEST_CMD equ   01111101b ;Init cmd byte for keyboard test ~~jwy 8/16/89
					;   bit 7: Reserved = 0
					;   bit 6: PC Compatibility mode = 1
					;   bit 5: disable mouse = 1
					;   bit 4: disable kbd = 1
					;   bit 3: Override inhibit switch = 1
					;   bit 2: System flag = 1
					;   bit 1: enable mouse int = 0
					;   bit 0: enable kbd int = 1

;=============================================================================;
;				Mouse Commands				      ;
;=============================================================================;

AUX_ACK 		equ   0FAh	; Aux Response code acknowledging receipt.
AUX_ERROR		equ   0FCh	; Aux Response code indicating error.
AUX_RESEND		equ   0FEh	; Aux Command/response code to resend data.

AUX_RESET_SCALING	equ  0E6h	; Reset scaling to 1:1.
AUX_SET_SCALING 	equ  0E7h	; Set scaling to 2:1.

AUX_SET_RESOLUTION	equ  0E8h	; Set resolution to next byte.
					; 0=1/mm; 1=2/mm; 2=4/mm; 3=8/mm
MAX_COUNTS_PER_MM	equ   3 	; 8 counts per millimeter.

AUX_STATUS_REQUEST	equ  0E9h	; Aux Command to issue status report.
					;  byte 3 = Sampling rate
					;  byte 2 = resolution setting
					;  byte 1 = flags
					;   bit 7 : reserved
					;   bit 6 : 0=stream mode; 1=remote mode
					;   bit 5 : 0=disabled; 1=enabled
					;   bit 4 : 0=scaling 1:1; 1=scaling 2:1
					;   bit 3 : reserved
					;   bit 2 : 1=left button pressed
					;   bit 1 : reserved
					;   bit 0 : 1=right button pressed

AUX_STREAM_MODE 	equ  0EAH	; Mouse sends data after any movement
AUX_READ_DATA		equ  0EBH	; Read data packet
AUX_REMOTE_MODE 	equ  0F0H	; Send data only when PC requests it

AUX_RESET_WRAP		equ  0ECh	; Reset wrap mode
AUX_SET_WRAP		equ  0EEh	; Set wrap mode ==> echo all but EC & FF

AUX_READ_DEVICE_TYPE	equ  0F2h	; Aux Command to read aux device type.

AUX_SET_SAMPLE_RATE	equ  0F3h	; Aux Command to set sample rate to next
					;  byte. Unit: samples/second.

AUX_XMIT_ENABLE 	equ  0F4h	; Aux Command to enable device transmissions.
AUX_XMIT_DISABLE	equ  0F5h	; Aux Command to disable device transmissions.
AUX_SET_DEFAULTS	equ  0F6H

AUX_RESET		equ  0FFh	; Aux Command to do reset & self-test.
MAX_PACKET_BYTES	equ   8 	; Maximum number of bytes in device packet.

MOUSE_ID		equ   0 	; Mouse ID byte

;=============================================================================;
;				  Scancodes				      ;
;=============================================================================;

; Make scan codes. Add 80H to get the break scan code.

SC_ENTER_MKE		equ	01Ch	; Carriage return
SC_BKSPC_MKE		equ	00EH	; Backspace
SC_ESC_MKE		equ	001H	; Escape
SC_SLASH_MKE		equ	035H	; Both slash keys
SC_BSLASH_MKE		equ	035h	; Backslash
SC_N_STAR_MKE		equ	037h	; Numeric keypad star
SC_N_MINUS_MKE		equ	04ah	; Numeric keypad minus
SC_N_PLUS_MKE		equ	04eh	; Numeric keypad plus

.LIST
