/*****************************************************************************\
*									      *
*   File name:	    dumpbuf.c						      *
*									      *
*   Description:    Dump a memory buffer				      *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1999/08/06 JFL Extracted from earlier projects.			      *
*									      *
*      (c) Copyright 1999-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"
#include "utildef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    DumpBuf						      |
|									      |
|   Description     Display the contents of a memory buffer		      |
|									      |
|   Parameters	    void far *fpBuf Buffer address			      |
|		    WORD wStart     Index of the first byte to display	      |
|		    WORD wStop	    Index of the first byte NOT to display    |
|									      |
|   Notes	    Aligns the output on offsets multiple of 0x10.	      |
|									      |
|   Returns	    None						      |
|									      |
|   History								      |
|    1999/05/26 JFL Extracted this code from older programs.		      |
*									      *
\*---------------------------------------------------------------------------*/

#define PERLINE 0x10			// Number of bytes dumped per line

void DumpBuf(void far *fpBuf, WORD wStart, WORD wStop)
    {
    WORD w, wLine, wColumn;
    unsigned char c;
    char far *fpc = fpBuf;

    for (wLine = wStart - (wStart % PERLINE); wLine < wStop; wLine += PERLINE)
	{
	printf("%04X  ", wLine);

	// On the left hand dump the hexadecimal data
	for (wColumn = 0; wColumn < PERLINE; wColumn++)
	    {
	    if (!(wColumn & 3)) printf(" ");
	    w = wLine + wColumn;
	    if ((w >= wStart) && (w < wStop))
		printf("%02X ", (unsigned char)(fpc[w]));
	    else
		printf("%3s", "");
	    }
	// On the right hand side, display the ASCII characters
	printf(" ");
	for (wColumn = 0; wColumn < PERLINE; wColumn++)
	    {
	    if (!(wColumn & 3)) printf(" ");
	    w = wLine + wColumn;
	    c = fpc[w];
	    if (c < ' ') c = '.';   // Control character would display garbage
	    if ((w >= wStart) && (w < wStop))
		printf("%c", c);
	    else
		printf(" ");
	    }
	printf("\n");
	}

    return;
    }
