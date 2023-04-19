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
*		    							      *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef _SYSLIB_CONSOLE_H_
#define _SYSLIB_CONSOLE_H_

#include "SysLib.h"		/* SysLib Library core definitions */

int GetCursorPosition(int *pX, int *pY);
int SetCursorPosition(int iX, int iY);

int GetConRows(void);
int GetConColumns(void);

#endif /* _SYSLIB_CONSOLE_H_ */
