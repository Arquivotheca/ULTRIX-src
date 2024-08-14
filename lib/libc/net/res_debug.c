#ifndef lint
static	char	*sccsid = "@(#)res_debug.c	4.1	(ULTRIX)	7/3/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1987-1990 by			*
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
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
static char sccsid[] = "@(#)res_debug.c	5.22 (Berkeley) 3/7/88";
 */
/*
 * Modification History:
 *
 * 07-Feb-90	sue
 *	Incorporated Hesiod changes from MIT to implement multiple
 *	"character strings" in a TXT record according to RFC1035.
 */

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/nameser.h>

extern char *p_cdname(), *p_rr(), *p_type(), *p_class();
extern char *inet_ntoa();

char *_res_opcodes[] = {
	"QUERY",
	"IQUERY",
	"CQUERYM",
	"CQUERYU",
	"4",
	"5",
	"6",
	"7",
	"8",
	"UPDATEA",
	"UPDATED",
	"UPDATEDA",
	"UPDATEM",
	"UPDATEMA",
	"ZONEINIT",
	"ZONEREF",
};

char *_res_resultcodes[] = {
	"NOERROR",
	"FORMERR",
	"SERVFAIL",
	"NXDOMAIN",
	"NOTIMP",
	"REFUSED",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"NOCHANGE",
};

p_query(msg)
	char *msg;
{
#ifdef DEBUG
	fp_query(msg,stdout);
#endif
}

/*
 * Print the contents of a query.
 * This is intended to be primarily a debugging routine.
 */
fp_query(msg,file)
	char *msg;
	FILE *file;
{
#ifdef DEBUG
	register char *cp;
	register HEADER *hp;
	register int n;

	/*
	 * Print header fields.
	 */
	hp = (HEADER *)msg;
	cp = msg + sizeof(HEADER);
	fprintf(file,"HEADER:\n");
	fprintf(file,"\topcode = %s", _res_opcodes[hp->opcode]);
	fprintf(file,", id = %d", ntohs(hp->id));
	fprintf(file,", rcode = %s\n", _res_resultcodes[hp->rcode]);
	fprintf(file,"\theader flags: ");
	if (hp->qr)
		fprintf(file," qr");
	if (hp->aa)
		fprintf(file," aa");
	if (hp->tc)
		fprintf(file," tc");
	if (hp->rd)
		fprintf(file," rd");
	if (hp->ra)
		fprintf(file," ra");
	if (hp->pr)
		fprintf(file," pr");
	fprintf(file,"\n\tqdcount = %d", ntohs(hp->qdcount));
	fprintf(file,", ancount = %d", ntohs(hp->ancount));
	fprintf(file,", nscount = %d", ntohs(hp->nscount));
	fprintf(file,", arcount = %d\n\n", ntohs(hp->arcount));
	/*
	 * Print question records.
	 */
	if (n = ntohs(hp->qdcount)) {
		fprintf(file,"QUESTIONS:\n");
		while (--n >= 0) {
			fprintf(file,"\t");
			cp = p_cdname(cp, msg, file);
			if (cp == NULL)
				return;
			fprintf(file,", type = %s", p_type(_getshort(cp)));
			cp += sizeof(u_short);
			fprintf(file,", class = %s\n\n", p_class(_getshort(cp)));
			cp += sizeof(u_short);
		}
	}
	/*
	 * Print authoritative answer records
	 */
	if (n = ntohs(hp->ancount)) {
		fprintf(file,"ANSWERS:\n");
		while (--n >= 0) {
			fprintf(file,"\t");
			cp = p_rr(cp, msg, file);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print name server records
	 */
	if (n = ntohs(hp->nscount)) {
		fprintf(file,"NAME SERVERS:\n");
		while (--n >= 0) {
			fprintf(file,"\t");
			cp = p_rr(cp, msg, file);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print additional records
	 */
	if (n = ntohs(hp->arcount)) {
		fprintf(file,"ADDITIONAL RECORDS:\n");
		while (--n >= 0) {
			fprintf(file,"\t");
			cp = p_rr(cp, msg, file);
			if (cp == NULL)
				return;
		}
	}
#endif
}

char *
p_cdname(cp, msg, file)
	char *cp, *msg;
	FILE *file;
{
#ifdef DEBUG
	char name[MAXDNAME];
	int n;

	if ((n = dn_expand(msg, msg + 512, cp, name, sizeof(name))) < 0)
		return (NULL);
	if (name[0] == '\0') {
		name[0] = '.';
		name[1] = '\0';
	}
	fputs(name, file);
	return (cp + n);
#endif
}

/*
 * Print resource record fields in human readable form.
 */
char *
p_rr(cp, msg, file)
	char *cp, *msg;
	FILE *file;
{
#ifdef DEBUG
	int type, class, dlen, n, c;
	struct in_addr inaddr;
	char *cp1;

	if ((cp = p_cdname(cp, msg, file)) == NULL)
		return (NULL);			/* compression error */
	fprintf(file,"\n\ttype = %s", p_type(type = _getshort(cp)));
	cp += sizeof(u_short);
	fprintf(file,", class = %s", p_class(class = _getshort(cp)));
	cp += sizeof(u_short);
	fprintf(file,", ttl = %u", _getlong(cp));
	cp += sizeof(u_long);
	fprintf(file,", dlen = %d\n", dlen = _getshort(cp));
	cp += sizeof(u_short);
	cp1 = cp;
	/*
	 * Print type specific data, if appropriate
	 */
	switch (type) {
	case T_A:
		switch (class) {
		case C_IN:
			bcopy(cp, (char *)&inaddr, sizeof(inaddr));
			if (dlen == 4) {
				fprintf(file,"\tinternet address = %s\n",
					inet_ntoa(inaddr));
				cp += dlen;
			} else if (dlen == 7) {
				fprintf(file,"\tinternet address = %s",
					inet_ntoa(inaddr));
				fprintf(file,", protocol = %d", cp[4]);
				fprintf(file,", port = %d\n",
					(cp[5] << 8) + cp[6]);
				cp += dlen;
			}
			break;
		default:
			cp += dlen;
		}
		break;
	case T_CNAME:
	case T_MB:
#ifdef OLDRR
	case T_MD:
	case T_MF:
#endif /* OLDRR */
	case T_MG:
	case T_MR:
	case T_NS:
	case T_PTR:
		fprintf(file,"\tdomain name = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file,"\n");
		break;

	case T_HINFO:
		if (n = *cp++) {
			fprintf(file,"\tCPU=%.*s\n", n, cp);
			cp += n;
		}
		if (n = *cp++) {
			fprintf(file,"\tOS=%.*s\n", n, cp);
			cp += n;
		}
		break;

	case T_SOA:
		fprintf(file,"\torigin = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file,"\n\tmail addr = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file,"\n\tserial=%ld", _getlong(cp));
		cp += sizeof(u_long);
		fprintf(file,", refresh=%ld", _getlong(cp));
		cp += sizeof(u_long);
		fprintf(file,", retry=%ld", _getlong(cp));
		cp += sizeof(u_long);
		fprintf(file,", expire=%ld", _getlong(cp));
		cp += sizeof(u_long);
		fprintf(file,", min=%ld\n", _getlong(cp));
		cp += sizeof(u_long);
		break;

	case T_MX:
		fprintf(file,"\tpreference = %ld,",_getshort(cp));
		cp += sizeof(u_short);
		fprintf(file," name = ");
		cp = p_cdname(cp, msg, file);
		break;

	case T_TXT:
		(void) fputs("\t\"", file);
		while (cp < cp1 + dlen) {
			if (n = (unsigned char) *cp++) {
				for (c = n; c > 0; c--) {
					if (*cp == '\n')
						(void) putc('\\', file);
					(void) putc(*cp++, file);
				}
			}
		}
		(void) fputs("\"\n", file);
		break;

	case T_MINFO:
		fprintf(file,"\trequests = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file,"\n\terrors = ");
		cp = p_cdname(cp, msg, file);
		break;

	case T_UINFO:
		fprintf(file,"\t%s\n", cp);
		cp += dlen;
		break;

	case T_UID:
	case T_GID:
		if (dlen == 4) {
			fprintf(file,"\t%ld\n", _getlong(cp));
			cp += sizeof(int);
		}
		break;

	case T_WKS:
		if (dlen < sizeof(u_long) + 1)
			break;
		bcopy(cp, (char *)&inaddr, sizeof(inaddr));
		cp += sizeof(u_long);
		fprintf(file,"\tinternet address = %s, protocol = %d\n\t",
			inet_ntoa(inaddr), *cp++);
		n = 0;
		while (cp < cp1 + dlen) {
			c = *cp++;
			do {
 				if (c & 0200)
					fprintf(file," %d", n);
 				c <<= 1;
			} while (++n & 07);
		}
		putc('\n',file);
		break;

#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		{
			int NumBytes = 8;
			char *DataPtr;
			int i;

			if (dlen < NumBytes) NumBytes = dlen;
			fprintf(file, "\tFirst %d bytes of hex data:",
				NumBytes);
			for (i = 0, DataPtr = cp; i < NumBytes; i++, DataPtr++)
				fprintf(file, " %x", *DataPtr);
			fputs("\n", file);
			cp += dlen;
		}
		break;
#endif /* ALLOW_T_UNSPEC */

	default:
		fprintf(file,"\t???\n");
		cp += dlen;
	}
	if (cp != cp1 + dlen)
		fprintf(file,"packet size error (%#x != %#x)\n", cp, cp1+dlen);
	fprintf(file,"\n");
	return (cp);
#endif
}

static	char nbuf[20];

/*
 * Return a string for the type
 */
char *
p_type(type)
	int type;
{
	switch (type) {
	case T_A:
		return("A");
	case T_NS:		/* authoritative server */
		return("NS");
#ifdef OLDRR
	case T_MD:		/* mail destination */
		return("MD");
	case T_MF:		/* mail forwarder */
		return("MF");
#endif /* OLDRR */
	case T_CNAME:		/* canonical name */
		return("CNAME");
	case T_SOA:		/* start of authority zone */
		return("SOA");
	case T_MB:		/* mailbox domain name */
		return("MB");
	case T_MG:		/* mail group member */
		return("MG");
	case T_MX:		/* mail routing info */
		return("MX");
	case T_TXT:		/* text record */
		return("TXT");
	case T_MR:		/* mail rename name */
		return("MR");
	case T_NULL:		/* null resource record */
		return("NULL");
	case T_WKS:		/* well known service */
		return("WKS");
	case T_PTR:		/* domain name pointer */
		return("PTR");
	case T_HINFO:		/* host information */
		return("HINFO");
	case T_MINFO:		/* mailbox information */
		return("MINFO");
	case T_AXFR:		/* zone transfer */
		return("AXFR");
	case T_MAILB:		/* mail box */
		return("MAILB");
	case T_MAILA:		/* mail address */
		return("MAILA");
	case T_ANY:		/* matches any type */
		return("ANY");
	case T_UINFO:
		return("UINFO");
	case T_UID:
		return("UID");
	case T_GID:
		return("GID");
#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		return("UNSPEC");
#endif /* ALLOW_T_UNSPEC */
	default:
		(void)sprintf(nbuf, "%d", type);
		return(nbuf);
	}
}

/*
 * Return a mnemonic for class
 */
char *
p_class(class)
	int class;
{

	switch (class) {
	case C_IN:		/* internet class */
		return("IN");
	case C_HS:
		return("HS");	/* Hesiod class */
	case C_ANY:		/* matches any class */
		return("ANY");
	default:
		(void)sprintf(nbuf, "%d", class);
		return(nbuf);
	}
}
