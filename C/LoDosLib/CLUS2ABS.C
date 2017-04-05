/*****************************************************************************\
*									      *
*   File name:	    clus2abs.c						      *
*									      *
*   Description:    Convert a cluster number to an Absolute sector number     *
*									      *
*   Notes:								      *
*									      *
*   History:								      *
*    1995/09/05 JFL Created this file					      *
*									      *
*      (c) Copyright 1994-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "msdos.h"
#include "lodos.h"

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    Cluster2Sector					      |
|									      |
|   Description:    Convert a cluster number to an absolute sector number     |
|									      |
|   Parameters:     DEVICEPARAMS *pdp	The parameters for the drive	      |
|		    WORD wCluster	The cluster number		      |
|									      |
|   Returns:	    The absolute sector number of the cluster's 1st sector    |
|                                                                             |
|   Notes:								      |
|                                                                             |
|   History:								      |
|									      |
|    1994/07/21 JFL Created this routine.				      |
|    1994/08/01 JFL The first cluster is number 2!			      |
|    1995/09/05 JFL Changed the 1st argument to DEVICEPARAMS * to prevent     |
|		     unnecessary calls to GetDeviceParameters.		      |
*									      *
\*---------------------------------------------------------------------------*/

long Cluster2Sector(DEVICEPARAMS *pdp, WORD wCluster)
    {
    long l;

    l = wCluster - 2;			// Cluster relative to cluster 2
    l *= pdp->dpSecPerClust;		// Sector number relative to cluster 2
    l += pdp->dpResSectors;		// + Reserved sectors
    l += pdp->dpFATs * pdp->dpFATsecs;	// + FATs sectors
    l += (pdp->dpRootDirEnts * sizeof(DIRENTRY)) / pdp->dpBytesPerSec;
					// + Root directory sectors
    return l;
    }
