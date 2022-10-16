/*****************************************************************************\
*                                                                             *
*   Filename:	    uuidnull.c						      *
*									      *
*   Description:    IsNullUuid() routine				      *
*                                                                             *
*   Notes:	    							      *
*                                                                             *
*   History:								      *
*    2002/01/03 JFL Created this file. 					      *
*		    Moved in routine IsNullUuid() from file uuid.c.	      *
*		    Changed IsNullUuid() argument to (void *).		      *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "uuid.h"

#define FALSE 0
#define TRUE 1

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    IsNullUuid						      |
|									      |
|   Description:    Test if a UUID is the NULL UUID (All zeros).	      |
|									      |
|   Parameters:     void *pUuid		Where to find the 16-bytes UUID.      |
|									      |
|   Returns:	    TRUE if NULL, FALSE otherwise.  			      |
|									      |
|   Notes:								      |
|									      |
|   History:								      |
|									      |
|    2000/09/25 JFL Created this routine				      |
|    2002/01/03 JFL Changed the argument to (void *).			      |
*									      *
\*---------------------------------------------------------------------------*/

int IsNullUuid(uuid_t *pUuid)
    {
    int i;
    BYTE *pb = (BYTE *)pUuid;
    
    for (i=0; i<16; i++) if (pb[i] != 0) return FALSE;
    return TRUE;
    }

