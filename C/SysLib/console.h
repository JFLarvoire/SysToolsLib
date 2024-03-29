/***************************** :encoding=UTF-8: ******************************\
*									      *
*  File name	    console.h						      *
*									      *
*  Description      Define console (= terminal) management functions	      *
*									      *
*  Notes	    							      *
*		    							      *
*  History								      *
*    2023-01-16 JFL jf.larvoire@free.fr created this file.		      *
*    2023-04-18 JFL Moved GetScreenRows() & GetScreenCols() to SysLib.	      *
*                   Renamed them as GetConRows() & GetConCols().	      *
*    2023-04-19 JFL Redefined the DOS version of GetCon*() as macros.	      *
*		    							      *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SYSLIB_CONSOLE_H_
#define _SYSLIB_CONSOLE_H_

#include "SysLib.h"		/* SysLib Library core definitions */

int GetCursorPosition(int *pX, int *pY);
int SetCursorPosition(int iX, int iY);

#ifdef _MSDOS
#define GetConRows()    (*(unsigned char far *)0x00400084L + 1)  /* Byte[40:84] = 0-based index of the last row */
#define GetConColumns() (*(int far *)0x0040004AL)		 /* Word[40:4A] = Number of columns */
#else
int GetConRows(void);
int GetConColumns(void);
#endif

#endif /* _SYSLIB_CONSOLE_H_ */
