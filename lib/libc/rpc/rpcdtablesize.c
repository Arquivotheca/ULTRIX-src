#ifndef lint
static char *sccsid = "@(#)rpcdtablesize.c	4.1      ULTRIX  2/28/91";
#endif lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1986 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   This software is  derived  from  software  received  from  the     *
 *   University    of   California,   Berkeley,   and   from   Bell     *
 *   Laboratories.  Use, duplication, or disclosure is  subject  to     *
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.                                          *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by  Digital    *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/
                                                                           
/*
 *      Portions of this software have been licensed to
 *      Digital Equipment Company, Maynard, MA.
 *      Copyright (c) 1986 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */
                                                                             
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * 1.3 88/02/08 
 * Used to be in file: rpc_dtablesize.c
 */


/*
 * Cache the result of getdtablesize(), so we don't have to do an
 * expensive system call every time.
 */
_rpc_dtablesize()
{
	static int size;
	
	if (size == 0) {
		size = getdtablesize();
	}
	return (size);
}
