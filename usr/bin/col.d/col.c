#ifndef lint
static	char	*sccsid = "@(#)col.c	4.1	(ULTRIX)  7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/*static char *sccsid = "@(#)col.c	4.1 (Berkeley) 10/1/80";*/
# include <stdio.h>
# define PL 256
# define ESC '\033'
# define RLF '\013'
# define SI '\017'
# define SO '\016'
# define GREEK 0200
# define LINELN 800

char *page[PL];
char lbuff [LINELN], *line;
int bflag, hflag, fflag, pflag;
int half;
int cp, lp;
int ll, llh, mustwr;
int pcp = 0;
char *pgmname;
char	*strcpy();

main (argc, argv)
	int argc; char **argv;
{
	int i;
	int greek;
	register int c;

	pgmname = argv[0];

	for (i = 1; i < argc; i++) {
		register char *p;
		if (*argv[i] != '-') {
			fprintf (stderr, "%s: bad option %s\n",
				pgmname, argv[i]);
			exit (2);
		}
		for (p = argv[i]+1; *p; p++) {
			switch (*p) {
			case 'b':
				bflag++;
				break;

			case 'h':
				hflag++;
				break;

			case 'p':
				pflag++;
				break;

			case 'f':
				fflag++;
				break;

			case 'x':	/* map to hflag on Ultrix */
				hflag++;
				break;

			default:
				fprintf (stderr, "%s: bad option letter %c\n",
					pgmname, *p);
				exit (2);
			}
		}
	}

	for (ll=0; ll<PL; ll++)
		page[ll] = 0;

	cp = 0;
	ll = 0;
	greek = 0;
	mustwr = PL;
	line = lbuff;

	while ((c = getchar()) != EOF) {
		switch (c) {
		case '\n':
			incr();
			incr();
			cp = 0;
			continue;

		case '\0':
			continue;

		case ESC:
			c = getchar();
			switch (c) {
			case '7':	/* reverse full line feed */
				decr();
				decr();
				break;

			case '8':	/* reverse half line feed */
				if (fflag)
					decr();
				else {
					if (--half < -1) {
						decr();
						decr();
						half += 2;
					}
				}
				break;

			case '9':	/* forward half line feed */
				if (fflag)
					incr();
				else {
					if (++half > 0) {
						incr();
						incr();
						half -= 2;
					}
				}
				break;
			}
			continue;

		case SO:
			greek = GREEK;
			continue;

		case SI:
			greek = 0;
			continue;

		case RLF:
			decr();
			decr();
			continue;

		case '\r':
			cp = 0;
			continue;

		case '\t':
			cp = (cp + 8) & -8;
			continue;

		case '\b':
			if (cp > 0)
				cp--;
			continue;

		case ' ':
			cp++;
			continue;

		default:
			if (pflag) {
				outc(ESC);
				cp++;
				outc(c);
				cp++;
			}
			else  {
			c &= 0177;
			if (c > 040 && c < 0177) {	/* if printable */
				outc(c | greek);
				cp++; }
			}
			continue;
		}
	}

	for (i=0; i<PL; i++)
		if (page[(mustwr+i)%PL] != 0)
			emit (page[(mustwr+i) % PL], mustwr+i-PL);
	emit (" ", (llh + 1) & -2);
	exit(0);
}

outc (c)
	register char c;
{
	if (lp > cp) {
		line = lbuff;
		lp = 0;
	}

	while (lp < cp) {
		switch (*line) {
		case '\0':
			*line = ' ';
			lp++;
			break;

		case '\b':
			lp--;
			break;

		default:
			lp++;
		}
		line++;
	}
	while (*line == '\b') {
		line += 2;
	}
	if (bflag || *line == '\0' || *line == ' ')
		*line = c;
	else {
		register char c1, c2, c3;
		c1 = *++line;
		*line++ = '\b';
		c2 = *line;
		*line++ = c;
		while (c1) {
			c3 = *line;
			*line++ = c1;
			c1 = c2;
			c2 = c3;
		}
		lp = 0;
		line = lbuff;
	}
}

store (lno)
{
	char *malloc();

	lno %= PL;
	if (page[lno] != 0)
		free (page[lno]);
	page[lno] = malloc((unsigned)strlen(lbuff) + 2);
	if (page[lno] == 0) {
		fprintf (stderr, "%s: no storage\n", pgmname);
		exit (2);
	}
	strcpy (page[lno],lbuff);
}

fetch(lno)
{
	register char *p;

	lno %= PL;
	p = lbuff;
	while (*p)
		*p++ = '\0';
	line = lbuff;
	lp = 0;
	if (page[lno])
		strcpy (line, page[lno]);
}
emit (s, lineno)
	char *s;
	int lineno;
{
	static int cline = 0;
	register int ncp;
	register char *p;
	static int gflag = 0;

	if (*s) {
		while (cline < lineno - 1) {
			putchar ('\n');
			pcp = 0;
			cline += 2;
		}
		if (cline != lineno) {
			putchar (ESC);
			putchar ('9');
			cline++;
		}
		if (pcp)
			putchar ('\r');
		pcp = 0;
		p = s;
		while (*p) {
			ncp = pcp;
			while (*p++ == ' ') {
				if ((++ncp & 7) == 0 && hflag) {
					pcp = ncp;
					putchar ('\t');
				}
			}
			if (!*--p)
				break;
			while (pcp < ncp) {
				putchar (' ');
				pcp++;
			}
			if (gflag != (*p & GREEK) && *p != '\b') {
				if (gflag)
					putchar (SI);
				else
					putchar (SO);
				gflag ^= GREEK;
			}
			putchar (*p & ~GREEK);
			if (*p++ == '\b')
				pcp--;
			else
				pcp++;
		}
	}
}

incr()
{
	store (ll++);
	if (ll > llh)
		llh = ll;
	if (ll >= mustwr && page[ll%PL]) {
		emit (page[ll%PL], ll - PL);
		mustwr++;
		free (page[ll%PL]);
		page[ll%PL] = 0;
	}
	fetch (ll);
}

decr()
{
	if (ll > mustwr - PL) {
		store (ll--);
		fetch (ll);
	}
}
