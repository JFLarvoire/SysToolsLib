/*****************************************************************************\
*									      *
*   File name:	    sqrt.c						      *
*									      *
*   Description:    Integer square root 				      *
*									      *
*   Notes:	    Based on the April 1996 article in Doctor Dobb's Journal. *
*									      *
*   History:								      *
*    1996/05/03 JFL Created this file					      *
*									      *
*      (c) Copyright 1996-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "utildef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    isqrt						      |
|									      |
|   Description:    Compute an integer square root			      |
|									      |
|   Parameters:     unsigned int n					      |
|									      |
|   Returns:	    The largest integer less or equal to the square root      |
|									      |
|   Notes:	    Does not use any multiplication			      |
|		    Based on the fact that multiplying by a power of 2 is     |
|		    the same as shifting left by the logarithm base 2.	      |
|									      |
|   History:								      |
|									      |
|    1996/05/03 JFL Created this routine				      |
|    2000/11/08 JFL Changed the return type to signed int, to facilitate use. |
*                                                                             *
\*---------------------------------------------------------------------------*/

int isqrt(unsigned int n)
    {
    unsigned int iRoot; 	    // Solution
    unsigned int iRoot2;	    // Square of the solution
    unsigned int iLogRoot;	    // Log base two of the root
    unsigned int iBit;		    // 2^iLogRoot = 1<<iLogRoot
    unsigned int iTemp; 	    // Temporary result

    if (n < 2) return n;

    // Compute the log base 2 of the square root of n
    iTemp = n;
    iLogRoot = 0;
    while (iTemp >>= 2) iLogRoot += 1;

    // This yields the highest bit of the root
    iRoot = 1 << iLogRoot;	    // First approximation for the root
    iRoot2 = iRoot << iLogRoot;     // First approximation for the root^2

    iBit = iRoot;
    while (iLogRoot--)		    // For every remaining bit in the solution
	{
	iBit >>= 1;			// Next possible bit in the solution
					// Note: iBit = 1 << iLogRoot
	iTemp = (iRoot << 1) + iBit;	// 2*iRoot + iBit
	iTemp <<= iLogRoot;		// (2*iRoot + iBit) * iBit
	iTemp += iRoot2;		// iRoot2 + 2*iRoot*iBit + iBit*iBit
	if (iTemp <= n) 		// if (iRoot + iBit)^2 <= n
	    {
	    iRoot += iBit;		    // Then keep iBit in solution
	    iRoot2 = iTemp;		    // New value of the root square
	    }
	}

    return (int)iRoot;
    }
