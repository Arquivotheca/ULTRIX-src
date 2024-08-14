/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1990 by                           *
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
/* ------------------------------------------------------------------ */
/* | Copyright Unpublished, MIPS Computer Systems, Inc.  All Rights | */
/* | Reserved.  This software contains proprietary and confidential | */
/* | information of MIPS and its suppliers.  Use, disclosure or     | */
/* | reproduction is prohibited without the prior express written   | */
/* | consent of MIPS.                                               | */
/* ------------------------------------------------------------------ */

 /*
  * Modification History:
  *
  *     
  * 	12-May-1990	Paul Grist
  *		Created this file, and the lw byte swap routine for
  *          	user level VME support.  
  */

#include <asm.h>
#include <regdef.h>

/***************************************************************************
 *
 *   swap_lw_bytes(buffer);
 *   unsigned long buffer;
 *
 *   RETURNS: resulting byte swapped long
 *
 ***************************************************************************
 *
 *   swap long word bytes: will operate on one unsigned 32 bit quantity
 *
 *
 *  			31               0
 *			 +---+---+---+---+
 *	start with:	 | a | b | c | d |
 *			 +---+---+---+---+
 *
 *			31               0
 *	end with:        +---+---+---+---+
 *			 | d | c | b | a |
 *			 +---+---+---+---+
 *
 ***************************************************************************/


LEAF(swap_lw_bytes)
#ifdef MIPSEL
	# a0 has the input, unsigned 32 bit quanity - abcd
	sll	v0,a0,24	# shift left 24     - d000  in v0
	srl     v1,a0,24   	# shift right 24    - 000a  in v1
	or	v0,v1 		# v0 <- v0 | v1     - d00a  in v0  *
	srl	t0,a0,8         # shift right 8     - 0abc  in t0
	and     t1,t0,0xff00    # mask out b        - 00b0  in t1
	and 	v1,t0,0xff      # mask out c        - 000c  in v1
	sll	v1,16		# shift left 16     - 0c00  in v1
	or	v1,t1		# v1 <- v1 | t1     - 0cb0  in v1  *
	or 	v0,v1		# v0 <- v0 | v1     - dcba  in v0
#else
	move	v0,a0		# move v0 into ret value
#endif
	j	ra		# return
.end swap_lw_bytes

