/*****************************************************************************\
*									      *
*   File name:	    UTILDEF.H						      *
*									      *
*   Description:    BIOS.LIB definitions not part of the standard C library.  *
*									      *
*   Notes:	    Not all the routines defined below have been ported to    *
*		     BIOS.LIB. To be done!				      *
*									      *
*		    The routines defined in assembly language modules must    *
*		     have either the _cdecl or the _fastcall keywords to show *
*		     what parameter passing convention has been implemented.  *
*									      *
*		    The routines defined in C language modules must have the  *
*		     _cdecl keyword when they have a variable number of       *
*		     arguments. Else the keyword must not be specified. The   *
*		     current setting will be used: _cdecl by default, or      *
*		     _fastcall if the /Gr switch is passed to the C compiler. *
*									      *
*   History:								      *
*    1990-12-07 JFL Created this file.					      *
*    1994-03-10 JFL Changed standard types to Windows convention.	      *
*		    Added mechanism against multiple inclusions.	      *
*		    C++ proof.						      *
*    1994-05-10 JFL Added prototypes for new routines find_tsr and	      *
*		     wait_refresh.					      *
*		    Renamed routine _pw_load_8042(void) 		      *
*				 as download_user_pw(char *).		      *
*    1995-02-17 JFL Fixed bug in UINT definition.			      *
*		    Added Everest2 and Cherokee IDs.			      *
*		    Renamed Psp as _psp to be more MSVC compatible	      *
*    1995-04-07 JFL Added definitions for BREAKARG.C.			      *
*    1995-08-25 JFL Removed definitions of find_tsr().			      *
*		    Added macros BYTE?() and WORD?().			      *
*		    Added definitions for DISKREAD.C.			      *
*    1995-08-28 JFL Added definitions for swapb and swapw		      *
*    1995-09-18 JFL Added definitions for DISKWRIT.C.			      *
*    1995-10-17 JFL Added definitions for BEEP.ASM.			      *
*    1995-12-19 JFL Added missing definitions for the 8042, keyboard and      *
*		      mouse; disable_kbd_n_mouse; restore_kbd_n_mouse.	      *
*    1997-05-21 JFL Added the resident qualifier.			      *
*    1999-08-05 JFL Added PnP BIOS definitions. 			      *
*    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
*    2016-04-24 JFL Added defs in getticks.asm, gettime.asm and getdate.asm.  *
*    2016-04-25 JFL Added comment lib bios.h to tell the linker to use it.    *
*									      *
*      (c) Copyright 1990-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef __UTILDEF_H__
#define __UTILDEF_H__

/* Force linking with the bios.lib library */
#ifndef BIOS_LIB
#define BIOS_LIB "bios.lib"
#pragma message("Adding pragma comment(lib, \"" BIOS_LIB "\")")
#pragma comment(lib, BIOS_LIB)
#endif /* defined(BIOS_LIB) */

#ifdef __cplusplus
extern "C"
{
#endif

/* Define x86 register types */
#ifndef DWORD_DEFINED
#define DWORD_DEFINED

typedef unsigned char   BYTE;   /*  8-bits unsigned integer */
typedef unsigned short	WORD;	/* 16-bits unsigned integer */
typedef unsigned long	DWORD;	/* 32-bits unsigned integer */

/* Define the FAR qualifier for DOS, but not for Windows */
#ifdef _H2INC
  #define _MSDOS /* The h2inc.exe tool is for generating DOS code */
#endif
#ifndef FAR
  #ifdef _MSDOS
    #define FAR far
  #endif /* _MSDOS */
  #ifdef _WIN32
    #define FAR
  #endif /* _WIN32 */
#endif /* FAR */

/* Common far pointers based on these types */
typedef void FAR *	LPVOID;
typedef BYTE FAR *	LPBYTE;
typedef WORD FAR *	LPWORD;
typedef DWORD FAR *	LPDWORD;

/* Manipulate bytes, words and dwords. Can be used both as rvalue and as lvalue. */
#define DWORD0(qw) (((DWORD *)(&(qw)))[0])
#define DWORD1(qw) (((DWORD *)(&(qw)))[1])

#define WORD0(qw) (((WORD *)(&(qw)))[0])
#define WORD1(qw) (((WORD *)(&(qw)))[1])
#define WORD2(qw) (((WORD *)(&(qw)))[2])
#define WORD3(qw) (((WORD *)(&(qw)))[3])

#define BYTE0(qw) (((BYTE *)(&(qw)))[0])
#define BYTE1(qw) (((BYTE *)(&(qw)))[1])
#define BYTE2(qw) (((BYTE *)(&(qw)))[2])
#define BYTE3(qw) (((BYTE *)(&(qw)))[3])
#define BYTE4(qw) (((BYTE *)(&(qw)))[4])
#define BYTE5(qw) (((BYTE *)(&(qw)))[5])
#define BYTE6(qw) (((BYTE *)(&(qw)))[6])
#define BYTE7(qw) (((BYTE *)(&(qw)))[7])

/* Reference a BYTE, WORD, DWORD, or QWORD, at offset N in a structure */
#define BYTE_AT(p, n) (*(BYTE *)((BYTE *)(p)+(int)(n)))
#define WORD_AT(p, n) (*(WORD *)((BYTE *)(p)+(int)(n)))
#define DWORD_AT(p, n) (*(DWORD *)((BYTE *)(p)+(int)(n)))

#endif /* defined(DWORD_DEFINED) */

/* Redefine a few MS-Windows standard types */
#define VOID void

typedef char far *	LPCHAR;

typedef unsigned int	UINT;	/* Unsigned value (size dependent on OS) */
typedef int             BOOL;   /* Boolean value */

typedef unsigned long	ULONG;	/* 32-bit unsigned integer */
typedef   signed long   LONG;   /* 32-bit signed integer */

typedef char *          PSTR;   /* Current model pointer to a character string */
typedef const char *    PCSTR;  /* Current model pointer to a constant "" "" */
typedef char near *     NPSTR;  /* 16-bit pointer to a character string */
typedef char far *      LPSTR;  /* 32-bit pointer to a character string */
typedef const char far *LPCSTR; /* 32-bit pointer to a constant character string */

/* Types from RomSetup's STDTYP.H that have no equivalent in Windows. */
typedef   signed char   tiny;   /* 8 bits signed integer. From STDTYP.H. */

/* Useful unit conversion macros */
#define KB2BYTE(kb) ((kb) << 10)	/* Convert Kilo-bytes to bytes */
#define BYTE2KB(b)  ((b)  >> 10)	/* Convert bytes to Kilo-bytes */
#define KB2PARA(kb) ((kb) << 6)		/* Convert Kilo-bytes to paragraphs */
#define PARA2KB(p)  ((p)  >> 6)		/* Convert paragraphs to Kilo-bytes */
#define BYTE2PARA(b) ((b) >> 4)		/* Convert bytes to paragraphs */
#define PARA2BYTE(p) ((p) << 4)		/* Convert paragraphs to bytes */


/* Qualifier for resident objects */
#define resident __based(__segname("RESID"))

/* RESID segment information */
extern char Beg_of_RSEG[];	/* Label at the beginning of the resident segment */
extern char End_of_RSEG[];	/* Label at the end of the resident segment */
#define GetResidSegmentSize() (End_of_RSEG - Beg_of_RSEG)

/* RESID segment relocation management */
DWORD __cdecl RunRelocated(void *pProc, WORD wSeg);
WORD FixRelocationParallax(WORD wReloc);


/* External references / function prototypes */

/* In startup.asm */

#define ARGLINESIZE 127

extern unsigned short _psp;         /* Not defined for device drivers */
/* ~~jfl 2000/09/28 Using unsigned short instead of WORD for _psp,
		    to allow to define it here and in clibdef.h. */
extern WORD EndOfAllocMem;          /* Not defined for device drivers */
extern WORD SegEnv;                 /* Not defined for device drivers */
extern BYTE ArgLineSize;            /* Not defined for device drivers */
extern char ArgLine[ARGLINESIZE];   /* Not defined for device drivers */

/* extern void far * _fastcall MakeFP(WORD ofs, WORD seg); */
/* More typical argument ordering for far pointer building */
/* #define MAKEFP(seg, ofs) MakeFP((WORD)(ofs), (WORD)(seg)) */
/* ~~jfl 2001/08/03 Redefined MAKEFP as a pure macro. */
/* Recent versions of the optimizing C compiler generate optimal code for that. */
#define MAKEFP(seg, off) ((void far *)(((DWORD)(WORD)(seg) << 16) | (WORD)(off)))

/* In breakarg.c */

extern char ArgLineCopy[ARGLINESIZE];
extern int BreakArgLine(char far *fpParms, char *pszArg[], int iMaxArgs);


/* CHS Sector index origin used in all BIOS access routines */
#define ISECT0 0	/* Contrary to standards, we index BIOS sectors from #0 */

/* In diskread.c */
extern int _cdecl BiosDiskRead(WORD iDrive, WORD iCyl, WORD iHead,
			       WORD iSect, WORD n, char far *lpcBuffer);

/* In diskwrit.c */
extern int _cdecl BiosDiskWrite(WORD iDrive, WORD iCyl, WORD iHead,
				WORD iSect, WORD n, char far *lpcBuffer);


/* In isswitch.c */
extern int IsSwitch(char *pszArg);

/* In dumpbuf.c */
extern void DumpBuf(void far *lpBuf, WORD wStart, WORD wStop);

/* In sqrt.c */
int isqrt(unsigned int n);

/* In assembly language modules */

   /* Math */
extern WORD     _fastcall   reverse(WORD);
extern short    _fastcall   regle_de_trois(short, short, short);
extern long     _fastcall   mul16x16to32(short, short);
extern short    _fastcall   div32x16to16(long, short);
extern BYTE     _fastcall   bcdtoh(BYTE);
extern BYTE	_fastcall   htobcd(BYTE);
extern WORD	_fastcall   swapb(WORD);
extern DWORD	_fastcall   swapw(DWORD);

   /* System */
extern void	_fastcall   beep(short freq, short millisecs);
extern LPVOID	_fastcall   Canonicalize(DWORD lp);
#define canonicalize(p) Canonicalize((DWORD)(LPVOID)p)
extern void	_fastcall   _disable_ctlc(void);
extern WORD	_cdecl	    GetDS(void);
extern WORD	_cdecl	    GetCS(void);
extern WORD	_cdecl	    GetFlags(void);
#define CARRY_FLAG 0x0001
extern short	_cdecl	    GetPostErrorCodes(short far**);
extern void     _fastcall   _instl_cr_hdlr(void);
extern short    _fastcall   interrupts_off(void);
extern void     _fastcall   interrupts_back(short);
extern short    _fastcall   private_entry(short);
extern short    _fastcall   read_rtc_cmos(short);
extern void     _fastcall   reset(short);
extern void     _fastcall   _restr_cr_hdlr(void);
extern short    _fastcall   search_error_code(short);
extern void	_fastcall   wait_refresh(void);
extern void     _fastcall   write_rtc_cmos(short, short);

   /* Video */
extern void     _fastcall   gotoXY(short, short);
extern void     _fastcall   get_cursor_position(short *, short *);
extern void     _cdecl      putch_color(short, short);
extern void     _cdecl      scrolldown(short, short, short, short);
extern void     _cdecl      scrollup(short, short, short, short);
extern void     _fastcall   cursor_off(void);
extern void     _fastcall   cursor_on(void);
extern void     _fastcall   _drawframe(int);
extern void     _fastcall   set_blinking(int);
extern short    _fastcall   vid_ram_size(void);
extern int      _fastcall   get_active_page(void);
extern void     _fastcall   set_active_page(int);
extern int	_fastcall   set_video_mode(int);
extern int      _fastcall   get_frame(void);


   /* Keyboard */
extern short    _fastcall   get_scancode(void);
extern short    _fastcall   get_keycode(void);

   /* Keyboard controller */
extern short    _fastcall   read_8042(void);
extern short    _fastcall   write_8042(short, short);
extern void	_fastcall   download_user_pw(char *);

   /* Turbo quasar family specific */
extern short    _fastcall   read_eeprom(short, short, BYTE *);
extern short    _fastcall   write_eeprom(short, short, BYTE *);
extern short    _fastcall   get_reserved(void);
extern BOOL     _fastcall   is_page_mapped(short);
extern short    _cdecl      call_boot_rom(short, ...);/* max = 4 parameters */
extern void     _fastcall   get_string(char*, short, short, short);
extern short    _fastcall   bootrom_HP_entry(short);
extern short    _fastcall   check_no_ram(void);

/* In C language modules */

extern void cputs_color(short, const char *);
#define putstr_color cputs_color; /* For a long time I had an putstr routine, then I learned about cputs which did the same as my putstr */
extern void display_field(short, short, short, const char *, short);
extern short xlate_key(short);

/* In both timebios.asm and timedos.asm */

extern void     _fastcall   get_date(char *);
extern void     _fastcall   get_time(char *);
extern void     _fastcall   save_date(char *);
extern void     _fastcall   save_time(char *);
extern short    _fastcall   get_seconds(void);
extern void     _fastcall   set_codepage(int);

/* In getticks.asm */

extern DWORD	_fastcall   _bios_getticks(void);	/* Number of ticks (at 18.2Hz) since midnight */

/* In gettime.asm and getdate.asm */

#pragma pack(1)

struct _biosdate_t {
    unsigned char day;		/* 1-31 */
    unsigned char month;	/* 1-12 */
    unsigned short int year;	/* 1980-2099 */
    };
extern int	_fastcall   _bios_getdate(struct _biosdate_t *);

struct _biostime_t {
    unsigned char dst;		/* 0=DST off; 1=DST on */
    unsigned char second;	/* 0-59 */
    unsigned char minute;	/* 0-59 */
    unsigned char hour;		/* 0-23 */
    };
extern int	_fastcall   _bios_gettime(struct _biostime_t *);

#pragma pack()

/* in aethnet.asm */
extern short    _fastcall   AM21xxGetMAC(short *);

/* in atkring.asm */
extern short    _fastcall   TRGetCardConfiguration(short *);


/* Miscellanneous equates */

/* Equates for color attributes */

#define FOREG_BLACK         0x00
#define FOREG_DARK_BLUE     0x01
#define FOREG_GREEN         0x02
#define FOREG_CYAN          0x03
#define FOREG_RED           0x04
#define FOREG_MAGENTA       0x05
#define FOREG_BROWN         0x06
#define FOREG_WHITE         0x07
#define FOREG_GRAY          0x08
#define FOREG_BLUE          0x09
#define FOREG_LIGHT_GREEN   0x0A
#define FOREG_LIGHT_BLUE    0x0B
#define FOREG_LIGHT_RED     0x0C
#define FOREG_PINK          0x0D
#define FOREG_YELLOW        0x0E
#define FOREG_INTENSE_WHITE 0x0F

#define BACKG_BLACK         0x00
#define BACKG_DARK_BLUE     0x10
#define BACKG_GREEN         0x20
#define BACKG_CYAN          0x30
#define BACKG_RED           0x40
#define BACKG_MAGENTA       0x50
#define BACKG_BROWN         0x60
#define BACKG_WHITE         0x70

#define FLASH 0x80

/* Scan codes */

#define KEY_F1              0x3B00      /* F1 key code (scancode * 0x100) */
#define KEY_F2              0x3C00
#define KEY_F3              0x3D00
#define KEY_F4              0x3E00
#define KEY_F5              0x3F00
#define KEY_F6              0x4000
#define KEY_F7              0x4100
#define KEY_F8              0x4200
#define KEY_F9              0x4300
#define KEY_F10             0x4400
#define KEY_F11             0x8500
#define KEY_F12             0x8600
#define KEY_HOME            0x4700
#define KEY_UP              0x4800
#define KEY_PGUP            0x4900
#define KEY_LEFT            0x4B00
#define KEY_CENTER          0x4C00
#define KEY_RIGHT           0x4D00
#define KEY_END             0x4F00
#define KEY_DOWN            0x5000
#define KEY_PGDOWN          0x5100
#define KEY_INS             0x5200
#define KEY_DEL             0x5300
#define KEY_ESC             0x001B
#define KEY_ENTER           0x000D
#define KEY_TAB             0x0009
#define KEY_BACKTAB         0x0F00
#define KEY_BACKSPACE       0x0008

/* BIOS return codes */

#define RS_SUCCESSFUL       0
#define RS_FAIL             -2
#define RS_UNSUPPORTED      2
#define SYSTEM_LOCKED       1

/* BIOS absolute addresses */

/*
Bits 7:5 at F000:00FA     Bits 4:0 at F000:00FA
or   3:0 at F000:00F3(*)  or   byte   F000:00F2(#)

   0 -> 80286              0 -> A, A+   orig vectra
   0 -> 80286              1 -> ES/12   A++
   3 -> 80386              2 -> RS/20   CARRERA
   2 -> 8086               3 -> PORT CS NEWPORT
   0 -> 80286              4 -> ES      V8
   2 -> 8086               5 -> CS      NOVA
   3 -> 80386              6 -> RS/16   CORVAIR
   3 -> 80386              7 -> QS/16   ALPINE
   3 -> 80386              8 -> QS/20   ALPINE/20
   3 -> 80386              9 -> RS/20C  CABRIOLET
   3 -> 80386              A -> RS/25C  TARGA
   0 -> 80286              B -> LS/286  BLACK SHEEP
   4 -> 80386SX            C -> QS/16S  JETTA
   3 -> 80386              D -> 386/25  MARCO POLO
   5 -> 80486              E -> 486     TOMCAT
   0 -> 80286              F -> 286     BASTILLE
   5 -> 80486             10 -> 486     POLARIS
   3 -> 80386             11 -> 386/20  MARCO POLO 20
   4 -> 80386SX           12 -> 386/16N Quasar/16
   4 -> 80386SX           13 -> 386/20N Quasar/20
   6 -> 80486SX           14 -> 486S/20 Nike
   4 -> 80386SX           15 -> 386S/20 Phasor/20
   6 -> 80486SX           16 -> 486S/25 BRONCO
   5 -> 80486             17 -> 486/33  CHEROKEE
	P24		  18 -> P24/50	SIERRA
	P24		  19 -> P24/66	WRANGLER
	P24		  1A -> P24/66	BARRACUDA
   4 -> 80386SX           1B -> 386S/25N Turbo Quasar/25
   5 -> 80486DX           1C -> 486DX   PULSAR
   6 -> 80486SX           1C -> 486SX   PULSAR
   7 -> (*)9->P23T        1C -> 487     PULSAR
        (*)8->P24
        (*)A->80487SX
   4 -> 80386SX           1D -> 386S/25 Phasor/25
   4 -> 80386SX           1E -> 386S/33 Etherlite 386/33
   5 -> 80486DX        (#)20 -> 486DX   Etherlite 486
   6 -> 80486SX        (#)20 -> 486SX   Etherlite 486
   7 -> (*)9->P23T     (#)20 -> 487     Etherlite 486
        (*)8->P24
        (*)A->80487SX
(*)0C-> P24T           (#)21 -> NEPTUNE
(*)0B-> P5 (Pentium)   (#)22 -> EVEREST
(*)0B-> P5 (Pentium)   (#)4E -> EVEREST2
(*)0B-> P5 (Pentium)   (#)4F -> CHEROKEEG
*/

#define VECTRA_ID_BYTE1_PTR ((BYTE far *)0xF00000FAL)
#define VECTRA_ID_BYTE2_PTR ((BYTE far *)0xF00000F2L)

#define ID_VECTRA    0xFF     /* added to run the setup on all the old VECTRA
             family, doesn't corespond to a specific PC */

#define ID_ES12      (0x01 & 0x1F)
#define ID_RS20      (0x62 & 0x1F)
#define ID_ES8       (0x04 & 0x1F)
#define ID_RS16      (0x66 & 0x1F)
#define ID_QS16      (0x67 & 0x1F)
#define ID_QS20      (0x68 & 0x1F)
#define ID_RS20C     (0x69 & 0x1F)
#define ID_RS25C     (0x6A & 0x1F)
#define ID_QS16S     (0x8C & 0x1F)
#define ID_MPOLO     (0x6D & 0x1F)
#define ID_TOMCAT    (0xAE & 0x1F)
#define ID_BASTILLE  (0x0F & 0x1F)
#define ID_POLARIS   (0xB0 & 0x1F)
#define ID_MPOLO20   (0x71 & 0x1F)
#define ID_NIKE      (0xD4 & 0x1F)
#define ID_PHASOR20  (0x95 & 0x1F)
#define ID_BRONCO    (0xD6 & 0x1F)
#define ID_CHEROKEE  (0xB7 & 0x1F)
#define ID_SIERRA    (0xF8 & 0x1F)
#define ID_WRANGLER  (0xF9 & 0x1F)
#define ID_BARRACUDA (0x9A & 0x1F)
#define ID_TQUASAR16 (0x92 & 0x1F)
#define ID_TQUASAR20 (0x93 & 0x1F)
#define ID_TQUASAR25 (0x9B & 0x1F)
#define ID_PULSAR    (0x1C)       /* 1C = common value of the bits 0 to 4 of the
				     PULSAR family */
#define ID_PHASOR25  (0x9D & 0x1F)
#define ID_386ETHERL33_E (0x9E & 0x1F)  /* Etherlite 386/33 in Eclipse box */
#define ID_486ETHERL  0x20       /* Etherlite 486 in Eclipse or Corona box
                                    and one or two serial ports */
#define ID_NEPTUNE    0x21       /* NEPTUNE Eclipse or Corona box
                                    and one or two serial ports */
#define ID_EVEREST    0x22       /* EVEREST */
#define ID_EVEREST2   0x4E       /* EVEREST2 */
#define ID_CHEROKEEG  0x4F       /* CHEROKEEG*/

#define MICROPROC_MASK      0xE0
#define I80286              0x00
#define I8086               0x40
#define I80386              0x60
#define I80386SX            0x80
#define I80486DX            0xA0
#define I80486SX            0xC0
#define I80MOREF3           0xE0  /* Intel Overdrive/486DX2/487SX/... */

#define get_pc_id1() ((*VECTRA_ID_BYTE1_PTR) & (BYTE)0x1F)
#define get_pc_id2() (*VECTRA_ID_BYTE2_PTR)
/* we consider only the bits 0 to 4 of F000:FA or F000:F2, in which is
   the number of the machine */

#define BIOS_CAPABILITY_MARKER (*(BYTE far *)0xF00000FB)
#define BCM_XCACHE_MASK 0x04            /* 1 = External cache present */
#define BCM_MINIDIN_MASK 0x08           /* 1 = MiniDIN mouse; 0 = HIL mouse */
#define BCM_EISA_MASK 0x10              /* 1 = EISA bus */
#define BCM_NO_XBIOS_MASK 0x20          /* 1 = XBIOS not available */
#define BCM_EXTENSION_MASK 0x80         /* 1 = Extension at 00F0 valid */

#define HP_SECOND_CAPABILITY_MARKER  ((BYTE far *)0xF00000F0)
       /* b0=1 : support HP LAN boot rom
   	  b1=1 : no support of HP one byte cmd
   	  b2=1 : EMBEDDED LAN PRESENT
   	  b3=1 : Corona box : no FAN test
          b4=1 : Serial Port B presence
	  b5=1 : Infra-red presence
	  b6=1 : PCMCIA presence */
#define CORONA_BOX_MASK   0x08
#define SERIAL_PORTB_MASK 0x10
#define INFRA_RED_MASK	  0x20
#define PCMCIA_MASK	  0x40

/*===========================================================================*\
|			    8042 ports and data 			      |
\*===========================================================================*/

#define IO_8042_DATA	    0x60	/* 8042 data port (read/write) */

#define IO_8042_STATUS	    0x64	/* 8042 status port (read) */
#define I8042_PARITY_ERROR	0x80	/* Last byte had even parity - error */
#define I8042_REC_TIMEOUT	0x40	/* Transmission started, but not complete */
#define I8042_TRANS_TIMEOUT	0x20	/* Transmission from keyboard not completed */
#define I8042_KBD_INHIBIT	0x10	/* Keyboard NOT inhibited */
#define I8042_CMD_DATA		0x08	/* 1 = command byte; 0 = data byte */
#define I8042_SYSTEM_FLAG	0x04	/* System flag - set to 0 after power on reset */
#define I8042_IBF		0x02	/* Input buffer full flag */
#define I8042_OBF		0x01	/* Output buffer full flag */
#define AUX_OUT_BUFFER_FULL     0x20    /* For 8042 aux. output buffer full status. */

#define IO_8042_CMD	    0x64	/* 8042 command port (write) */
#define READ_CMD_BYTE           0x20    /* 8042 command to read command byte */
#define I8042_WRITE_CMD 	0x60	/* Write KBD controller command byte */
#define I8042_INIT_CMD		    0x65    /* Initial Command byte */
#define DIS_KBD_N_MOUSE             0x30    /* disable kbd and mouse */
#define TEST_MOUSE_IRQ              0x32    /* Idem, but allow mouse IRQs */
					    /*	 bit 7: Reserved = 0 */
					    /*	 bit 6: Keyboard translate mode = 1 */
					    /*	 bit 5: Disable AUX device = 1 */
					    /*	 bit 4: Disable keyboard = 1 */
					    /*	 bit 3: Override inhibit switch = 0 */
					    /*	 bit 2: System flag = 1 */
					    /*	 bit 1: Enable AUX interrupt = 1 */
					    /*	 bit 0: Enable OBF interrupt = 1 */
#define CHECK_PW_CMD		0xA4	/* 8042 command to test if there is a pwd */
#define LOAD_PW_CMD		0xA5	/* 8042 command to load password */
#define ENABLE_SECURITY 	0xA6	/* 8042 command to lock the keyboard */
#define AUX_DISABLE_DEVICE	0xA7	/* 8042 command to disable aux device */
#define AUX_ENABLE_DEVICE	0xA8	/* 8042 command to enable aux device */
#define AUX_INTERFACE_TEST	0xA9	/* 8042 command to test aux. device interface */
#define AUX_ST_COMPLETE 	    0xAA    /* Aux. reply to aux. reset command */

#define I8042_RESET_CMD 	0xAA	/* 8042 self-test/reset command */
#define I8042_RESET_OK		    0x55    /* Successful response to reset command */

#define I8042_INTRF_TEST_CMD	0xAB	/*KBD interface test command */
#define I8042_INTRF_OK		    0x00    /*Successful response */
#define I8042_INTRF_RESULT_MASK     0x0F    /*``JKL (1/31/87) extract kbd */
					/*intrf test error. */
/* #define I8042_KBD_DIAG_DUMP	0xAC	// Keyboard diagnostic dump (Unsupported) */
#define I8042_DISABLE_KBD_INTRF 0xAD	/* Cmd to disable kbd interface */
#define I8042_ENABLE_KBD_INTRF	0xAE	/* Cmd to enable kbd interface */
#define I8042_READ_INPUT	0xC0	/* Read 8042 jumper information */
#define INPUT_PORT_PWD_ENABLE	    0x80    /* 1 = Password enabled; 0 = disabled */
#define VID_MONO_DISPLAY	    0x40    /* 1 = Primary display attached to MDA */
#define INPUT_PORT_RESERVED	    0x20    /* Reserved (Manufacturing jumper not installed) */
#define ENABLE_SYS_BOARD_MEM	    0x10    /* 1 = 2nd 256K on system board enabled */
#define INPUT_PORT_KEY_CLICK	    0x08    /* Key click control */
#define I8042_KBD_FUSE		    0x04    /* 1 = Power sense line fuse OK */
#define INPUT_PORT_AUX_DATA	    0x02    /* Auxiliary input data line */
#define INPUT_PORT_KBD_DATA	    0x01    /* Keyboard input data line */

#define I8042_READ_OUTPUT	0xD0	/* read the 8042 output port */
#define I8042_WRITE_OUTPUT	0xD1	/* write the 8042 output port */
#define I8042_KBD_DATA		    0x80    /* keyboard data out line */
#define I8042_KBD_CLOCK 	    0x40    /* keyboard clock out line */
#define I8042_AUX_INT		    0x20    /* aux interrupt */
#define I8042_KBD_INT		    0x10    /* keyboard interrupt */
#define I8042_AUX_CLOCK 	    0x08    /* aux clock out line */
#define I8042_AUX_DATA		    0x04    /* aux data out line */
#define I8042_A20_GATE		    0x02    /* A20 gate status */
#define I8042_RESET_BIT 	    0x01    /* system reset */

#define WRITE_KDB_BUFFER	0xD2	/* Write keyboard output buffer */

#define WRITE_AUX_BUFFER	0xD3	/* Write auxiliary device output buffer */
#define AUX_WRITE_DEVICE	0xD4	/* 8042 command to write next byte to aux. device */

#define RST_A20_COMMAND 	0xDD	/* 1 byte command to reset GATE A20 */
#define SET_A20_COMMAND 	0xDF	/* 1 byte command to set GATE A20 */

#define KC_GET_READY		0xDE	/* 8042 command to get ready for HP private command */
#define I8042_HP_PW_BUFFER	    0x3F    /* Password buffer in 8042 RAM */
#define KC_VOL_COMMAND		    0x80    /* The volume command */
#define KC_DEFAULT_CLICK_VOL		0x08	/* Default volume is medium */
#define DEFAULT_RATE_DELAY		0x2D	/* default rate and delay when HP CMOS */
						/* is bad - 500ms delay & 10.9 cps rate */
#define KC_BAST_SPEED_HIGH	    0x90    /* Bastille only: Switch to high speed */
#define KC_BAST_SPEED_LOW	    0x91    /* Bastille only: Switch to low speed */
#define KC_LOAD_ADMIN		    0x92    /* Quasar/Pulsar: Load admin password byte. */
#define I8042_HP_PW_LOCK_CMD	    0x92    /* Synonym for the above */
#define KC_TEST_ADMIN		    0x93    /* Quasar/Pulsar: Test admin password. */
#define I8042_HP_PW_UNLOCK_CMD	    0x93    /* Synonym for the above */
#define I8042_DELAY_CMD 	    0x94    /* Delay the 8042 */
#define KC_GET_VERSION		    0xFE    /* Return the 3 byte HP ID.  Example: */
					    /* for rev B.12.34 the 3 data bytes */
					    /* returned would be 42H, 12H, 34H. */

#define I8042_READ_TEST_INPUT	0xE0	    /* Read Test inputs */
				    /*	bit 0 = Keyboard clock input */
				    /*	bit 1 = Keyboard data input */

#define I8042_SYSTEM_RESET	0xFE	    /* 8042 system reset command */
#define I8042_NULL_CMD		0xFF	    /* 8042 null command */

/*===========================================================================*\
|			      Keyboard Commands 			      |
\*===========================================================================*/

/* Acknowledgements sent by the keyboard after receiving any byte */
#define KBD_REP_OVERRUN 	    0x00    /* 16 bytes buffer overrun */
#define KBD_ACK 		    0xFA    /* Acknowlege commands except EE & FE. */
#define I8042_KBD_DIAG_FAIL	    0xFD    /* means keyboard failed selftest */

#define I8042_GET_KYB_ID_CMD	0xD2	/* Get old vectra keyboards ID */

#define KBD_SET_LEDS		0xED	/* Set keyboard indicator LEDs */
#define KBD_LED_CAPS_LOCK	    0x04
#define KBD_LED_NUM_LOCK	    0x02
#define KBD_LED_SCROLL_LOCK	    0x01

#define KBD_ECHO		0xEE	/* Echo command. Returns itself. */

#define KBD_NOP 		0xEF	/* No op */

#define KBD_SET_SCANCODE_SET	0xF0	/* Set alternate scancodes sets 1/2/3 */

#define KBD_ID_READ		0xF2	/* Read enhanced keyboard ID (2 bytes) */

#define KBD_SET_TYPEMATIC	0xF3	/* Set typematic rate/delay */

#define KBD_ENABLE		0xF4
#define KBD_DISABLE		0xF5
#define KBD_SET_DEFAULTS	0xF6

#define I8042_RESEND_CMD	0xFE	/* keyboard resend command */

#define I8042_KBD_TEST_CMD	0xFF	/* cmd to invoke keyboard selftest */
#define I8042_KBD_TEST_ACK	    0xFA    /* ACK from the keybaord selftest cmd */
#define I8042_KBD_TEST_OK	    0xAA    /* means keyboard pass selftest */
#define I8042_KBD_TEST_FAIL	    0xFC    /* means keyboard failed selftest */

#define I8042_INIT_KBD_TEST_CMD     0x7D    /* Init cmd byte for keyboard test ~~jwy 8/16/89 */
					    /*	bit 7: Reserved = 0 */
					    /*	bit 6: PC Compatibility mode = 1 */
					    /*	bit 5: disable mouse = 1 */
					    /*	bit 4: disable kbd = 1 */
					    /*	bit 3: Override inhibit switch = 1 */
					    /*	bit 2: System flag = 1 */
					    /*	bit 1: enable mouse int = 0 */
					    /*	bit 0: enable kbd int = 1 */

/*===========================================================================*\
|				Mouse Commands				      |
\*===========================================================================*/

#define AUX_ACK 		    0xFA   /* Aux Response code acknowledging receipt. */
#define AUX_ERROR		    0xFC   /* Aux Response code indicating error. */
#define AUX_RESEND		    0xFE   /* Aux Command/response code to resend data. */

#define AUX_RESET_SCALING	0xE6	   /* Reset scaling to 1:1. */
#define AUX_SET_SCALING 	0xE7	   /* Set scaling to 2:1. */

#define AUX_SET_RESOLUTION	0xE8	   /* Set resolution to next byte. */
					   /* 0=1/mm; 1=2/mm; 2=4/mm; 3=8/mm */
#define MAX_COUNTS_PER_MM	    3	   /* 8 counts per millimeter. */

#define AUX_STATUS_REQUEST	0xE9	   /* Aux Command to issue status report. */
					      /* byte 3 = Sampling rate */
					      /* byte 2 = resolution setting */
					      /* byte 1 = flags */
					      /*	bit 7 : reserved */
					      /*	bit 6 : 0=stream mode; 1=remote mode */
					      /*	bit 5 : 0=disabled; 1=enabled */
					      /*	bit 4 : 0=scaling 1:1; 1=scaling 2:1 */
					      /*	bit 3 : reserved */
					      /*	bit 2 : 1=left button pressed */
					      /*	bit 1 : reserved */
					      /*	bit 0 : 1=right button pressed */

#define AUX_STREAM_MODE 	0xEA	   /* Mouse sends data after any movement */
#define AUX_READ_DATA		0xEB	   /* Read data packet */
#define AUX_REMOTE_MODE 	0xF0	   /* Send data only when PC requests it */

#define AUX_RESET_WRAP		0xEC	   /* Reset wrap mode */
#define AUX_SET_WRAP		0xEE	   /* Set wrap mode ==> echo all but EC & FF */

#define AUX_READ_DEVICE_TYPE	0xF2	   /* Aux Command to read aux device type. */
#define MOUSE_ID		    0	   /* Mouse ID byte */

#define AUX_SET_SAMPLE_RATE	0xF3	   /* Aux Command to set sample rate to next */
					      /* byte. Unit: samples/second. */

#define AUX_XMIT_ENABLE 	0xF4	   /* Aux Command to enable device transmissions. */
#define AUX_XMIT_DISABLE	0xF5	   /* Aux Command to disable device transmissions. */
#define AUX_SET_DEFAULTS	0xF6

#define AUX_RESET		0xFF	   /* Aux Command to do reset & self-test. */
#define MAX_PACKET_BYTES	    8	   /* Maximum number of bytes in device packet. */

/*===========================================================================*\
|									      |
\*===========================================================================*/

WORD _fastcall disable_kbd_n_mouse(void);
WORD _fastcall restore_kbd_n_mouse(WORD wOldCmdByte);

/*****************************************************************************\
*			    PnP BIOS Definitions			      *
\*****************************************************************************/

#pragma pack(1)

/* PnP BIOS API */
typedef short (_cdecl far * LPPNPBIOS)(int iFunction, ...);

/* PnP BIOS header, located at a paragraph somewhere in segment F000. */
typedef struct
    {
    DWORD dwSignature;			/* Must be "$PnP". */
    BYTE bVersion;
    BYTE bHeaderLength;
    WORD wControl;
    BYTE bChecksum;
    LPVOID lpNotification;
    LPPNPBIOS lpRmEntry;
    WORD wPmEntryOffset;
    DWORD dwPmEntryBase;
    DWORD dwOemId;
    WORD wRmDS;
    DWORD dwPmDataBase;
    } PNPHEADER, far *LPPNPHEADER;

#pragma pack()

LPVOID FindHeader(DWORD dwExpected);	/* Locate a PnP BIOS-type header */

/* Various signatures used to identify the type of header */
#define $PnP 0x506E5024 /* "$PnP" Signature for PnP BIOS and SMBIOS 2.0+ API */
#define $DMI 0x494D4424 /* "$DMI" Signature for SMBIOS 1.0 tables */
#define _D2_ 0x524E5048 /* "_DMI20_NT_" Signature for HP DMI 2.0 32-bits-RAM tables */
#define _SM_ 0x5F4D535F /* "_SM_" Signature for SMBIOS 2.1+ 32-bits-RAM-style tables */
#define _SM3 0x334D535F /* "_SM3_" Signature for SMBIOS 3.0+ 64-bits-RAM-style tables */

#ifdef __cplusplus
}
#endif

#endif /* __UTILDEF_H__ */
