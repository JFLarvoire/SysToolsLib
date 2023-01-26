/*****************************************************************************\
*									      *
*  File name	    GetCurPos.c						      *
*									      *
*  Description      Get the console cursor position			      *
*									      *
*  Notes	    The Unix version requires linking with option -ltermcap   *
*		    Termcap is part of the libncurses5-dev package. Ex:       *
*		    sudo apt-get install libncurses5-dev                      *
*		    							      *
*  History								      *
*    2023-01-15 JFL jf.larvoire@free.fr created this module.		      *
*    2023-01-24 JFL Fixed the DOS version and made pointer arguments optional.*
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
|   Function	    GetCursorPosition					      |
|									      |
|   Description     Get the coordinates of the cursor on the text screen      |
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

int GetCursorPosition(int *pX, int *pY) {
  short wX, wY;

  get_cursor_position(&wX, &wY); /* Get it from the BIOS int 10H */

  if (pX) *pX = (int)wX;
  if (pY) *pY = (int)wY;
  return 0;
}

#endif

/****************************** WIN32 Version ********************************/

#ifdef _WIN32

#include <windows.h>

int GetCursorPosition(int *pX, int *pY) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    return 1;
  }

  if (pX) *pX = csbi.dwCursorPosition.X;
  if (pY) *pY = csbi.dwCursorPosition.Y;
  return 0;
}

#endif

/******************************* Unix Version ********************************/

#ifdef _UNIX

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#if defined(HAS_TERMCAP) && HAS_TERMCAP

/* Use the termcap library, for best performance */
/* Requires linking with the -ltermcap option */
/* Termcap is part of the libncurses5-dev package. Ex: install with:
   sudo apt-get install libncurses5-dev
*/

/*
#if defined(_DEBUG)
#pragma message "GetCursorPosition() will use the Termcap library"
#endif
*/

#include <termios.h>
#include <termcap.h>

int GetCursorPosition(int *pX, int *pY) {
  char buf[16];
  int i, n;
  char c;
  struct termios tc, tc0;
  int iErr = 1;

  /* Save the initial TTY echo mode, and turn echo off */
  tcgetattr(0, &tc);
  tc0 = tc;
  tc.c_lflag &= ~(ICANON|ECHO); /* Disable per-line-buffering and echoing */ 
  tcsetattr(0, TCSAFLUSH, &tc); /* Also flush the input buffer */

  /* Query the cursor position using the ANSI escape sequence */
  write(1, "\x1B[6n", 4); /* The terminal sends back ESC [ ROW ; COL R */

  /* Scan the intput until the end of the TTY response: ESC [ ROW ; COL R */
  for (i=0, c='\0'; c != 'R' && i < sizeof(buf); i++) {
    if (!read(0, &c, 1)) {
      perror("GetCursorPosition: Error reading response");
      goto cleanup_and_return;
    }
    buf[i] = c;
  }

  /* Convert the output to a parseable C string */
  if ((i < 2) || (i >= sizeof(buf))) {
    fprintf(stderr, "GetCursorPosition: Received %d characters", i);
    goto cleanup_and_return;
  }
  buf[i] = '\0';
  /* DEBUG fwrite(buf+1, 1, i-1, stderr); */

  /* Parse the output: ESC [ ROW ; COL R */
  if (pX) {
    *pX = 0;
    for (i -= 2, n = 1; i >= 0 && buf[i] != ';'; i--, n *= 10) *pX += (buf[i] - '0') * n;
  }
  if (pY) {
    *pY = 0;
    for (i-- , n = 1; i >= 0 && buf[i] != '['; i--, n *= 10) *pY += (buf[i] - '0') * n;
  }
  /* DEBUG fprintf(stderr, " => %d,%d\n", *pX, *pY); */

  iErr = 0;

cleanup_and_return:
  /* Restore the initial TTY echo mode */
  tcsetattr(0, TCSANOW, &tc0);
  return iErr;
}

#else

/* Use Shell commands */

#pragma GCC warning "GetCursorPosition() will use Bash commands, which is inefficient"

#include <stdlib.h>

int GetCursorPosition(int *pX, int *pY) {
  char buf[16];
  char szOldTTYconf[128] = "stty ";
  int i, n;
  int c;
  int iErr = 0;
  FILE *hPipe = NULL;

  /* Open a pipe, to get the output of a shell command */
  hPipe = popen("OLD=`stty -g`; stty raw -echo; echo $OLD", "r");
  if (!hPipe) {
    fprintf(stderr, "GetCursorPosition: Can't open the pipe. %s\n", strerror(errno));
    iErr = 1;
    goto cleanup_and_return;
  }

  /* Get the initial TTY configuration */
  if (!fgets(szOldTTYconf+5, sizeof(szOldTTYconf)-5, hPipe)) {
    perror("GetCursorPosition: Error reading the old TTY conf");
    iErr = 2;
    goto cleanup_and_return;
  }
  pclose(hPipe); /* Ignore errors */

  /* Query the cursor position using the ANSI escape sequence */
  write(1, "\x1B[6n", 4); /* The terminal sends back ESC [ ROW ; COL R */

  /* Scan the intput until the end of the TTY response: ESC [ ROW ; COL R */
  for (i=0, c='\0'; c != 'R' && i < sizeof(buf); i++) {
    if (!read(0, &c, 1)) {
      perror("GetCursorPosition: Error reading response");
      iErr = 3;
      goto cleanup_and_return;
    }
    buf[i] = c;
  }

  /* Convert the output to a parseable C string */
  if ((i < 2) || (i >= sizeof(buf))) {
    fprintf(stderr, "GetCursorPosition: Received %d characters", i);
    iErr = 4;
    goto cleanup_and_return;
  }
  buf[i] = '\0';
  /* DEBUG fwrite(buf+1, 1, i-1, stderr); */

  /* Parse the output: ESC [ ROW ; COL R */
  if (pX) {
    *pX = 0;
    for (i -= 2, n = 1; i >= 0 && buf[i] != ';'; i--, n *= 10) *pX += (buf[i] - '0') * n;
  }
  if (pY) {
    *pY = 0;
    for (i-- , n = 1; i >= 0 && buf[i] != '['; i--, n *= 10) *pY += (buf[i] - '0') * n;
  }
  /* DEBUG fprintf(stderr, " => %d,%d\n", *pX, *pY); */

  iErr = 0;

cleanup_and_return:
  if (szOldTTYconf[5]) system(szOldTTYconf);
  return iErr;
}

#endif /* HAS_TERMCAP */

#endif /* _UNIX */

