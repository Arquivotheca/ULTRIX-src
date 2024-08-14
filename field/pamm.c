
#ifndef lint
static	char	*sccsid = "@(#)pamm.c	1.1	(ULTRIX)	4/10/86";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *		  PAMM.C
 *
 *	SNAPSHOT PAMM Display Routine
 */

#include <stdio.h>

#define ZERO 0
#define VALID_RECORD 1
#define TRUE 1
#define FALSE 0

extern bytes_used;
extern rec_length;


void pamm_print()
{
	int	b,i,skip;


	rec_length = ZERO;
	skip = FALSE;

	if (getbyte() != VALID_RECORD)
	   {
	    printf("PAMM Record is Invalid..... Skiping \n");
	    skip = TRUE;
	   }

	b = getbyte();
	rec_length = (getbyte() << 8) + b;

	if (skip == FALSE)
	{
	  skip_bytes(6);  /* skip over .CDF file name */

	  printf("\n\n\tPAMM\n");

	  b = ZERO;
	  for (b = 0; b < (1024 - 0x20 + 1); (b = b + 0x20))
	  {
	      printf("\n%03x  ",b);
	      for (i = 0; i < 8; i++)
		 {
		   pnt_longword();
		   printf(" ");
		 }
	  }
	  printf("\n");
	}

	skip_to_end_of_rec();

}
