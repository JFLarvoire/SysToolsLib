/*****************************************************************************\
*                                                                             *
*   Filename:	    uuid.c						      *
*									      *
*   Description:    OS-independant UUID generation routines		      *
*                                                                             *
*   Notes:	    This code is designed to compile both as a 16-bits MS-DOS *
*		    program, and as a 32-bits WIN32 console application.      *
*									      *
*		    TO DO: Move routine get_system_time() into its own        *
*		    module, possibly changing it to Unix' gettimeofday().     *
*									      *
*   History:								      *
*    2000-09-21 JFL Created this file, based on genuuid.c.		      *
*    2001-01-15 JFL Changed data types to Windows-compatible BYTE/WORD/DWORD. *
*    2002-01-03 JFL Moved routine IsNullUuid() to new file uuidnull.c.	      *
*    2015-08-18 JFL Reference the iDebug variable only in _DEBUG builds.      *
*    2016-04-23 JFL Moved the MAC Address query routine to macaddr.c.         *
*    2016-04-24 JFL Added the BIOS version of get_system_time().	      *
*    2016-04-26 JFL Changed the unsigned64_t type to SysLib's QWORD.	      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "uuid.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#ifdef _DEBUG
extern int iDebug;	/* Defined in main module. If TRUE, display debug information */
#endif /* _DEBUG */

/*****************************************************************************\
*                                                                             *
*                               WIN32 Routines                                *
*                                                                             *
\*****************************************************************************/

#ifdef _WIN32

/* Defined in uuid.h as macros calling WIN32 API UUID management functions */

#endif /* _WIN32 */

/*****************************************************************************\
*                                                                             *
*                           BIOS and MSDOS Routines                           *
*                                                                             *
\*****************************************************************************/

#ifdef _MSDOS

#include "macaddr.h"

/*---------------------------------------------------------------------------*\
*									      *
|   Notes:	UUID generation code					      |
|									      |
|		Depends on:						      |
|		GetMacAddress()		Get the MAC Address of a network card.|
|		GetPcUuid()		Get the PC UUID			      |
|									      |
|   History:								      |
|    1997/09/02 JFL Modified the code from HP labs			      |
*                                                                             *
\*---------------------------------------------------------------------------*/

/*
**  Add two unsigned 64-bit long integers.
*/
#define ADD_64b_2_64b(A, B, sum) \
    { \
        if (!(((A)->dw0 & 0x80000000UL) ^ ((B)->dw0 & 0x80000000UL))) { \
            if (((A)->dw0&0x80000000UL)) { \
                (sum)->dw0 = (A)->dw0 + (B)->dw0; \
                (sum)->dw1 = (A)->dw1 + (B)->dw1 + 1; \
            } \
            else { \
                (sum)->dw0  = (A)->dw0 + (B)->dw0; \
                (sum)->dw1 = (A)->dw1 + (B)->dw1; \
            } \
        } \
        else { \
            (sum)->dw0 = (A)->dw0 + (B)->dw0; \
            (sum)->dw1 = (A)->dw1 + (B)->dw1; \
            if (!((sum)->dw0&0x80000000UL)) (sum)->dw1++; \
        } \
    }

/*
**  Add a 16-bit unsigned integer to a 64-bit unsigned integer.
*/
#define ADD_16b_2_64b(A, B, sum) \
    { \
        (sum)->dw1 = (B)->dw1; \
        if ((B)->dw0 & 0x80000000UL) { \
            (sum)->dw0 = (*A) + (B)->dw0; \
            if (!((sum)->dw0 & 0x80000000UL)) (sum)->dw1++; \
        } \
        else \
            (sum)->dw0 = (*A) + (B)->dw0; \
    }

/*
**  Global variables.
*/
static QWORD     time_last;
static WORD       clock_seq;

static void
mult32(DWORD u, DWORD v, QWORD *result)
{
    /* Following the notation in Knuth, Vol. 2. */
    DWORD uuid1, uuid2, v1, v2, temp;

    uuid1 = u >> 16;
    uuid2 = u & 0xFFFF;
    v1 = v >> 16;
    v2 = v & 0xFFFF;
    temp = uuid2 * v2;
    result->dw0 = temp & 0xFFFF;
    temp = uuid1 * v2 + (temp >> 16);
    result->dw1 = temp >> 16;
    temp = uuid2 * v1 + (temp & 0xFFFF);
    result->dw0 += (temp & 0xFFFF) << 16;
    result->dw1 += uuid1 * v1 + (temp >> 16);
}

#if !(defined(_BIOS) || defined(_LODOS)) /* This is truely for MS-DOS */

#include <sys/types.h>
#include <time.h>
#include <process.h>
#include <dos.h>

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    MilliSec						      |
|									      |
|   Description:    Convert a DOS time into a number of milliseconds since 0H |
|									      |
|   Parameters:     struct _dostime_t *pTime	    Where to store the time   |
|									      |
|   Returns:	    Number of milli-seconds				      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1996/12/13 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

long MilliSec(struct _dostime_t *pdt)
    {
    long l;

    l = (long)(pdt->hour);
    l *= 60;
    l += (long)(pdt->minute);
    l *= 60;
    l += (long)(pdt->second);
    l *= 100;
    l += (long)(pdt->hsecond);
    l *= 10;

    return l;
    }

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    get_system_time					      |
|									      |
|   Description:    Get the UUID date/time = # of µs since October 15, 1582   |
|									      |
|   Parameters:     QWORD *uuid_time	    Where to store the time	      |
|									      |
|   Returns:	    Number of micro-seconds since October 15, 1582	      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    1996/12/13 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

/* Number of days in the year, starting on march 1 */
		 /* Mar Avr May Jun Jul Aug Sep Oct Nov Dec Jan Feb */
		 /* 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29  */
int ytd[14] = {0, 0, 0, 31, 61, 92,122,153,184,214,245,275,306,337};/* 366 */

static void
get_system_time(QWORD *uuid_time)
{
    QWORD utc, usecs, os_basetime_diff;
    struct _dostime_t dt;
    struct _dosdate_t dd;
    long l;
    int iYear, iMonth, iDay;

    /* ~~jfl 1997/09/02 Use MS-DOS time and date functions to compute UUID time
    gettimeofday(&tp, (struct timezone *)0);
    mult32((long)tp.tv_sec, 10000000, &utc);
    mult32((long)tp.tv_usec, 10, &usecs);
    ADD_64b_2_64b(&usecs, &utc, &utc);
    */
    _dos_gettime(&dt);
    l = MilliSec(&dt);
#ifdef _DEBUG
    if (iDebug)
	{
	printf("%02d:%02d:%02d,%02d = %ld ms\n", dt.hour, dt.minute,
		dt.second, dt.hsecond, l);
	}
#endif /* _DEBUG */
    mult32(l/1000, 10000000, &utc);
    mult32(l%1000, 10000, &usecs);
    ADD_64b_2_64b(&usecs, &utc, &utc);

    _dos_getdate(&dd);
    iDay = dd.day - 1;		    /* 0-based index */
    iMonth = dd.month - 1;	    /* 0-based index */
    iYear = dd.year - 1980;	    /* Make sure computations don't overflow */
    if (iMonth < 2)		    /* January or february */
	{
	iMonth += 12;
	iYear -= 1;
	}
    /* Days since march 1st, 1980 */
    l = iDay + ytd[iMonth] + ((iYear/4)*1461) + ((iYear%4)*365);
#ifdef _DEBUG
    if (iDebug)
	{
	printf("%d/%02d/%02d = %ld days since 1980/03/01\n",
		dd.year, dd.month, dd.day, l);
	}
#endif /* _DEBUG */
    l *= 86400; /* Seconds since march 1st, 1980 */
    mult32(l, 10000000, &usecs);
    ADD_64b_2_64b(&usecs, &utc, &utc);

    /* Offset between UUID formatted times and DOS formatted times.
     * UUID UTC base time is October 15, 1582.
     * Unix base time is January 1, 1970. Delta = 0x01B21DD2 13814000
     * DOS base time is March 1, 1980.	  Delta = 0x000B64E6 FD600000 */
    os_basetime_diff.dw0 = 0x110E4000;
    os_basetime_diff.dw1 = 0x01BD82B9;
    ADD_64b_2_64b(&utc, &os_basetime_diff, uuid_time);
#ifdef _DEBUG
    if (iDebug)
	{
	printf("So now = 1582/10/15 + 0x%08lX %08lX 1/10th ns\n",
		uuid_time->dw1, uuid_time->dw0);
	}
#endif /* _DEBUG */
}

#else	/* defined(_BIOS) || defined(_LODOS) : This is actually for BIOS */

#include "utildef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    get_system_time					      |
|									      |
|   Description:    Get the UUID date/time = # of µs since October 15, 1582   |
|									      |
|   Parameters:     QWORD *uuid_time	    Where to store the time	      |
|									      |
|   Returns:	    Number of micro-seconds since October 15, 1582	      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|    2016-04-24 JFL Adapted the DOS version for BIOS.			      |
*									      *
\*---------------------------------------------------------------------------*/

/* Number of days in the year, starting on march 1 */
		 /* Mar Avr May Jun Jul Aug Sep Oct Nov Dec Jan Feb */
		 /* 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29  */
int ytd[14] = {0, 0, 0, 31, 61, 92,122,153,184,214,245,275,306,337};/* 366 */

static void
get_system_time(QWORD *uuid_time)
{
    QWORD utc, usecs, os_basetime_diff;
    struct _biosdate_t dd;
    DWORD dw;
    int iYear, iMonth, iDay;

    /* ~~jfl 1997/09/02 Use MS-DOS time and date functions to compute UUID time
    gettimeofday(&tp, (struct timezone *)0);
    mult32((long)tp.tv_sec, 10000000, &utc);
    mult32((long)tp.tv_usec, 10, &usecs);
    ADD_64b_2_64b(&usecs, &utc, &utc);
    */
    dw = _bios_getticks(); /* 65536 ticks/hour = 1572864/day ~= 18.2 Hz */
    /* The original IBM PC timer frequency was based on an NTSC signal that is
       supposed to beat at 64K ticks per hour.
       65536 / 3600s = 1,572,864 / day.
       But the PC generated that signal from its 4.77 MHz processor clock,
       and ended up beating at a slightly different frequency.
       According to Ralph Brown interrupt list, the tick frequency is now
		       1,573,040 / day = 65536 / 3599.597213s */
    /* #s  = #T/F = #T*3600/65536
       #µs = #T * 3,599,597,213 / 65,536 = (#T * 3,599,597,213) >> 16 */
    mult32(dw, 3599597213, &utc);
    WORD0(utc) = WORD1(utc);
    WORD1(utc) = WORD2(utc);
    WORD2(utc) = WORD3(utc);
    WORD3(utc) = 0;

    _bios_getdate(&dd);
    iDay = dd.day - 1;		    /* 0-based index */
    iMonth = dd.month - 1;	    /* 0-based index */
    iYear = dd.year - 1980;	    /* Make sure computations don't overflow */
    if (iMonth < 2)		    /* January or february */
	{
	iMonth += 12;
	iYear -= 1;
	}
    /* Days since march 1st, 1980 */
    dw = iDay + ytd[iMonth] + ((iYear/4)*1461) + ((iYear%4)*365);
#ifdef _DEBUG
    if (iDebug)
	{
	printf("%d/%02d/%02d = %ld days since 1980/03/01\n",
		dd.year, dd.month, dd.day, dw);
	}
#endif /* _DEBUG */
    dw *= 86400; /* Seconds since march 1st, 1980 */
    mult32(dw, 10000000, &usecs);
    ADD_64b_2_64b(&usecs, &utc, &utc);

    /* Offset between UUID formatted times and DOS formatted times.
     * UUID UTC base time is October 15, 1582.
     * Unix base time is January 1, 1970. Delta = 0x01B21DD2 13814000
     * DOS base time is March 1, 1980.	  Delta = 0x000B64E6 FD600000 */
    os_basetime_diff.dw0 = 0x110E4000;
    os_basetime_diff.dw1 = 0x01BD82B9;
    ADD_64b_2_64b(&utc, &os_basetime_diff, uuid_time);
#ifdef _DEBUG
    if (iDebug)
	{
	printf("So now = 1582/10/15 + 0x%08lX %08lX 1/10th ns\n",
		uuid_time->dw1, uuid_time->dw0);
	}
#endif /* _DEBUG */
}

#endif /* defined(_BIOS) || defined(_LODOS) */

/*
** See "The Multiple Prime Random Number Generator" by Alexander
** Hass pp. 368-381, ACM Transactions on Mathematical Software,
** 12/87.
*/
static DWORD rand_m;
static DWORD rand_ia;
static DWORD rand_ib;
static DWORD rand_irand;

static void
true_random_init(void)
{
    QWORD t;
    WORD seed;

 /* Generating our 'seed' value Start with the current time, but,
  * since the resolution of clocks is system hardware dependent and
  * most likely coarser than our resolution (10 usec) we 'mixup' the
  * bits by xor'ing all the bits together.  This will have the effect
  * of involving all of the bits in the determination of the seed
  * value while remaining system independent.  Then for good measure
  * to ensure a unique seed when there are multiple processes
  * creating UUIDs on a system, we add in the PID.
  */
    rand_m = 971;
    rand_ia = 11113;
    rand_ib = 104322;
    rand_irand = 4181;
    get_system_time(&t);
    seed  = WORD0(t.dw0);    /* ~~jfl 1997/09/02 Use simplified macros */
    seed ^= WORD1(t.dw0);
    seed ^= WORD0(t.dw1);
    seed ^= WORD1(t.dw1);
    rand_irand += seed + getpid();
}

static WORD
true_random(void)
{
    if ((rand_m += 7) >= 9973)
        rand_m -= 9871;
    if ((rand_ia += 1907) >= 99991)
        rand_ia -= 89989;
    if ((rand_ib += 73939) >= 224729)
        rand_ib -= 96233;
    rand_irand = (rand_irand * rand_m) + rand_ia + rand_ib;
    return (WORD)((rand_irand >> 16) ^ (rand_irand & RAND_MASK));
}

/*
**  Startup initialization routine for the UUID module.
*/
void
uuid_init(void)
{
    true_random_init();
    get_system_time(&time_last);
#ifdef NONVOLATILE_CLOCK
    clock_seq = read_clock();
#else
    clock_seq = true_random();
#endif
}

static int
time_cmp(QWORD *time1, QWORD *time2)
{
    if (time1->dw1 < time2->dw1) return -1;
    if (time1->dw1 > time2->dw1) return 1;
    if (time1->dw0 < time2->dw0) return -1;
    if (time1->dw0 > time2->dw0) return 1;
    return 0;
}

static void new_clock_seq(void)
{
    clock_seq = (clock_seq + 1) % (CLOCK_SEQ_LAST + 1);
    if (clock_seq == 0) clock_seq = 1;
#ifdef NONVOLATILE_CLOCK
    write_clock(clock_seq);
#endif
}

void uuid_create(uuid_t *uuid)
{
    static QWORD    time_now;
    static WORD     time_adjust;
    BYTE            eaddr[6];
    int             got_no_time = 0;
    BYTE	    pcuuid[16];
    int		    i;

    if (!GetMacAddress(eaddr))	  /* TO BE PROVIDED */
        {
        /* If no MAC address found, build a fake one with the PC UUID. */
        if (GetPcUuid((uuid_t*)pcuuid))
            {
#ifdef _DEBUG
	    if (iDebug) printf("Warning: No MAC address found!\n");
#endif /* _DEBUG */
	    memset(eaddr, 0, 6);
	    for (i=0; i<16; i++) eaddr[i%6] ^= pcuuid[i];
	    }
	else
	    {
	    /* Else use a random value */
#ifdef _DEBUG
	    if (iDebug) printf("Warning: No MAC address found! Using random value.\n");
#endif /* _DEBUG */
	    for (i=0; i<6; i++) eaddr[i] = (BYTE)rand();
            }
        }


    do {
        get_system_time(&time_now);
        switch (time_cmp(&time_now, &time_last)) {
        case -1:
            /* Time went backwards. */
            new_clock_seq();
            time_adjust = 0;
            break;
        case 1:
            time_adjust = 0;
            break;
        default:
            if (time_adjust == 0x7FFF)
                /* We're going too fast for our clock; spin. */
                got_no_time = 1;
            else
                time_adjust++;
            break;
        }
    } while (got_no_time);

    time_last.dw0 = time_now.dw0;
    time_last.dw1 = time_now.dw1;

    if (time_adjust != 0) {
        ADD_16b_2_64b(&time_adjust, &time_now, &time_now);
    }

    /* Construct a uuid with the information we've gathered
     * plus a few constants. */
    uuid->time_low = time_now.dw0;
    uuid->time_mid = WORD0(time_now.dw1);
    uuid->time_hi_and_version = WORD1(time_now.dw1);
    uuid->time_hi_and_version |= (1 << 12);
    uuid->clock_seq_low = BYTE0(clock_seq);
    uuid->clock_seq_hi_and_reserved = (BYTE)(BYTE1(clock_seq) & 0x3F);
    uuid->clock_seq_hi_and_reserved |= (BYTE)0x80;
    memcpy(uuid->node, &eaddr, sizeof uuid->node);

#ifdef _DEBUG
    if (iDebug) { printf("Generated UUID "); PrintUuid(uuid); printf("\n"); }
#endif /* _DEBUG */
}

#endif /* _MSDOS */

/*****************************************************************************\
*									      *
*				    The End				      *
*									      *
\*****************************************************************************/

