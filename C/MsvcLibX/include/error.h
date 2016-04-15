/* The GNU CoreUtils library error function */
/* msvclibx: Makeshit implementation as a macro with 4 arguments */

/* Gotcha: The Windows SDK also contains a file called error.h */
/* See C:\Program Files\Microsoft SDKs\Windows\v7.0\INCLUDE */

#ifndef _ERROR_H_
#define _ERROR_H_

#include "msvclibx.h"

extern void error(int status, int errnum, const char *format, ...);

#endif
