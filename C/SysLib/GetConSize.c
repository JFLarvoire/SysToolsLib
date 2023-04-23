/*****************************************************************************\
*                                                                             *
*  Filename	    GetConSize.c					      *
*									      *
*  Description      Get the number of rows or columns of the text console     *
*									      *
*  Notes	    The Unix version is more efficient when using the termcap *
*		    library. This requires linking with option -ltermcap.     *
*		    Termcap is part of the libncurses5-dev package. Ex:       *
*		    sudo apt-get install libncurses5-dev                      *
*		    							      *
*  History								      *
*    2023-04-18 JFL Moved GetScreenRows() & GetScreenCols() to SysLib.	      *
*                   Renamed them as GetConRows() & GetConCols().	      *
*    2023-04-19 JFL Redefined the DOS version as macros.		      *
*                   Added the missing inclusion of <config.h>.                *
*                                                                             *
*       © Copyright 2016-2018 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _GNU_SOURCE	/* ISO C, POSIX, BSD, and GNU extensions */

#include <config.h>	/* The Unix version has multiple alternatives */

#include "console.h"	/* SysLib console management functions */

#include "debugm.h"	/* SysToolsLib debug macros */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Functions	    GetConRows						      |
|		    GetConCols						      |
|		    							      |
|   Description     Get the number of rows or columns of the text console     |
|									      |
|   Returns	    The number of rows or columns on the text console	      |
|									      |
|   Notes	    							      |
|		    							      |
|   History								      |
|    1990-03-21 JFL Created GetScreenRows() & GetScreenCols() for DOS.	      |
|    1992-02-28 JFL Added OS/2 versions.				      |
|    1995-01-17 JFL Added Win32 versions.				      |
|    2005-02-22 JFL Fixed bug with GetScreenRows, off by 1 line in DOS.       |
|    2005-05-03 JFL Added Unix versions.				      |
|    2023-04-18 JFL Renamed them as GetConRows() & GetConCols().	      |
|    2023-04-19 JFL Simplified Unix' Exec() using popen().      	      |
*									      *
\*---------------------------------------------------------------------------*/

/******************************************************************************
*                                                                             *
*                               MS_DOS Version                                *
*                                                                             *
******************************************************************************/

#ifdef _MSDOS

/* Now defined as macros. Here are the original definitions as functions */
#undef GetConRows
#undef GetConColumns

typedef unsigned char uint8;

int GetConRows(void) {
  uint8 far *fpc;

  fpc = (uint8 far *)0x00400084L;   /* Byte[40:84] = Index of the last row */
  return *fpc + 1;		    /* Add 1 to get the number of rows */
}

int GetConColumns(void) {
  return *(int far *)0x0040004AL;   /* Word[40:4A] = Number of columns */
}

#endif

/******************************************************************************
*                                                                             *
*                               OS/2 Version                                  *
*                                                                             *
******************************************************************************/

#ifdef _OS2

#define INCL_VIO
#include "os2.h"

/* Make sure to include os2.h at the beginning of this file, and before that
    to define the INCL_VIO constant to enable the necessary section */

int GetConRows(void) {
  VIOMODEINFO vmi;

  VioGetMode(&vmi, 0);

  return vmi.row;
}

int GetConColumns(void) {
  VIOMODEINFO vmi;

  VioGetMode(&vmi, 0);

  return vmi.col;
}

#endif

/******************************************************************************
*                                                                             *
*                               WIN32 Version                                 *
*                                                                             *
******************************************************************************/

#ifdef _WIN32

#include "windows.h"

/* Make sure to include windows.h at the beginning of this file, and especially
    the kernel section */

int GetConRows(void) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    return 0;   /* Disable pause mode if console size unknown */

  return csbi.srWindow.Bottom + 1 - csbi.srWindow.Top;
}

int GetConColumns(void) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    return 0;   /* Disable pause mode if console size unknown */

  return csbi.srWindow.Right + 1 - csbi.srWindow.Left;
}

#endif

/*****************************************************************************\
*									      *
*				 Unix Version				      *
*									      *
\*****************************************************************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */
#define _UNIX
#endif

#ifdef _UNIX

#include <stdio.h>
#include <unistd.h>

#if defined(HAS_TERMCAP) && HAS_TERMCAP

/*
#if defined(_DEBUG)
#pragma message "Building the termcap library version"
#endif
*/

/* Requires linking with the  -ltermcap option */

#include <termcap.h>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

static char term_buffer[2048];
static int tbInitDone = FALSE;

int init_terminal_data() {
  char *termtype;
  int success;

  if (tbInitDone) return 0;

  termtype = getenv("TERM");
  if (termtype == 0) {
    printf("Specify a terminal type with `setenv TERM <yourtype>'.\n");
    exit(1);
  }

  success = tgetent(term_buffer, termtype);
  if (success < 0) {
    printf("Could not access the termcap data base.\n");
    exit(1);
  }
  if (success == 0) {
    printf("Terminal type `%s' is not defined.\n", termtype);
    exit(1);
  }

  tbInitDone = TRUE;
  return 0;
}

int GetConRows(void) {
  init_terminal_data();
  return tgetnum("li");
}

int GetConColumns(void) {
  init_terminal_data();
  return tgetnum("co");
}

#else /* !HAS_TERMCAP */

#if defined(_DEBUG)
#pragma GCC warning "Building the tput command version, which is inefficient. Consider installing the termcap library (part of libncurses5-dev)."
#endif

/* Execute a command, and capture its output */
#define BLOCK_SIZE 1024
char *Exec(char *pszCmd) {
  char *pszBuf = NULL;
  FILE *hFile;

  /* Read the command output via a pipe */
  hFile = popen(pszCmd, "r");
  if (hFile) {
    size_t nReadTotal = 0;
    size_t nRead;
    do {
      char *pszLast = pszBuf;
      pszBuf = realloc(pszBuf, nReadTotal + BLOCK_SIZE);
      if (!pszBuf) { free(pszLast); break; }
      nRead = fread(pszBuf+nReadTotal, 1, BLOCK_SIZE, hFile);
      nReadTotal += nRead;
    } while (!(feof(hFile) || ferror(hFile)));
    if (pszBuf) {
      char *pszLast = pszBuf;
      pszBuf[nReadTotal] = '\0';
      pszBuf = realloc(pszBuf, nReadTotal + 1);
      if (!pszBuf) free(pszLast);
    }
    pclose(hFile);
  }

  return pszBuf; /* Must be freed by the caller */
}

int GetConRows(void) {
  int nRows = 25; /* Default for VGA screens */
  /* char *pszRows = getenv("LINES"); */
  /* if (pszRows) nRows = atoi(pszRows); */
  char *pszBuf = Exec("tput lines");
  if (pszBuf) {
    nRows = atoi(pszBuf);
    free(pszBuf);
  }
  return nRows;
}

int GetConColumns(void) {
  int nCols = 80; /* Default for VGA screens */
  /* char *pszCols = getenv("COLUMNS"); */
  /* if (pszCols) nCols = atoi(pszCols); */
  char *pszBuf = Exec("tput cols");
  if (pszBuf) {
    nCols = atoi(pszBuf);
    free(pszBuf);
  }
  return nCols;
}

#endif /* !HAS_TERMCAP */

#endif /* _UNIX */

/******************************************************************************
*                                                                             *
*                         End of OS-specific routines                         *
*                                                                             *
******************************************************************************/

