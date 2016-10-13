/*****************************************************************************\
*                                                                             *
*   Filename:	    qword.cpp						      *
*									      *
*   Description:    Define a 64-bits integer class usable in C++ arithmetics. *
*                   A structure-based makeshift if also provided for C.       *
*                                                                             *
*   Notes:	    MSVC 1.5 lacks a 64-bits integer type.		      *
*		    We needed one to use EFI include files in DOS programs.   *
*		    Constraints:					      *
*		    - QWORD type usable as base types in computations.        *
*			==> Redefine all operator xx functions.		      *
*		    - No constructor, to avoid problems with unions.	      *
*			==> Use friend functions _QWORD for initializations.  *
*									      *
*		    ISSUE: Constants created by the _QWORD() functions are    *
*		    not deleted automatically. This causes memory leaks.      *
*									      *
*		    TO DO:						      *
*		    - Add missing operator xx functions.		      *
*		    - Add conversion functions to other integer types.	      *
*		    - Add Conversion function to decimal string, etc.	      *
*									      *
*   History:								      *
*    2000-09-15 JFL Created this file.                                        *
*    2002-11-05 JFL Fixed bug in QWORD::operator/=(), due to MSVC 1.52 bug!   *
*    2003-01-16 JFL Added operations + += - -= * *= / /= with integer args.   *
*		    Changed arguments QWORD& to const QWORD&.		      *
*    2003-01-17 JFL Conditionally compiled-out all operatorX functions. They  *
*		    may be useless when operatorX= functions are defined.     *
*		    Added operator%=.					      *
*		    Fixed memory leak of operatorX functions.		      *
*    2003-03-22 JFL Adapted opfFormat() and operator OPFARG() to new oprintf.h.
*    2010-10-08 JFL Bugfix: Changed strtoqw's first argument to const char *. * 
*    2015-08-18 JFL Added operator<< and operator>>.                          * 
*    2015-11-04 JFL Fixed mul4x4() in the large memory model.                 * 
*    2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
*									      *
\*****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>     // For _ui64toa()
#include <string.h>
#include <ctype.h>	// For isxdigit()

#include "qword.h"

#define TRUE 1
#define FALSE 0

// Useful constants
// Don't initialize them statically here, as this causes a far function call,
// incompatible with the tiny memory model.
QWORD qwZero;	// Initialized by default to 0 by C init code.

/*****************************************************************************\
*           First the full-fledged implementation for C++.                    *
\*****************************************************************************/

#ifdef __cplusplus

#ifdef _MSDOS	// 16-bits MS-DOS or BIOS programs

// Pseudo constructors, implemented as friend functions.
// This is done to avoid problems with structures containing QWORDs,
//  which generate compilation errors if any constructor is defined.

// #pragma message("Using QWORD C++ class definition.")

QWORD& _QWORD()
    {
    QWORD *pqw = new QWORD;
    pqw->dw0 = pqw->dw1 = 0;
    return *pqw;
    }    

QWORD& _QWORD(unsigned long dw)
    {
    QWORD *pqw = new QWORD;
    pqw->dw0 = dw;
    pqw->dw1 = 0;
    return *pqw;
    }    

QWORD& _QWORD(unsigned long dw0, unsigned long dw1)
    {
    QWORD *pqw = new QWORD;
    pqw->dw0 = dw0;
    pqw->dw1 = dw1;
    return *pqw;
    }    

QWORD& _QWORD(const QWORD& qw)
    {
    QWORD *pqw = new QWORD;
    pqw->dw0 = qw.dw0;
    pqw->dw1 = qw.dw1;
    return *pqw;
    }    

// Now the QWORD arithmetic manipulation routines.

QWORD& QWORD::operator+=(const QWORD& qw2)
    {
    unsigned long dwLow;
    dwLow = dw0 + qw2.dw0;
    dw1 += qw2.dw1;
    if (dwLow < dw0) dw1 += 1;		// Carry
    dw0 = dwLow;
    return *this;
    }

QWORD& QWORD::operator+=(DWORD dw2)
    {
    unsigned long dwLow;
    dwLow = dw0 + dw2;
    if (dwLow < dw0) dw1 += 1;		// Carry
    dw0 = dwLow;
    return *this;
    }

#if ALL_OPERATORS_NEEDED

QWORD QWORD::operator-(const QWORD& qw2) const
    {
    QWORD qw;
    qw = *this;
    qw -= qw2;
    return qw;
    }

#endif // ALL_OPERATORS_NEEDED

QWORD& QWORD::operator-=(const QWORD& qw2)
    {
    unsigned long dwLow;
    dwLow = dw0 - qw2.dw0;
    dw1 -= qw2.dw1;
    if (dwLow > dw0) dw1 -= 1;		// Carry
    dw0 = dwLow;
    return *this;
    }

#define DB(x) __asm _emit x			// BYTE
#define DW(x) DB((x) & 0xFF) DB((x) >> 8U)	// WORD
#define DD(x) DW((x) & 0xFFFF) DW((x) >> 16U)	// DWORD

#define DATASIZE DB(0x66)	    // Prefix changes next instr. data size
#define ADRSIZE DB(0x67)	    // Prefix changes next instr. address size

#pragma warning(disable:4704)

void mul4x4(DWORD dw0, DWORD dw1, QWORD *pqwResult)
    {
    _asm
    	{
    	; mov	eax, dw0
    	DATASIZE
    	mov	ax, word ptr dw0
    	
    	; mul	dw1
    	DATASIZE
    	mul	word ptr dw1

#if M_I86SM || M_I86MM	/* Tiny, small, and medium memory models with a single data segment */
	mov	bx, pqwResult
#else
	les	bx, pqwResult
#endif

	; Warning: The following assembly code assumes QWORD is stored in memory as 2 consecutive DWORDs.
	    	
    	; mov	pqwRestult->dw0, eax
    	DATASIZE
#if M_I86SM || M_I86MM	/* Tiny, small, and medium memory models with a single data segment */
    	mov	word ptr [bx], ax
#else
    	mov	word ptr es:[bx], ax
#endif

    	; mov	pqwResult->dw1, edx
    	DATASIZE
#if M_I86SM || M_I86MM	/* Tiny, small, and medium memory models with a single data segment */
    	mov	word ptr [bx+4], dx
#else
    	mov	word ptr es:[bx+4], ax
#endif
    	}

    return;
    }

DWORD div8x4(QWORD qw, DWORD dw)
    {
    _asm
    	{
	; Warning: The following assembly code assumes QWORD is stored in memory as 2 consecutive DWORDs.
	    	
    	; mov	eax, qw.dw0
    	DATASIZE
    	mov	ax, word ptr qw
    	
    	; mov	edx, qw.dw1
    	DATASIZE
    	mov	dx, word ptr qw+4
    	
    	; div	dw
    	DATASIZE
    	div	word ptr dw
    	
    	; mov	dw, eax
    	DATASIZE
    	mov	word ptr dw, ax
    	}

    return dw;
    }

DWORD mod8x4(QWORD qw, DWORD dw)
    {
    _asm
    	{
	; Warning: The following assembly code assumes QWORD is stored in memory as 2 consecutive DWORDs.
	    	
    	; mov	eax, qw.dw0
    	DATASIZE
    	mov	ax, word ptr qw
    	
    	; mov	edx, qw.dw1
    	DATASIZE
    	mov	dx, word ptr qw+4
    	
    	; div	dw
    	DATASIZE
    	div	word ptr dw

    	; mov	dw, edx
    	DATASIZE
    	mov	word ptr dw, dx
    	}
    
    return dw;
    }

#pragma warning(default:4704)

#if ALL_OPERATORS_NEEDED

QWORD QWORD::operator*(DWORD dw2) const
    {
    QWORD qw;
    qw = *this;
    qw *= dw2;
    return qw;
    }

#endif // ALL_OPERATORS_NEEDED

QWORD& QWORD::operator*=(DWORD dw2)
    {
    DWORD dwHi = dw1 * dw2;
    mul4x4(dw0, dw2, this);
    dw1 += dwHi;
    return *this;
    }

#if ALL_OPERATORS_NEEDED

QWORD QWORD::operator/(DWORD dw2) const
    {
    QWORD qw;
    qw = *this;
    qw /= dw2;
    return qw;
    }

#endif // ALL_OPERATORS_NEEDED

#pragma optimize("", off)   // ~~jfl 2002/11/05 Disable all optimizations to fix compiler bug!
QWORD& QWORD::operator/=(DWORD dw2)
    {
    DWORD dwHi = dw1/dw2;
    dw1 %= dw2;
    dw0 = div8x4(*this, dw2); // ~~jfl 2002/11/05 MSVC 1.52 compiler bug: *this aliasing with {dw0, dw1} not managed correctly.
    dw1 = dwHi;
    return *this;
    }
#pragma optimize("", on)    // ~~jfl 2002/11/05 Restore all original settings.

DWORD QWORD::operator%(DWORD dw2) const
    {
    QWORD qw = *this;
    qw /= dw2;
    qw *= dw2;
    return dw0 - qw.dw0;
    }

QWORD& QWORD::operator%=(DWORD dw2)
    {
    QWORD qw = *this;
    qw /= dw2;
    qw *= dw2;
    return *this -= qw;
    }

QWORD& QWORD::operator<<=(int i)
    {
    if (i<0) return *this >>= -i;
    if (i>64) i=64;
    while (i--)
	{
	dw1 <<= 1;
	if (dw0 & 0x80000000) dw1 |= 1;
	dw0 <<= 1;
	}
    return *this;
    }

QWORD& QWORD::operator>>=(int i)
    {
    if (i<0) return *this <<= -i;
    if (i>64) i=64;
    while (i--)
	{
	dw0 >>= 1;
	if (dw1 & 1) dw0 |= 0x80000000;
	dw1 >>= 1;
	}
    return *this;
    }

#if ALL_OPERATORS_NEEDED

QWORD QWORD::operator<<(int i) const
    {
    QWORD qw;
    qw = *this;
    qw <<= i;
    return qw;
    }

QWORD QWORD::operator>>(int i) const
    {
    QWORD qw;
    qw = *this;
    qw >>= i;
    return qw;
    }

#endif // ALL_OPERATORS_NEEDED

// Comparison operators

int QWORD::operator!(void) const
    {
    return (*this == qwZero);
    }

int QWORD::operator==(const QWORD& qw2) const
    {
    return (dw0 == qw2.dw0) && (dw1 == qw2.dw1);
    }

int QWORD::operator!=(const QWORD& qw2) const
    {
    return (dw0 != qw2.dw0) || (dw1 != qw2.dw1);
    }

int QWORD::operator>(const QWORD& qw2) const
    {
    if (dw1 > qw2.dw1) return 1;
    if (dw1 < qw2.dw1) return 0;
    // If the high DWORDs are the same, compare the low DWORDs.
    return (dw0 > qw2.dw0);
    }

int QWORD::operator>=(const QWORD& qw2) const
    {
    if (dw1 > qw2.dw1) return 1;
    if (dw1 < qw2.dw1) return 0;
    // If the high DWORDs are the same, compare the low DWORDs.
    return (dw0 >= qw2.dw0);
    }

int QWORD::operator<(const QWORD& qw2) const
    {
    if (dw1 < qw2.dw1) return 1;
    if (dw1 > qw2.dw1) return 0;
    // If the high DWORDs are the same, compare the low DWORDs.
    return (dw0 < qw2.dw0);
    }

int QWORD::operator<=(const QWORD& qw2) const
    {
    if (dw1 < qw2.dw1) return 1;
    if (dw1 > qw2.dw1) return 0;
    // If the high DWORDs are the same, compare the low DWORDs.
    return (dw0 <= qw2.dw0);
    }

// Comparison to normal integers

int QWORD::operator==(unsigned long dw) const
    {
    if (dw1) return 0;	// Anything with more than 32 bits cannot be equal to any DWORD
    return (dw0 == dw);
    }

int QWORD::operator!=(unsigned long dw) const
    {
    if (dw1) return 1;	// Anything with more than 32 bits cannot be equal to any DWORD
    return (dw0 != dw);
    }

int QWORD::operator>(unsigned long dw) const
    {
    if (dw1) return 1;	// Anything with more than 32 bits is greater than any DWORD
    return (dw0 > dw);
    }

int QWORD::operator>=(unsigned long dw) const
    {
    if (dw1) return 1;	// Anything with more than 32 bits is greater than any DWORD
    return (dw0 >= dw);
    }

int QWORD::operator<(unsigned long dw) const
    {
    if (dw1) return 0;	// Anything with more than 32 bits is greater than any DWORD
    return (dw0 < dw);
    }

int QWORD::operator<=(unsigned long dw) const
    {
    if (dw1) return 0;	// Anything with more than 32 bits is greater than any DWORD
    return (dw0 <= dw);
    }

// Conversion operators
    
#ifndef MINICOMS	// BiosLib does not support floats or doubles

QWORD::operator double() const		// Conversion operator: Cast (double)QWORD
    {
    double d;
    
    d = dw1;
    d *= (1L << 16);
    d *= (1L << 16);
    d += dw0;
    
    return d;
    }

#endif

// Formatting functions

char* qwtox(const QWORD& qw, char *pBuf)	// Convert to hexadecimal string - Buffer provided.
    {
    qwtostr(qw, pBuf, 16);
    return pBuf; // Allows to use ToHex() as an argument to printf() in the calling routine.
    }

#ifndef NO_OPRINTF

int strcpyform(
char *to,       /* Where to copy the from string */
char *from,     /* String to copy */
int size,       /* Number of characters to copy or 0 = copy actual size */
int actual,     /* Actual size of the string to copy */
char cFill)     /* Character to use to fill the left of the field */
    {
    int right = TRUE;		  /* TRUE if right justified */
    int fill = 0;

    if (actual < 0) return 0;	  /* Error */
    if (size == 0) size = actual; /* Free form copy */
    if (size < 0)		  /* Left justify */
	{
	right = FALSE;
	size = -size;
	}
    if (size < actual)		  /* Copy only part of the string */
	{
	if (right) from += actual - size;
	actual = size;
	}
    fill = size - actual;	  /* Number of spaces to add */

    if (right)			  /* If right justified ... */
	{
	while (fill--) *to++ = cFill; /* ... fill left with fill character */
	}
    while (actual--) *to++ = *from++; /* Actual copy */
    if (!right) 		  /* If left justified ... */
	{
	while (fill--) *to++ = ' ';   /* ... fill right with spaces */
	}
    *to = '\0';

    return size;
    }

int QWORD::opfFormat(char *pszOut, size_t iSize, const char *pszForm, const OPFARG *popfArg)
    {
    QWORD *pqw = (QWORD *)(popfArg->pObj);
    int iFormLen;
    int iActualLen;
    char c;
    char szTemp[32];
    char cFill = ' ';   // Default filler for the left side of output fields
    int iBase;
    int iNegative = 0;

    /* Get the format parameters */    
    c = *(pszForm++);
    if (c != '%') return 0;
    
    if (*pszForm == '0')
	{
	cFill = '0';	// Optional filler
	pszForm += 1; 	// Skip the 0
	}

    iFormLen = (int)strtol(pszForm, (char **)&pszForm, 10);
    if ((iFormLen > 0) && ((unsigned)iFormLen >= iSize)) return 0;
    if ((iFormLen < 0) && ((unsigned)(-iFormLen) >= iSize)) return 0;
get_base:
    switch (c = *(pszForm++))
        {
        case 'I':			// Ignore optional I64 size specifier
        case '6':
        case '4':
            goto get_base;
        case 'o': 			// Just for fun define octal too.
            iBase = 8; 
            break;
        case 'u': 			// Unsigned decimal
            iBase = 10;
            break;
        case 'd':			// Signed decimal
        case 'i':
            iBase = 10;
            if (pqw->dw1 & 0x80000000)
                {
                iNegative = 1;
                szTemp[0] = '-';
                // *pqw = -*pqw;
                pqw->dw1 ^= 0xFFFFFFFF;
                pqw->dw0 ^= 0xFFFFFFFF;
                *pqw += (DWORD)1;
                }
            break;
        case 'X':			// Hexadecimal
        case 'x': 
            iBase = 16; 
            break;
        default: return 0;
        }

    /* Convert the QWORD to a local string */
    iActualLen = qwtostr(*pqw, szTemp+iNegative, iBase)+iNegative;
    
    /* Copy it to the output buffer */
    if ((unsigned)iActualLen >= iSize) return 0;
    iActualLen = strcpyform(pszOut, szTemp, iFormLen, iActualLen, cFill);
    
    return iActualLen;
    }

QWORD::operator OPFARG()
    {
    return OPFARG(QWORD::opfFormat, this);
    }

#endif // !defined(NO_OPRINTF)

#endif // defined(_MSDOS)

#ifdef _WIN32	// 32-bits WIN32 programs

// #pragma message("Using QWORD WIN32 __int64 definition.")

// Formatting functions

char* qwtox(const QWORD& qw, char *pBuf)	// Convert to hexadecimal string - Buffer provided.
    {
    return _ui64toa(qw, pBuf, 16);			// Use stdlib.h routine _ui64toa(), in base 16.
    }

#endif // defined(_WIN32)

// Then functions implemented identically in 16-bits and 32-bits C++ :

int printfx(char *pszFormat, const QWORD &qw)	// Print as hexadecimal string - %s format provided.
    {
    char szBuf[18];

    return printf(pszFormat, qwtox(qw, szBuf));
    }

#if 0
int qwtostr(const QWORD &qw, char *pszString, int iBase)
    {
    return 0;
    char *digits = "0123456789ABCDEF";
    int iLen = 0;
    QWORD qw2 = qw;	// Local copy, that can be modifed, without memory leaks.
    
    if (qw2 >= (DWORD)iBase) 
        {
        qw2 /= (DWORD)iBase;
        iLen = qwtostr(qw2, pszString, iBase);
        qw2 = qw;
        }
    pszString[iLen++] = digits[(int)(qw2%(DWORD)iBase)];
    pszString[iLen] = '\0';
    return iLen;
    }
#else
int qwtostr(QWORD qw, char *pszString, int iBase)
    {
    const char *digits = "0123456789ABCDEF";
    int iLen = 0;
    if (qw >= (DWORD)iBase) 
        {
        QWORD qw2 = qw;
        qw2 /= (DWORD)iBase;
        iLen = qwtostr(qw2, pszString, iBase);
        }
    pszString[iLen++] = digits[(int)(qw%(DWORD)iBase)];
    pszString[iLen] = '\0';
    return iLen;
    }
#endif 
    
/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    strtoqw	 					      |
|									      |
|   Description:    Convert a string to a QWORD, in any given base.	      |
|									      |
|   Parameters:     char *pszString	The string to convert 		      |
|		    QWORD& qw		Output value			      |
|		    int iBase		Number base (up to 16)		      |
|									      |
|   Returns:	    The number of characters processed			      |
|                                                                             |
|   History:								      |
|									      |
|    2001/03/20 JFL Created this routine				      |
|    2010/10/08 JFL Changed the first argument type to const char *.	      |
*									      *
\*---------------------------------------------------------------------------*/

int strtoqw(const char *pszString, QWORD &qw, int iBase)
    {
    const char *pc;
    int i;
    
    qw = (DWORD)0;
    
    for (pc = pszString; *pc; pc++)
        {
        if ((!sscanf(pc, "%1x", &i)) || (i>=iBase)) break;
        qw *= (DWORD)iBase;
        qw += (DWORD)i;
        }
        
    return (int)(pc-pszString);
    }

int xtoqw(char *psz, QWORD& qw)
    {
    return strtoqw(psz, qw, 16);
    }

/*****************************************************************************\
*           Then the makeshift implementation for plain C.                    *
\*****************************************************************************/

#else // !defined(cplusplus)

// #pragma message("Using QWORD C structure definition.")

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    Qword2Double					      |
|									      |
|   Description:    Convert a QWORD structure into a C standard double.       |
|									      |
|   Parameters:     QWORD qw						      |
|									      |
|   Returns:	    The corresponding double.				      |
|									      |
|   Notes:	    This is a workaround for the lack of 64-bits integer      |
|		     arithmetics in available 16-bits compilers.	      |
|									      |
|   History:								      |
|									      |
|     2000/09/11 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS

double Qword2Double(QWORD qw)
    {
    double d;
    
    d = qw.dw1;
    d *= (1L << 16);
    d *= (1L << 16);
    d += qw.dw0;
    
    return d;
    }

#endif // _MSDOS

#ifdef _WIN32

double Qword2Double(QWORD qw)
    {	// Unsigned __qword conversion not implemented!
    return ((2*((double)(__int64)((qw)/2)))+((int)(qw)&1));
    }

#endif

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    Dword2Qword						      |
|									      |
|   Description:    Convert a DWORD into a QWORD structure.		      |
|									      |
|   Parameters:     DWORD dw						      |
|									      |
|   Returns:	    The corresponding QWORD.				      |
|									      |
|   Notes:	    This is a workaround for the lack of 64-bits integer      |
|		     arithmetics in available compilers.		      |
|									      |
|   History:								      |
|									      |
|     2000/09/11 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS

QWORD Dword2Qword(DWORD dw)
    {
    QWORD qw;
    
    qw.dw0 = dw;
    qw.dw1 = 0;
    
    return qw;
    }

#endif // _MSDOS

#ifdef _WIN32
// Note: WIN32 version defined natively in qword.h.
#endif // defined(_WIN32)

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    xtoqw						      |
|									      |
|   Description:    Convert an hexadecimal string into a QWORD structure.     |
|									      |
|   Parameters:     char *psz		Input string			      |
|		    QWORD *pqw		Output buffer			      |
|									      |
|   Returns:	    The number of input characters processed.		      |
|									      |
|   Notes:	    This is a workaround for the lack of 64-bits integer      |
|		     arithmetics in available 16-bits compilers.	      |
|									      |
|   History:								      |
|									      |
|     2000/09/11 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS

int xtoqw(char *psz, QWORD *pqw)
    {
    int iLen;

    // Compute the number of hex characters
    for (iLen = 0; psz[iLen]; iLen++) if (!isxdigit(psz[iLen])) break;
    if (iLen<=8)
        {
        pqw->dw0 = xtodw(psz);
        pqw->dw1 = 0;
        }
    else if (iLen<=16)
        {
        char szBuf[9];
        int iLenTop = iLen-8;	// Size of the top DWORD.
        // Make a local copy of the top DWORD.
	strncpy(szBuf, psz, iLenTop);
	szBuf[iLenTop] = '\0';
	// Convert the string using sdtlib function.
	pqw->dw0 = xtodw(psz+iLenTop);
	pqw->dw1 = xtodw(szBuf);
	}
    else
        {
        pqw->dw0 = pqw->dw1 = 0xFFFFFFFF;	// Overflow
        }

    return iLen;
    }

#endif

#ifdef _WIN32

int strtoqw(char *pszString, QWORD *pqw, int iBase)
    {
    char *pc;
    int i;
    
    *pqw = (DWORD)0;
    
    for (pc = pszString; *pc; pc++)
        {
        if ((!sscanf(pc, "%1x", &i)) || (i>=iBase)) break;
        *pqw *= (DWORD)iBase;
        *pqw += (DWORD)i;
        }
        
    return pc-pszString;
    }

int xtoqw(char *psz, QWORD *pqw)
    {
    return strtoqw(psz, pqw, 16);
    }

#endif

/*---------------------------------------------------------------------------*\
|									      *
|   Function:	    qwtox						      |
|									      |
|   Description:    Convert a QWORD into an hexadecimal string.		      |
|									      |
|   Parameters:     char *psz		Input string			      |
|		    QWORD *pqw		Output buffer			      |
|									      |
|   Returns:	    The output buffer address.				      |
|									      |
|   Notes:	    Allows to use qwtox() as an argument to printf().	      |
|									      |
|   History:								      |
|									      |
|     2001/09/07 JFL Initial implementation.				      |
*									      *
\*---------------------------------------------------------------------------*/

#ifdef _MSDOS

char* qwtox(const QWORD qw, char *pBuf)	// Convert to hexadecimal string - Buffer provided.
    {
    int n = 0;
    if (qw.dw1) 
	{
	n = sprintf(pBuf, "%lX", qw.dw1);
	n += sprintf(pBuf, "%08lX", qw.dw0);
	}
    else
	{
	n = sprintf(pBuf, "%lX", qw.dw0);
	}

    return pBuf; // Allows to use qwtox() as an argument to printf() in the calling routine.
    }

#endif // defined(_MSDOS)

#ifdef _WIN32
// Note: WIN32 version defined natively in qword.h.
#endif // defined(_WIN32)

/*****************************************************************************\
*            End of makeshift implementation for plain C.                    *
\*****************************************************************************/

#endif // !defined(__cplusplus)

DWORD xtodw(char *psz)
    {
    char szBuf[9];
    // Prevent overflow on QWORDs by copying the string locally.
    strncpy(szBuf, psz, 8);
    szBuf[8] = '\0';
    // Convert the string using sdtlib function.
    return strtoul(szBuf, NULL, 16);
    }

