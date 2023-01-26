/*****************************************************************************\
*									      *
*  File name	    SetCurPos.c						      *
*									      *
*  Description      Set the console cursor position			      *
*									      *
*  Notes	    The Unix version requires linking with option -ltermcap   *
*		    Termcap is part of the libncurses5-dev package. Ex:       *
*		    sudo apt-get install libncurses5-dev                      *
*		    							      *
*  History								      *
*    2023-01-25 JFL jf.larvoire@free.fr created this module.		      *
*		    							      *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include <config.h>	/* The Unix version has multiple alternatives */

#include "console.h"

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#endif /* __unix__ */

/*---------------------------------------------------------------------------*\
*									      *
|   Function	    SetCursorPosition					      |
|									      |
|   Description     Set the coordinates of the cursor on the text screen      |
|									      |
|   Arguments								      |
|		    							      |
|   Return value    0 = Success						      |
|		    							      |
|   Notes	    Make sure you flush the console file output first!	      |
|		    							      |
|   History								      |
*									      *
\*---------------------------------------------------------------------------*/

/****************************** MS_DOS Version *******************************/

#ifdef _MSDOS

#include "utildef.h"

int SetCursorPosition(int iX, int iY) {
  set_cursor_position((short)iX, (short)iY); /* Set it using the BIOS int 10H */
  return 0;
}

#endif

/****************************** WIN32 Version ********************************/

#ifdef _WIN32

#include <windows.h>

int SetCursorPosition(int iX, int iY) {
  COORD coord;

  coord.X = (SHORT)iX;
  coord.Y = (SHORT)iY;
  return !SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

#endif

/******************************* Unix Version ********************************/

#ifdef _UNIX

#include <stdio.h>

int SetCursorPosition(int iX, int iY) {
  /* Set the cursor position using the ANSI escape sequence */
  printf("\x1B[%d;%dH", iY, iX);
  fflush(stdout);
  return 0;
}

#endif /* _UNIX */

