
#ifndef lint
static char *sccsid = "@(#)dz.c	1.26	ULTRIX	2/12/87";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985, 1986 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * dz.c  6.1   7/29/83
 *
 * Modification history
 *
 * DZ11/DZ32/DZV11/DZQ11 terminal driver
 *
 * 18-Mar-86 - jaw
 *
 *	Derived from 4.2BSD labeled: dz.c      6.1     83/07/29.
 *	br/cvec changed to NOT use registers.
 *
 * 26-Apr-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 * 13-Jun-86 - jaw
 *
 *	Fix to uba reset and drivers.
 *
 * 30-Jun-86 - Tim Burke
 *
 *	Bug fix to TIOCMODEM looks first to see if device type is DZ32
 *	before examining carrier detect.
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 * 15-Dec-86 - Tim Burke
 *
 *	When a break occurs, (interpreted as a framing error) set the variable
 *	c to be the interrupt character.  There was a problem here due to the
 *	fact that sign extension is done which causes unwanted side affects. To
 *	solve this bug, the character is stripped to 8 bits.
 *
 *	Modified probe routine to wait for self test to complete.
 *
 *	Fix DEVIOGET to return propper modem status information.
 *
 *  6-Feb-87 - Tim Burke
 *
 *	Removed printf of master reset failure in probe routine, as it may be
 *	incorrectly appearing. (Particularly in the DMF & DMZ drivers)
 */

#include "dz.h"
#if NDZ > 0 || defined(BINARY)

#include "../data/dz_data.c"

/*
* Driver information for auto-configuration stuff.
*/
int	dzprobe(), dzattach(), dzrint();
u_short dzstd[] = { 0 };
struct	uba_driver dzdriver =
{ dzprobe, 0, dzattach, 0, dzstd, "dz", dzinfo };


int	dzstart(), dzxint(), dzdma();
int	ttrstrt();
int	dzact;

#define dzwait(x)	while (((x)->dzlcs & DZ_ACK) == 0)
#define FASTTIMER	(hz/30) 	/* rate to drain silos, when in use */

int	dzsilos;			/* mask of dz's with silo in use */
int	dztimerintvl;			/* time interval for dztimer */
int	dzhighrate = 100;		/* silo on if dzchars > dzhighrate */
int	dzlowrate = 75; 		/* silo off if dzrate < dzlowrate */

/*
* The dz11 doesn't interrupt on carrier transitions, so
* we have to use a timer to watch it.
*/
char	dz_timer;		/* timer started? */

char	dz_speeds[] =
{ 0,020,021,022,023,024,0,025,026,027,030,032,034,036,037,0 };

#ifndef PORTSELECTOR
#define ISPEED	B300
#define IFLAGS	(EVENP|ODDP|ECHO)
#else
#define ISPEED	B4800
#define IFLAGS	(EVENP|ODDP)
#endif

dzprobe(reg)
	caddr_t reg;
{
	register struct dzdevice *dzaddr = (struct dzdevice *)reg;
	register int totaldelay;

#ifdef lint
	dzrint(0); dzxint((struct tty *)0);
#endif
	dzaddr->dzcsr = DZ_TIE|DZ_MSE|DZ_32;
	if (dzaddr->dzcsr & DZ_32)
		dzaddr->dzlnen = 1;
	else
		dzaddr->dztcr = 1;		/* enable any line */
	DELAY(100000);
	dzaddr->dzcsr = DZ_CLR|DZ_32;		/* reset everything */
	/*
	 * If a self test is not being done, start one up.  Wait for the
	 * self-test (to a max of 4 sec.) to complete before interrupting.
	 */

	if ((dzaddr->dzcsr & DZ_CLR) == 0)
	    dzaddr->dzcsr |= DZ_CLR;
	totaldelay = 0;
	while ( (dzaddr->dzcsr & DZ_CLR) && ( totaldelay <= 70) ){
	    totaldelay++;
	    DELAY(50000);
	}
	/*
	 * This message may be incorrectly printed - particularly in the
	 * DMF & DMZ drivers.
	if (dzaddr->dzcsr & DZ_CLR)
	    printf("Warning: DZ device failed to exit self-test\n");
	 */

	if (cvec && cvec != 0x200)
		cvec -= 4;
	return (sizeof (struct dzdevice));
}

dzattach(ui)
	register struct uba_device *ui;
{
	register struct pdma *pdp = &dzpdma[ui->ui_unit*8];
	register struct tty *tp = &dz_tty[ui->ui_unit*8];
	register int cntr;
	extern dzscan();

	for (cntr = 0; cntr < 8; cntr++) {
		pdp->p_addr = (struct dzdevice *)ui->ui_addr;
		pdp->p_arg = (int)tp;
		pdp->p_fcn = dzxint;
		pdp++, tp++;
	}
	dzsoftCAR[ui->ui_unit] = ui->ui_flags;
	dzdefaultCAR[ui->ui_unit] = ui->ui_flags;
	if (dz_timer == 0) {
		dz_timer++;
		timeout(dzscan, (caddr_t)0, hz);
		dztimerintvl = FASTTIMER;
	}
}

/*ARGSUSED*/
dzopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	int inuse;  /*hold state of inuse bit while blocked waiting for carr*/

	unit = minor(dev);
	if (unit >= dz_cnt || dzpdma[unit].p_addr == 0)
		return (ENXIO);
	tp = &dz_tty[unit];
	while (tp->t_state&TS_CLOSING) /* let DTR stay down for awhile */
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	tp->t_addr = (caddr_t)&dzpdma[unit];
	tp->t_oproc = dzstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
#ifndef PORTSELECTOR
		if (tp->t_ispeed == 0) {
#endif
			tp->t_ospeed = tp->t_ispeed = ISPEED;
			tp->t_flags = IFLAGS;
#ifndef PORTSELECTOR
		}
#endif
		/* tp->t_state |= TS_HUPCLS; */
		dzparam(unit);
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0){
		return (EBUSY);
	}
	(void) dzmctl(dev, DZ_ON, DMSET);
	(void) spl5();
	if (flag & O_NDELAY)
		tp->t_state |= TS_ONDELAY;
	else
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			inuse = tp->t_state&TS_INUSE;
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
			/* if we opened "block if in use"  and
			 *  the terminal was not inuse at that time
			 *  but is became "in use" while we were
			 *  waiting for carrier then return
			 */
			if ((flag & O_BLKINUSE) && (inuse==0) &&
				(tp->t_state&TS_INUSE)) {
					(void) spl0();
					return(EALREADY);
			}
		}
	(void) spl0();
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
dzclose(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	register struct dzdevice *dzaddr;
	int dz;

	unit = minor(dev);
	dz = unit >> 3;
	tp = &dz_tty[unit];
	(*linesw[tp->t_line].l_close)(tp);
	dzaddr = dzpdma[unit].p_addr;
	if (dzaddr->dzcsr&DZ_32)
		(void) dzmctl(dev, DZ_BRK, DMBIC);
	else
		dzaddr->dzbrk = (dz_brk[dz] &= ~(1 << (unit&07)));
	if ((tp->t_state&(TS_HUPCLS|TS_WOPEN)) || (tp->t_state&TS_ISOPEN)==0) {
		(void) dzmctl(dev, DZ_OFF, DMSET);
		if ((dzsoftCAR[dz] & (1<<(unit&07)))==0)  {
			/*drop DTR for at least a sec. if modem line*/
			tp->t_state |= TS_CLOSING;
			sleep((caddr_t)&lbolt, PZERO-10);
			sleep((caddr_t)&lbolt, PZERO-10);
			tp->t_state &= ~(TS_CLOSING);
			wakeup((caddr_t)&tp->t_rawq);
		}
	}
	/* reset line to default mode */
	dzsoftCAR[dz] &= ~(1<<(unit&07));
	dzsoftCAR[dz] |= (1<<(unit&07)) & dzdefaultCAR[dz];
	ttyclose(tp);
}

dzread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = &dz_tty[minor(dev)];
	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

dzwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = &dz_tty[minor(dev)];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*ARGSUSED*/
dzrint(dz)
	int dz;
{
	register struct tty *tp;
	register int c;
	register struct dzdevice *dzaddr;
	register struct tty *tp0;
	register int unit;
	int overrun = 0;

	if ((dzact & (1<<dz)) == 0)
		return;
	unit = dz * 8;
	dzaddr = dzpdma[unit].p_addr;
	tp0 = &dz_tty[unit];
	dzaddr->dzcsr &= ~(DZ_RIE|DZ_MIE);	/* the manual says this song */
	dzaddr->dzcsr |= DZ_RIE|DZ_MIE; 	/*   and dance is necessary */
	while (dzaddr->dzcsr & DZ_MSC) {	/* DZ32 modem change interrupt */
		c = dzaddr->dzmtsr;
		tp = tp0 + (c&7);
		if (tp >= &dz_tty[dz_cnt])
			break;
		dzaddr->dzlcs = c&7;	/* get status of modem lines */
		dzwait(dzaddr); 	/* wait for them */
		if (c & DZ_CD)		/* carrier status change? */
		if (dzaddr->dzlcs & DZ_CD) {	/* carrier up? */
			if ((tp->t_state&TS_CARR_ON) == 0) {
				wakeup((caddr_t)&tp->t_rawq);
				tp->t_state |= TS_CARR_ON;
				tp->t_state &= ~TS_ONDELAY;
			}
		} else {	/* no carrier */
			if (tp->t_state&TS_CARR_ON) {
				gsignal(tp->t_pgrp, SIGHUP);
				gsignal(tp->t_pgrp, SIGCONT);
				dzaddr->dzlcs = DZ_ACK|(c&7);
				ttyflush(tp, FREAD|FWRITE);
			}
			tp->t_state &= ~TS_CARR_ON;
		}
	}
	while ((c = dzaddr->dzrbuf) < 0) {	/* char present */
		dzchars[dz]++;
		tp = tp0 + ((c>>8)&07);
		if (tp >= &dz_tty[dz_cnt])
			continue;
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
			if ((tp->t_state&TS_WOPEN) == 0)
#endif
			continue;
		}
		if (c&DZ_FE)
			if (tp->t_flags & RAW)
				c = 0;
			else{
				c = tp->t_intrc;
				/*
				 * Strip extraneous sign extension bits.
				 */
				c &= 0377;
			}
		if (c&DZ_DO && overrun == 0) {
			printf("dz%d,%d: silo overflow\n", dz, (c>>8)&7); 
			overrun = 1;
		}
		if (c&DZ_PE)
			if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
			  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
				continue;
#if NHC > 0
		if (tp->t_line == HCLDISC) {
			HCINPUT(c, tp);
		} else
#endif
		(*linesw[tp->t_line].l_rint)(c, tp);
	}
}

/*ARGSUSED*/
dzioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev);
	register struct tty *tp = &dz_tty[unit];
	register int dz = unit >> 3;
	register struct dzdevice *dzaddr;
	register int s;
	struct uba_device *ui = dzinfo[dz];
	struct dz_softc *sc = &dz_softc[ui->ui_unit];
	struct devget *devget;
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
		if (cmd == TIOCSETP || cmd == TIOCSETN)
			dzparam(unit);
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		dzaddr = ((struct pdma *)(tp->t_addr))->p_addr;
		if (dzaddr->dzcsr&DZ_32)
			(void) dzmctl(dev, DZ_BRK, DMBIS);
		else
			dzaddr->dzbrk = (dz_brk[dz] |= 1 << (unit&07));
		break;

	case TIOCCBRK:
		dzaddr = ((struct pdma *)(tp->t_addr))->p_addr;
		if (dzaddr->dzcsr&DZ_32)
			(void) dzmctl(dev, DZ_BRK, DMBIC);
		else
			dzaddr->dzbrk = (dz_brk[dz] &= ~(1 << (unit&07)));
		break;

	case TIOCSDTR:
		(void) dzmctl(dev, DZ_DTR|DZ_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) dzmctl(dev, DZ_DTR|DZ_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) dzmctl(dev, dmtodz(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) dzmctl(dev, dmtodz(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) dzmctl(dev, dmtodz(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dztodm(dzmctl(dev, 0, DMGET));
		break;

	case TIOCNMODEM:  /* ignore modem status */
		s = spl5();
		dzsoftCAR[dz] |= (1<<(unit&07));
		if (*(int *)data) /* make mode permanent */
			dzdefaultCAR[dz] |= (1<<(unit&07));
		tp->t_state |= TS_CARR_ON;
		splx(s);
		break;

	case TIOCMODEM:  /* look at modem status - sleep if no carrier */
		s = spl5();
		dzsoftCAR[dz] &= ~(1<<(unit&07));
		if (*(int *)data) /* make mode permanent */
			dzdefaultCAR[dz] &= ~(1<<(unit&07));
		dzaddr = (struct dzdevice *)(dzpdma[unit].p_addr);
		if (dzaddr->dzcsr & DZ_32){
			if (dzaddr->dzlcs & DZ_CD) {
				tp->t_state &= ~TS_ONDELAY;
				tp->t_state |= TS_CARR_ON;
			}
			else
				tp->t_state &= ~(TS_CARR_ON);
		}
		else {
			if (dzaddr->dzmsr & (1<<(unit&07)) ){
				tp->t_state &= ~TS_ONDELAY;
				tp->t_state |= TS_CARR_ON;
			}
			else
				tp->t_state &= ~(TS_CARR_ON);
		}
		splx(s);
		break;

	case TIOCWONLINE:
		s = spl5();
		while ((tp->t_state & TS_CARR_ON) == 0)
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
		splx(s);
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));

		if(dzsoftCAR[dz] & (1<<(unit&07))) {
			sc->sc_category_flags[unit] |= DEV_MODEM;
			sc->sc_category_flags[unit] &= ~DEV_MODEM_ON;
		}
		else
			sc->sc_category_flags[unit] |= (DEV_MODEM|DEV_MODEM_ON);

		devget->category = DEV_TERMINAL;

		if(ui->ui_hd->uba_type & (UBAUVI|UBAUVII)) {
			devget->bus = DEV_QB;
		} else {
			devget->bus = DEV_UB;
		}

		switch (devget->bus) {

		case DEV_UB:
			dzaddr = ((struct pdma *)(tp->t_addr))->p_addr;
			if (dzaddr->dzcsr & DZ_32) {
				bcopy(DEV_DZ32,devget->interface,
				      strlen(DEV_DZ32));
			} else {
				bcopy(DEV_DZ11,devget->interface,
				      strlen(DEV_DZ11));
			}
			break;

		case DEV_QB:
			dzaddr = ((struct pdma *)(tp->t_addr))->p_addr;
			if (dzaddr->dzcsr & DZ_32) {
				bcopy(DEV_DZQ11,devget->interface,
				      strlen(DEV_DZQ11));
			} else {
				bcopy(DEV_DZV11,devget->interface,
				      strlen(DEV_DZV11));
			}
			break;
		}

		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which UBA/QB */
		devget->ctlr_num = dz;			/* which interf.*/
		devget->slave_num = unit%8;		/* which line	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "dz"	*/
		devget->unit_num = unit;		/* dz line?	*/
		devget->soft_count =
		      sc->sc_softcnt[unit];		/* soft er. cnt.*/
		devget->hard_count =
		      sc->sc_hardcnt[unit];		/* hard er cnt. */
		devget->stat = sc->sc_flags[unit];	/* status	*/
		devget->category_stat =
		      sc->sc_category_flags[unit];	/* cat. stat.	*/
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

dmtodz(bits)
	register int bits;
{
	register int b;

	b = (bits >>1) & 0370;
	if (bits & DML_ST) b |= DZ_ST;
	if (bits & DML_RTS) b |= DZ_RTS;
	if (bits & DML_DTR) b |= DZ_DTR;
	if (bits & DML_LE) b |= DZ_LE;
	return(b);
}

dztodm(bits)
	register int bits;
{
	register int b;

	b = (bits << 1) & 0360;
	if (bits & DZ_DSR) b |= DML_DSR;
	if (bits & DZ_DTR) b |= DML_DTR;
	if (bits & DZ_ST) b |= DML_ST;
	if (bits & DZ_RTS) b |= DML_RTS;
	return(b);
}

dzparam(unit)
	register int unit;
{
	register struct tty *tp;
	register struct dzdevice *dzaddr;
	register int lpr;

	tp = &dz_tty[unit];
	dzaddr = dzpdma[unit].p_addr;
	if (dzsilos & (1 << (unit >> 3)))
		dzaddr->dzcsr = DZ_IEN | DZ_SAE;
	else
		dzaddr->dzcsr = DZ_IEN;
	dzact |= (1<<(unit>>3));
	if (tp->t_ispeed == 0) {
		(void) dzmctl(unit, DZ_OFF, DMSET);	/* hang up line */
		return;
	}
	lpr = (dz_speeds[tp->t_ispeed]<<8) | (unit & 07);
	if (tp->t_flags & (RAW|LITOUT))
		lpr |= BITS8;
	else
		lpr |= (BITS7|PENABLE);
	if ((tp->t_flags & EVENP) == 0)
		lpr |= OPAR;
	if (tp->t_ispeed == B110)
		lpr |= TWOSB;
	dzaddr->dzlpr = lpr;
}

dzxint(tp)
	register struct tty *tp;
{
	register struct pdma *dp;
	register dz, unit;

	dp = (struct pdma *)tp->t_addr;

	/* it appears that the dz will ocassionally give spurious
	   transmit ready interupts even when not enabled.  If the
	   line was never opened, the following is necessary */

	if (dp == NULL) {
		tp->t_addr = (caddr_t) &dzpdma[minor(tp->t_dev)];
		dp = (struct pdma *) tp->t_addr;
	}
	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else {
		ndflush(&tp->t_outq, dp->p_mem-tp->t_outq.c_cf);
		dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	}
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		dzstart(tp);
	dz = minor(tp->t_dev) >> 3;
	unit = minor(tp->t_dev) & 7;
	if (tp->t_outq.c_cc == 0 || (tp->t_state&TS_BUSY)==0)
		if (dp->p_addr->dzcsr & DZ_32)
			dp->p_addr->dzlnen = (dz_lnen[dz] &= ~(1<<unit));
		else
			dp->p_addr->dztcr &= ~(1<<unit);
}

dzstart(tp)
	register struct tty *tp;
{
	register struct pdma *dp;
	register struct dzdevice *dzaddr;
	register int cc;
	int s, dz, unit;

	dp = (struct pdma *)tp->t_addr;
	dzaddr = dp->p_addr;
	s = spl5();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	if (tp->t_flags & (RAW|LITOUT))
		cc = ndqb(&tp->t_outq, 0);
	else {
		cc = ndqb(&tp->t_outq, 0200);
		if (cc == 0) {
			cc = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (cc&0x7f) + 6);
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	tp->t_state |= TS_BUSY;
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
	dz = minor(tp->t_dev) >> 3;
	unit = minor(tp->t_dev) & 7;
	if (dzaddr->dzcsr & DZ_32)
		dzaddr->dzlnen = (dz_lnen[dz] |= (1<<unit));
	else
		dzaddr->dztcr |= (1<<unit);
out:
	splx(s);
}

/*
 * Stop output on a line.
 */

/*ARGSUSED*/
dzstop(tp, flag)
	register struct tty *tp;
{
	register struct pdma *dp;
	register int s;

	dp = (struct pdma *)tp->t_addr;
	s = spl5();
	if (tp->t_state & TS_BUSY) {
		dp->p_end = dp->p_mem;
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

dzmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct dzdevice *dzaddr;
	register int unit, mbits;
	int b, s;

	unit = minor(dev);
	b = 1<<(unit&7);
	dzaddr = dzpdma[unit].p_addr;
	s = spl5();
	if (dzaddr->dzcsr & DZ_32) {
		dzwait(dzaddr)
		DELAY(100);		/* IS 100 TOO MUCH? */
		dzaddr->dzlcs = unit&7;
		DELAY(100);
		dzwait(dzaddr)
		DELAY(100);
		mbits = dzaddr->dzlcs;
		mbits &= 0177770;
	} else {
		mbits = (dzaddr->dzdtr & b) ? DZ_DTR : 0;
		mbits |= (dzaddr->dzmsr & b) ? DZ_CD : 0;
		mbits |= (dzaddr->dztbuf & b) ? DZ_RI : 0;
	}
	switch (how) {
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return(mbits);
	}
	if (dzaddr->dzcsr & DZ_32) {
		mbits |= DZ_ACK|(unit&7);
		dzaddr->dzlcs = mbits;
	} else {
		if (mbits & DZ_DTR)
			dzaddr->dzdtr |= b;
		else
			dzaddr->dzdtr &= ~b;
	}
	(void) splx(s);
	return(mbits);
}

int dztransitions, dzfasttimers;		/*DEBUG*/
dzscan()
{
	register i;
	register struct dzdevice *dzaddr;
	register bit;
	register struct tty *tp;
	register car;
	int olddzsilos = dzsilos;
	int dztimer();

	for (i = 0; i < dz_cnt ; i++) {
		dzaddr = dzpdma[i].p_addr;
		if (dzaddr == 0)
			continue;
		tp = &dz_tty[i];
		bit = 1<<(i&07);
		car = 0;
		if (dzsoftCAR[i>>3]&bit)
			car = 1;
		else if (dzaddr->dzcsr & DZ_32) {
			dzaddr->dzlcs = i&07;
			dzwait(dzaddr);
			car = dzaddr->dzlcs & DZ_CD;
		} else
			car = dzaddr->dzmsr&bit;
		if (car) {
			/* carrier present */
			if ((tp->t_state & TS_CARR_ON) == 0) {
				wakeup((caddr_t)&tp->t_rawq);
				tp->t_state |= TS_CARR_ON;
				tp->t_state &= ~TS_ONDELAY;
			}
		} else {
			if ((tp->t_state&TS_CARR_ON) &&
			  (tp->t_flags&NOHANG) == 0) {
				/* carrier lost */
				if (tp->t_state&TS_ISOPEN) {
					gsignal(tp->t_pgrp, SIGHUP);
					gsignal(tp->t_pgrp, SIGCONT);
					dzaddr->dzdtr &= ~bit;
					ttyflush(tp, FREAD|FWRITE);
				}
				tp->t_state &= ~TS_CARR_ON;
			}
		}
	}
	for (i = 0; i < nNDZ; i++) {
		ave(dzrate[i], dzchars[i], 8);
		if (dzchars[i] > dzhighrate && ((dzsilos & (1 << i)) == 0)) {
			dzpdma[i << 3].p_addr->dzcsr = DZ_IEN | DZ_SAE;
			dzsilos |= (1 << i);
			dztransitions++;		/*DEBUG*/
		} else if ((dzsilos & (1 << i)) && (dzrate[i] < dzlowrate)) {
			dzpdma[i << 3].p_addr->dzcsr = DZ_IEN;
			dzsilos &= ~(1 << i);
		}
		dzchars[i] = 0;
	}
	if (dzsilos && !olddzsilos)
		timeout(dztimer, (caddr_t)0, dztimerintvl);
	timeout(dzscan, (caddr_t)0, hz);
}

dztimer()
{
	register int dz;
	register int s;

	if (dzsilos == 0)
		return;
	s = spl5();
	dzfasttimers++; 	/*DEBUG*/
	for (dz = 0; dz < nNDZ; dz++)
		if (dzsilos & (1 << dz))
			dzrint(dz);
	splx(s);
	timeout(dztimer, (caddr_t) 0, dztimerintvl);
}

/*
 * Reset state of driver if UBA reset was necessary.
 * Reset parameters and restart transmission on open lines.
 */
dzreset(uban)
	int uban;
{
	register int unit;
	register struct tty *tp;
	register struct uba_device *ui;

	for (unit = 0; unit < nNDZLINE; unit++) {
		ui = dzinfo[unit >> 3];
		if (ui == 0 || ui->ui_ubanum != uban || ui->ui_alive == 0)
			continue;
		if (unit%8 == 0)
			printf(" dz%d", unit>>3);
		tp = &dz_tty[unit];
		if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
			dzparam(unit);
			(void) dzmctl(unit, DZ_ON, DMSET);
			tp->t_state &= ~TS_BUSY;
			dzstart(tp);
		}
	}
}
#endif
