/*****************************************************************************\
*                                                                             *
*   File name	    progname.h						      *
*                                                                             *
*   Description	    Define the getprogname() routine			      *
*                                                                             *
*   Notes	                                                              *
*                                                                             *
*   History                                                                   *
*    2021-12-21 JFL Created this file.					      *
*                                                                             *
\*****************************************************************************/

/* Define an OS-dependant getprogname() routine or macro */
#if defined(_MSC_VER)	/* MS-DOS or Windows */
  extern const char *getprogname(void);	/* Implemented in SysLib */
#elif defined(_UNIX)	/* Ex: Linux, MacOS, FreeBSD */
#  if defined(__USE_GNU) /* Defined by GNU include files if _GNU_SOURCE is pre-defined */
#    include <errno.h>	/* Defines program_invocation_name and program_invocation_short_name */
#    define getprogname() program_invocation_short_name
#  elif defined(__MACH__) && !defined(_ANSI_SOURCE) && (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
#    include <stdlib.h>	/* Defines getprogname() for MacOS */
#  elif defined(__BSD_VISIBLE) /* Defined by FreeBSD in all cases */
#    include <stdlib.h>	/* Defines getprogname() for FreeBSD */
#  else
#    error "No available getprogname() definition. Try defining _GNU_SOURCE or _DARWIN_C_SOURCE."
#  endif
#else
#  error "No available getprogname() definition for this OS"
#endif

