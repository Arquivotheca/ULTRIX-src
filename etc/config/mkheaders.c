#ifndef lint
static char *sccsid = "@(#)mkheaders.c	1.7	ULTRIX	10/3/86";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1984,86 by			*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*-----------------------------------------------------------------------
 *
 * Modification History
 *
 * 11-Mar-86 -- jrs
 *	Fix problem with handing count specifier on bus command
 *
 * 06-Mar-86 -- jrs
 *	Fix up problems caused by Berekely using hard coded values
 *	instead of header file constants.
 *
 * 25-Feb-86 -- jrs
 *	Changed to allow multiple "needs" files per files.* line
 *
 *-----------------------------------------------------------------------
 */

/*
 * Make all the .h files for the optional entries
 */

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "y.tab.h"

headers()
{
	register struct file_list *fl;
	int needndx;

	for (fl = ftab; fl != 0; fl = fl->f_next)
		for (needndx = 0; needndx < NNEEDS; needndx++) {
			if (fl->f_needs[needndx] == NULL) {
				break;
			}
			do_count(fl->f_needs[needndx], fl->f_needs[needndx],
						1);
		}
}

/*
 * count all the devices of a certain type and recurse to count
 * whatever the device is connected to
 */
do_count(dev, hname, search)
	register char *dev, *hname;
	int search;
{
	register struct device *dp, *mp;
	register int count;

	for (count = 0,dp = dtab; dp != 0; dp = dp->d_next)
		if (dp->d_unit != -1 && eq(dp->d_name, dev)) {
			if (dp->d_type == PSEUDO_DEVICE || dp->d_type == BUS) {
				count =
				    dp->d_slave != UNKNOWN ? dp->d_slave : 1;
				break;
			}
			count++;
			/*
			 * Allow holes in unit numbering,
			 * assumption is unit numbering starts
			 * at zero.
			 */
			if (dp->d_unit + 1 > count)
				count = dp->d_unit + 1;
			if (search) {
				mp = dp->d_conn;
				if (mp != 0 && mp != TO_NEXUS && 
				    mp->d_conn != TO_NEXUS) {
					do_count(mp->d_name, hname, 0);
					search = 0;
				}
			}
		}
	do_header(dev, hname, count);
}

do_header(dev, hname, count)
	char *dev, *hname;
	int count;
{
	char *file, *name, *inw, *toheader(), *tomacro();
	struct file_list *fl, *fl_head;
	FILE *inf, *outf;
	int inc, oldcount;

	file = toheader(hname);
	name = tomacro(dev);
	
	inf = fopen(file, "r");
	oldcount = -1;
	if (inf == 0) {
		outf = fopen(file, "w");
		if (outf == 0) {
			perror(file);
			exit(1);
		}
		fprintf(outf, "#define %s %d\n", name, count);
		(void) fclose(outf);
		return;
	}
	fl_head = 0;
	for (;;) {
		char *cp;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		inw = ns(inw);
		cp = get_word(inf);
		if (cp == 0 || cp == (char *)EOF)
			break;
		inc = atoi(cp);
		if (eq(inw, name)) {
			oldcount = inc;
			inc = count;
		}
		cp = get_word(inf);
		if (cp == (char *)EOF)
			break;
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = inw;
		fl->f_type = inc;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	(void) fclose(inf);
	if (count == oldcount) {
		for (fl = fl_head; fl != 0; fl = fl->f_next)
			free((char *)fl);
		return;
	}
	if (oldcount == -1) {
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = name;
		fl->f_type = count;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	outf = fopen(file, "w");
	if (outf == 0) {
		perror(file);
		exit(1);
	}
	for (fl = fl_head; fl != 0; fl = fl->f_next) {
		fprintf(outf, "#define %s %d\n",
		    fl->f_fn, count ? fl->f_type : 0);
		free((char *)fl);
	}
	(void) fclose(outf);
}

/*
 * convert a dev name to a .h file name
 */
char *
toheader(dev)
	char *dev;
{
	static char hbuf[80];

	(void) strcpy(hbuf, path(dev));
	(void) strcat(hbuf, ".h");
	return (hbuf);
}

/*
 * convert a dev name to a macro name
 */
char *tomacro(dev)
	register char *dev;
{
	static char mbuf[20];
	register char *cp;

	cp = mbuf;
	*cp++ = 'N';
	while (*dev)
		*cp++ = toupper(*dev++);
	*cp++ = 0;
	return (mbuf);
}
