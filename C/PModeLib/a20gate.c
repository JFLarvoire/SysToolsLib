/*****************************************************************************\
*                                                                             *
*   File name:	    A20GATE.C						      *
*									      *
*   Description:    High level A20 Gate management routines		      *
*									      *
*   Notes:	    Under DOS 5 and later, if DOS is loaded high, the very    *
*		    fact of calling GetXMSAddress reenables A20 automatically.*
*		    A20 remains enabled until the program terminates. In this *
*		    case, xms_enable_a20 and xms_disable_a20 are no-ops.      *
*									      *
*   History:								      *
*    1995/02/08 JFL Created this file					      *
*									      *
*      (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include "clibdef.h"		/* Make sure our implementation matches the
				     definition there */
#include "pmode.h"

//+--------------------------------------------------------------------------
//+ Function   : enable_a20
//+
//+ Purpose    : Enable the A20 line for extended memory access
//+
//+ Ouput      : 0 = Failure
//+		 1 = Success
//+
//+ Notes:     : Uses the XMS memory manager (HIMEM.SYS) if present.
//+
//+ Creation   : 1995/02/08 by Jean-Francois LARVOIRE
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+
//+--------------------------------------------------------------------------

int enable_a20(void)
{
    if (!wlpXMSValid) GetXMSAddress();

    if (lpXMS)
	return xms_enable_a20();
    else
	return !isa_enable_a20();
}

//+--------------------------------------------------------------------------
//+ Function   : disable_a20
//+
//+ Purpose    : Disable the A20 line after using extended memory
//+
//+ Ouput      : 0 = Failure
//+		 1 = Success
//+
//+ Notes:     : Uses the XMS memory manager (HIMEM.SYS) if present.
//+		 In this case, the A20 is not actually disabled, but only
//+		 restored in the state it was before the call to enable_a20.
//+
//+ Creation   : 1995/02/08 by Jean-Francois LARVOIRE
//+
//+ Modification History:
//+ Date         Author  Description
//+ -----------  ------  ----------------------------------------------------
//+
//+--------------------------------------------------------------------------

int disable_a20(void)
{
    if (!wlpXMSValid) GetXMSAddress();

    if (lpXMS)
	return xms_disable_a20();
    else
	return !isa_disable_a20();
}
