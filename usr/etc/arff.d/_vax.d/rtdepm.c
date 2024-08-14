#ifndef lint
static  char    *sccsid = "@(#)rtdepm.c	4.1  (ULTRIX)        7/2/90";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1984 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/


/*
FACILITY:

    RT-11 volume manipulation.

ABSTRACT:

    Manipulates directory entry pointers.

ENVIRONMENT:

    PRO/VENIX user mode.
    ULTRIX-11 user mode.
    ULTRIX-32 user mode.

AUTHOR: Brian Hetrick, CREATION DATE: 1 March 1985.

MODIFIED BY:

	Brian Hetrick, 01-Mar-85: Version 1.0
  000 - Original version of module.

*/

/*
 * INCLUDE FILES:
 */

#include "arff.h"

/*
 * TABLE OF CONTENTS:
 */

/*
 * MACROS:
 */

/*
 * EQUATED SYMBOLS:
 */

/*
 * OWN STORAGE:
 */

/*
 * EXTERNAL REFERENCES:
 */

rtidep (ds_ptr, de_ptr)

struct dirseg
    * ds_ptr;

struct dirent
    * * de_ptr;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Increments a directory entry pointer.

FORMAL PARAMETERS:

    Directory_segment.rr.r - The  directory  segment  into  which  the
	directory entry pointer points.
    Directory_entry_pointer.ma.r - The directory entry pointer  to  be
	incremented.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    /*
     *  The following expression takes care of the extra words in each
     *  directory entry, as described in the directory segment header.
     */

    * de_ptr = (struct dirent *)
	(((char *) ((* de_ptr) + 1)) +
	 sizeof (word) * ds_ptr -> ds_xtra);
}

rtddep (ds_ptr, de_ptr)

struct dirseg
    * ds_ptr;

struct dirent
    * * de_ptr;

/*********************************************************************

FUNCTIONAL DESCRIPTION:

    Decrements a directory entry pointer.

FORMAL PARAMETERS:

    Directory_segment.rr.r - The  directory  segment  into  which  the
	directory entry pointer points.
    Directory_entry_pointer.ma.r - The directory entry pointer  to  be
	decremented.

IMPLICIT INPUTS:

    None.

IMPLICIT OUTPUTS:

    None.

ROUTINE VALUE:

    None.

SIDE EFFECTS:

    None.

*********************************************************************/

{
    /*
     *  The following expression takes care of the extra words in each
     *  directory entry, as described in the directory segment header.
     */

    * de_ptr = (struct dirent *)
	(((char *) ((* de_ptr) - 1)) -
	 sizeof (word) * ds_ptr -> ds_xtra);
}
