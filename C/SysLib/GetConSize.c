/*****************************************************************************\
*                                                                             *
*   Filename	    GetConSize.c					      *
*									      *
*   Description     Get the number of rows or columns of the text console     *
*									      *
*   Notes	    							      *
*		    							      *
*   History								      *
*    2023-04-18 JFL Moved GetScreenRows() & GetScreenCols() to SysLib.	      *
*                   Renamed them as GetConRows() & GetConCols().	      *
*    2023-04-19 JFL Redefined the DOS version as macros.		      *
*                                                                             *
*       © Copyright 2016-2018 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define _GNU_SOURCE	/* ISO C, POSIX, BSD, and GNU extensions */

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

/* Requires linking with the  -ltermcap option */

#include <termcap.h>

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

/* Execute a command, and capture its output */
#define TEMP_BLOCK_SIZE 1024
char *Exec(char *pszCmd) {
  size_t nBufSize = 0;
  char *pszBuf = malloc(0);
  size_t nRead = 0;
  int iPid = getpid();
  char szTempFile[32];
  char *pszCmd2 = malloc(strlen(pszCmd) + 32);
  int iErr;
  FILE *hFile;
  sprintf(szTempFile, "/tmp/RowCols.%d", iPid);
  sprintf(pszCmd2, "%s >%s", pszCmd, szTempFile);
  iErr = system(pszCmd2);
  if (iErr) {
    free(pszBuf);
    return NULL;
  }
  /* Read the temp file contents */
  hFile = fopen(szTempFile, "r");
  while (1) {
    char *pszBuf2 = realloc(pszBuf, nBufSize + TEMP_BLOCK_SIZE);
    if (!pszBuf2) break;
    pszBuf = pszBuf2;
    nRead = fread(pszBuf+nBufSize, 1, TEMP_BLOCK_SIZE, hFile);
    nBufSize += TEMP_BLOCK_SIZE;
    if (nRead < TEMP_BLOCK_SIZE) break;
    if (feof(hFile)) break;
  }
  fclose(hFile);
  /* Cleanup */
  remove(szTempFile);
  free(pszCmd2);
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

