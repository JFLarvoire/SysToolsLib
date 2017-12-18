/*****************************************************************************\
*									      *
*   File name:	    lodos.h						      *
*									      *
*   Description:    LODOS.LIB global definitions			      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1995-08-25 JFL Created this file					      *
*    1995-11-28 JFL Moved functions definitions from MSDOS.H back into this   *
*		     file, with a conditional compilation to prevent an error *
*		     in case MSDOS.H is not included first.		      *
*    2015-10-27 JFL Added the generation of a library search record.	      *
*    2017-04-04 JFL Redefine stdin, stdout, stderr to be FILE *. Avoids warng.*
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef __LODOS_H__   // Prevent multiple inclusions
#define __LODOS_H__

/* Force linking with the lodos.lib library */
#define _LODOS_LIB "lodos.lib"
#pragma message("Adding pragma comment(lib, \"" _LODOS_LIB "\")")
#pragma comment(lib, _LODOS_LIB)

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/

#define FALSE 0
#define TRUE 1

typedef unsigned char	BYTE;	// 8 bits unsigned character
typedef unsigned short  WORD;   // 16-bit unsigne dvalue
typedef unsigned long   DWORD;  // 32-bit unsigned integer

/*****************************************************************************/
/*									     */
/*****************************************************************************/

#define FILENAME_MAX 64 	// Maximum length of a DOS pathname

/*****************************************************************************/
/*									     */
/*****************************************************************************/

/* Function prototypes */

// abs2phys.c
// Defined in msdos.h to avoid having to include msdos.h every time

// absdiskr.asm
extern int _cdecl AbsDiskRead(int iDrive, DWORD dwFirstSeg, WORD wNumSeg,
			      void far *lpBuf);

// absdiskw.asm
extern int _cdecl AbsDiskWrite(int iDrive, DWORD dwFirstSeg, WORD wNumSeg,
			       void far *lpBuf);

// bdos.asm
extern int _fastcall _bdos(int iFunc, WORD wDX, BYTE bAL);
#define _dos_resetdrives() _bdos(0x0D, 0, 0)

// cacheabs.c
extern int iCachedDrive;		/* -1=Invalid; 0=A, 1=B, 2=C, etc... */
#define NO_SECTOR 0xFFFFFFFFL		// Invalid sector number
extern DWORD dwCachedSector;		/* Sector number. NO_SECTOR=Invalid */
extern int iCacheDirty; 		/* If TRUE, needs to be flushed */
extern char cSectorCache[1024]; 	/* The cached sector */
extern int _cdecl CachedAbsDiskRead(int iDrive, DWORD dwSector, WORD wOffset,
				    WORD wLength, void *pBuf);
extern int _cdecl CachedAbsDiskWrite(int iDrive, DWORD dwSector, WORD wOffset,
				     WORD wLength, void *pBuf);
extern int _cdecl CachedAbsDiskFlush(void);

// cgroup.c
extern void *CGroupOffset(void *pCode);

// clus2abs.c
// Defined in msdos.h to avoid having to include msdos.h every time

// critsect.asm
// Prevent VMM from switching VMs
extern void _fastcall BeginCriticalSection(void);
// Reallow VMM to switch VMs
extern void _fastcall EndCriticalSection(void);

// dosclose.asm
extern int _fastcall _dos_close(int iRetCode);

// doscomit.asm
extern int _cdecl _dos_commit(int hf);

// dosexec.c
extern int _cdecl _dos_exec(char far *pszProgram, char far *pszArguments);

// dosexit.asm
extern void _fastcall _dos_exit(int iRetCode);

// dosgetft.asm
extern WORD _cdecl _dos_getftime(int hf, WORD *pwDate, WORD *pwTime);

// dosopen.asm
extern int _cdecl _dos_open(char *pszName, WORD wMode, int *phFile);
#define _O_RDONLY	0x0000	/* open for reading only */
#define _O_WRONLY	0x0001	/* open for writing only */
#define _O_RDWR 	0x0002	/* open for reading and writing */
#define _O_SHCOMP	0x0000	/* Share compatibility */
#define _O_SHDENYRW	0x0010	/* Share deny read write */
#define _O_SHDENYW	0x0020	/* Share deny write */
#define _O_SHDENYR	0x0030	/* Share deny read */
#define _O_SHDENYC	0x0040	/* Share deny compatibility accesses */
#define _O_NOINHERIT	0x0080	/* child process doesn't inherit file */

// dosread.asm
extern int _cdecl _dos_read(int hFile, void far *lpBuf, WORD wCount,
				    WORD *pwNumRead);

// dossetat.asm
extern int _fastcall _dos_setfileattr(char *pszPathname, WORD wAttrib);
#define _A_NORMAL   0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY   0x01    /* Read only file */
#define _A_HIDDEN   0x02    /* Hidden file */
#define _A_SYSTEM   0x04    /* System file */
#define _A_VOLID    0x08    /* Volume ID file */
#define _A_SUBDIR   0x10    /* Subdirectory */
#define _A_ARCH     0x20    /* Archive file */
#define _A_LFN	    (_A_HIDDEN | _A_SYSTEM | _A_VOLID)	/* Long file name */

// dossetft.asm
extern WORD _cdecl _dos_setftime(int hf, WORD wDate, WORD wTime);

// dosver.c
extern WORD _fastcall _dos_version(void);

// doswrite.asm
extern int _cdecl _dos_write(int hFile, void far *lpBuf, WORD wCount,
				    WORD *pwNumWritten);

// extdopen.asm
extern int _cdecl ExtendedOpen(char *pszName, WORD wMode, WORD wAttrib,
				WORD wAction, WORD *pwHandle);

// fgetenv.c
extern char far *_fgetenv(char *pszName);

// find_tsr.asm
// Locate a TSR using the standard int2F method
extern int _fastcall find_tsr(char *id_string);

// freemem.asm
// Free a block of DOS memory
extern int _fastcall _dos_freemem(WORD wSeg);

// getcwd.c
extern char *_getcwd(char *pszBuf, int iLen);
#define getcwd _getcwd	// Unix-compatible alias

// getdate.asm
#ifndef _DATETIME_T_DEFINED
// Note: The MSVC version uses a pack(2) directive. Let's try without it.
typedef struct _dosdate_t {
    BYTE day;		/* 1-31 */
    BYTE month;		/* 1-12 */
    WORD year;		/* 1980-2099 */
    BYTE dayofweek;	/* 0-6, 0=Sunday */
    } _dosdate_t;
#define dosdate_t _dosdate_t

// Note: Fields reordered compared to MSVC version.
typedef struct _dostime_t {
    BYTE hsecond;	/* 0-99 */
    BYTE second;	/* 0-59 */
    BYTE minute;	/* 0-59 */
    BYTE hour;		/* 0-23 */
    } _dostime_t;
#define dostime_t _dostime_t

#define _DATETIME_T_DEFINED
#endif 

extern void _fastcall _dos_getdate(_dosdate_t *pDate);

// getdevpa.c
// Defined in msdos.h to avoid having to include msdos.h every time

// getenv.c
extern char *getenv(char *pszName);

// getpsp.c
#ifdef MINICOMS     // No need to use a function within a minicom program.
#define GetPsp() _psp
#else
WORD GetPsp(void);  // Define function anyway for normal DOS programs.
#endif

// gettime.asm
extern void _fastcall _dos_gettime(_dostime_t *pTime);

// getvect.asm
// Get the interrupt vector
extern DWORD _fastcall _dos_getvect(int iIntNumber);

// getvmid.asm
extern int _cdecl GetVMID(void);

// getvmmv.asm
extern int _cdecl GetVmmVersion(void);

// getvxdep.asm
extern void far * _fastcall GetVxdEntryPoint(WORD wID);

//ioctldr.c
extern int _cdecl IoctlDiskRead(WORD iDrive, WORD iCyl, WORD iHead,
			 WORD iSect, WORD n, void far *lpcBuffer);

// lockvol.c
int LockLogicalVolume(int iDrive, WORD wLockLevel, WORD wPermissions);
int UnlockLogicalVolume(int iDrive);

// lseek.asm
extern long _cdecl lseek(int hFile, DWORD dwOffset, WORD wOrigin);

// remove.asm
int _fastcall remove(char *pszPathname);

// resetdrv.asm
int _fastcall ResetDrive(int iDrive, int iFlushFlag);

// setblock.asm
// Change a memory block size
extern int _fastcall _dos_setblock(WORD wLen, WORD wSeg, WORD *pMax);

// setdevpa.c
// Defined in msdos.h to avoid having to include msdos.h every time

// setmenv.c
int SetMasterEnv(char *pszName, char *pszValue);

// setvect.asm
// Set the int. vect.
extern void _fastcall _dos_setvect(int iIntNumber, DWORD dwHandler);

// system.c
int system(char *pszCommand);

// truename.asm
extern int _fastcall TrueName(char *pszDest, char *pszSource);

// tsr.asm
// Terminate and stay resident
extern void _fastcall tsr(int iRetCode, WORD wBufSize);

// yield.asm
extern void _cdecl ReleaseTimeSlice(void); // Release current ring 3 time slice

/*****************************************************************************/

/* Standard C File I/O */

#ifndef __CLIBDEF_H__	// Prevent multiple inclusions
typedef unsigned int size_t;
#endif

#define FILE void		// No file structure. A FILE * is a void *.

#define _fileno(hf) ((int)hf)	// In lodoslib, the FILE * is the DOS handle.
#define fileno _fileno		// Compatibility with old name

#define stdin  ((FILE *)0)	// DOS handle for standard input
#define stdout ((FILE *)1)	// DOS handle for standard output
#define stderr ((FILE *)2)	// DOS handle for error output

FILE *fopen(char *pszName, char *pszMode);
#define fclose(hf) _dos_close(fileno(hf))
size_t fread(void *pBuf, size_t nBytes, size_t nCount, FILE *hf);
size_t fwrite(void *pBuf, size_t nBytes, size_t nCount, FILE *hf);
int fseek(FILE *hf, long offset, int origin);
long filelength(int hf);
#define _filelength filelength
// int fflush(FILE *hf)
#define fflush(hf) _dos_commit(fileno(hf))

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0
int fseek(FILE *hf, long offset, int origin);
// void rewind(FILE *hf);
#define rewind(hf) fseek(hf, 0, SEEK_SET)

#define EOF -1

// long ftell(FILE *hf);
#define ftell(hf) lseek(fileno(hf), 0, SEEK_CUR)
int fgetc(FILE *hf);
char *fgets(char *pszLine, int iSize, FILE *hf);
int fputs(char *pszLine, FILE *hf);

/*****************************************************************************/

#ifdef __MSDOS_H__ // Required for DEVICEPARAMS definition

// abs2phys.c
void _cdecl Abs2PhysSector(DEVICEPARAMS *pdp, DWORD dwLogSect,
                   WORD *pwCyl, WORD *pwHead, WORD *pwSect);

// clus2abs.c
long Cluster2Sector(DEVICEPARAMS *pdp, WORD wCluster);

// getdevpa.c
extern int _cdecl GetDeviceParams(int drive, DEVICEPARAMS *pdp);

// setdevpa.c
extern int _cdecl SetDeviceParams(int drive, DEVICEPARAMS *pdp);

#endif

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // end if __LODOS_H__ included
