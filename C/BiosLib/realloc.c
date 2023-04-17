/************************ :encoding=UTF-8:tabSize=8: *************************\
*									      *
*   File name	    realloc.c						      *
*									      *
*   Description     Resize a memory block				      *
*									      *
*   Notes	    WARNING: Only works with the last allocated block.	      *
*		    WARNING: As with BiosLib's malloc, no error checking done.*
*		    							      *
*   History								      *
*    2023-04-17 JFL Created this module.				      *
*									      *
*                   © Copyright 2023 Jean-François Larvoire                   *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    realloc						      |
|									      |
|   Description     Resize a memory block				      |
|									      |
|   Parameters      void *buf	    	The buffer to resize		      |
|		    size_t len		Its new requested length	      |
|		    							      |
|   Returns	    The address of the resized buffer			      |
|									      |
|   Notes	    A limited subset of the Standard C library routine	      |
|		    							      |
|		    WARNING: Only works with the last allocated block.	      |
|		    WARNING: As with BiosLib's malloc, no error checking done.|
|		    							      |
|   History								      |
|    2023-02-17 JFL Created this routine in chars.c for building a LODOS vers.|
|    2023-04-17 JFL Moved it to BiosLib, and added a minimal safety check     |
|		    verifying it's the last allocated block.		      |
*									      *
\*---------------------------------------------------------------------------*/

/* Quick & Dirty hack to allow compiling some SysToolsLib programs for LODOS */
void *realloc(void *buf, size_t len) {
  if (!buf) return malloc(len);	/* Just allocate a new block */
  if (buf != malloc_last) return NULL; /* Can only resize the last allocated block */
  malloc_base = (char *)buf + len;
  return buf;
}
