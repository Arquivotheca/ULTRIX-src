
#ifndef lint
static char *sccsid = "@(#)qv.c	1.22	ULTRIX	1/9/87";
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
 ************************************************************************
 *
 * This driver provides glass tty functionality to the qvss. It is a strange
 * device in that it supports three subchannels. The first being the asr,
 * the second being a channel that intercepts the chars headed for the screen
 * ( like a pseudo tty ) and the third being a source of mouse state changes.
 *
 * There may be one and only one qvss in the system.  This restriction is based
 * on the inability to map more than one at a time.  This restriction will
 * exist until the kernel has shared memory services. This driver therefore
 * support a single unit. No attempt was made to have it service more.
 *
 * 08-Jan-87 - rafiey (Ali Rafieymehr)
 *
 *	Added two new ioctls to turn the video on and off. These two ioctls
 *	are to be used for save screen. Also corrected a crt parameter for
 *	VR260.
 *
 * 30-Oct-86 - rafiey (Ali Rafieymehr)
 *
 *	Fixed a bug (the way "unit" was defined) in "qvioctl" routine.
 *	Also added "qv_open_flag" lock switch for single process access.
 *
 * 26-Aug-86 - rsp (Ricky Palmer)
 *
 *	Cleaned up devioctl code to (1) zero out devget structure
 *	upon entry and (2) use strlen instead of fixed storage
 *	for bcopy's.
 *
 * 11-Jul-86 - ricky palmer
 *
 *	Added adpt and nexus fields to DEVIOCGET code.
 *
 * 20-May-86 - ricky palmer
 *
 *	Added new DEVIOCGET ioctl request code. V2.0
 *
 * 16-Apr-86 -- darrell
 *	badaddr is now called via the macro BADADDR.
 *
 * 15-Apr-86 -- afd
 *	Changed UMEMmap to QMEMmap and umem to qmem.
 *
 *	v_console() is now refered to as v_consputc, and there is a
 *	corresponding v_consgetc() (defined in /sys/vax/conf.c), which
 *	can point to "qvgetc()".
 *
 *	Added "qvgetc()" routine for console read.  Needed to read
 *	user's answer to the "root device?" prompt with a generic kernel.
 *
 * 21-Mar-86 -- pmk
 *	Changed DELAY to 20000, because microdelay now in real microsec.
 *
 * 18-mar-86  -- jaw	 br/cvec changed to NOT use registers.
 *
 *
 * 11 Mar 86 -- darrell
 *	Changed references to the percpu structure to the cpusw structure
 *	and only referenced the global cpusw once.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 * 02 Aug 85 -- rjl
 *	Changed the names of the special setup routines so that the system
 *	can have a qvss or a qdss system console.
 *
 * 03 Jul 85 -- rjl
 *	Added a check for virtual mode in qvputc so that the driver
 *	doesn't crash while in a dump which is done in physical mode.
 *
 * 10 Apr 85 -- jg
 *	Well, our theory about keyboard handling was wrong; most of the
 *	keyboard is in autorepeat, down mode.  These changes are to make
 *	the qvss work the same as the Vs100, which is not necessarily
 *	completely correct, as some chord usage may fail.  But since we
 *	can't easily change the Vs100, we might as well propagate the
 *	problem to another device.  There are also changes for screen and
 *	mouse accellaration.
 *
 * 27 Mar 85 -- rjl
 *	MicroVAX-II systems have interval timers that interrupt at ipl4.
 *	Everything else is higher and thus causes us to miss clock ticks. The
 *	problem isn't severe except in the case of a device like this one that
 *	generates lots of interrupts. We aren't willing to make this change to
 *	all device drivers but it seems acceptable in this case.
 *
 *  3 Dec 84 -- jg
 *	To continue the tradition of building a better mouse trap,  this
 *	driver has been extended to form Vs100 style event queues.  If the
 *	mouse device is open, the keyboard events are intercepted and put
 *	into the shared memory queue.  Unfortunately, we are ending up with
 *	one of the longest Unix device drivers.  Sigh....
 *
 * 20 Nov 84 -- rjl
 *	As a further complication this driver is required to function as the
 *	virtual system console. This code runs before and during auto-
 *	configuration and therefore is require to have a second path for setup.
 *	It is futher constrained to have a character output routine that
 *	is not dependant on the interrupt system.
 *
 */


#include "qv.h"
#if NQV > 0  || defined(BINARY)

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../vaxuba/qvioctl.h"
#include "../h/tty.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/bk.h"
#include "../h/clist.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/cpuconf.h"
#include "../h/devio.h"

#include "../vax/cpu.h"
#include "../vax/mtpr.h"

#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"

struct	uba_device *qvinfo[NQV];
struct	qv_softc {
	long	sc_flags;		/* Flags			*/
	long	sc_category_flags;	/* Category flags		*/
	u_long	sc_softcnt;		/* Soft error count total	*/
	u_long	sc_hardcnt;		/* Hard error count total	*/
	char	sc_device[DEV_SIZE];	/* Device type string		*/
} qv_softc[NQV];

struct	tty qv_tty[NQV*4];

int	nNQV = NQV;
int	nqv = NQV*4;

/*
 * Definition of the driver for the auto-configuration program.
 */
int	qvprobe(), qvattach(), qvkint(), qvvint();
u_short qvstd[] = { 0 };
struct	uba_driver qvdriver =
	{ qvprobe, 0, qvattach, 0, qvstd, "qv", qvinfo };

/*
 * Local variables for the driver. Initialized for 15' screen
 * so that it can be used during the boot process.
 */

#define QVWAITPRI	(PZERO+1)
#define QVSSMAJOR	35
/*
 * v_consputc is the switch that is used to redirect the console cnputc to the
 * virtual console vputc.
 * v_consgetc is the switch that is used to redirect the console getchar to the
 * virtual console vgetc.
 */
extern (*v_consputc)();
extern (*v_consgetc)();
/*
 * qv_def_scrn is used to select the appropriate tables. 0=15 inch 1=19 inch,
 * 2 = new display (untested).
 */
int qv_def_scrn = 0;

#define QVMAXEVQ	64	/* must be power of 2 */
#define EVROUND(x)	((x) & (QVMAXEVQ - 1))

/*
 * Screen parameters 15 & 19 inch monitors. These determine the max size in
 * pixel and character units for the display and cursor positions.
 * Notice that the mouse defaults to original square algorithm, but X
 * will change to its defaults once implemented.
 */
struct qv_info *qv_scn;
struct qv_info qv_scn_defaults[] = {
	{0, {0, 0}, 0, {0, 0}, 0, 0, 30, 80, 768, 480, 768-16, 480-16,
	 0, 0, 0, 0, 0, QVMAXEVQ, 0, 0, {0, 0}, {0, 0, 0, 0}, 2, 4},
	{0, {0, 0}, 0, {0, 0}, 0, 0, 55, 120, 960, 864, 960-16, 864-16,
	 0, 0, 0, 0, 0, QVMAXEVQ, 0, 0, {0, 0}, {0, 0, 0, 0}, 2, 4},
	{0, {0, 0}, 0, {0, 0}, 0, 0, 56, 120,1024, 864,1024-16, 864-16,
	 0, 0, 0, 0, 0, QVMAXEVQ, 0, 0, {0, 0}, {0, 0, 0, 0}, 2, 4}
};

/*
 * Screen controller initialization parameters. The definations and use
 * of these parameters can be found in the Motorola 68045 crtc specs. In
 * essence they set the display parameters for the chip. The first set is
 * for the 15" screen and the second is for the 19" seperate sync. There
 * is also a third set for a 19" composite sync monitor which we have not
 * tested and which is not supported.
 */
static short qv_crt_parms[][16] = {
	   { 31, 25, 27, 0142, 31, 13, 30, 31, 4, 15, 040, 0, 0, 0, 0, 0 },
/* VR100*/ { 39, 30, 32, 0262, 55, 5, 54, 54, 4, 15, 040, 0, 0, 0, 0, 0 },
/* VR260*/ { 39, 32, 33, 0264, 55, 5, 54, 54, 4, 15, 040, 0, 0, 0, 0, 0},
};
#define QVMOUSECHAN	2

/*
 * Screen parameters
 */
struct qv_info	*qv_scn;
int maxqvmem = 254*1024 - sizeof(struct qv_info) - QVMAXEVQ*sizeof(vsEvent);

/*
 * Keyboard state
 */
struct qv_keyboard {
	int shift;			/* state variables	*/
	int cntrl;
	int lock;
	char last;			/* last character	*/
} qv_keyboard;

short divdefaults[15] = { LK_DOWN,	/* 0 doesn't exist */
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_DOWN,
	LK_UPDOWN,   LK_UPDOWN,   LK_AUTODOWN, LK_AUTODOWN,
	LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN, LK_AUTODOWN,
	LK_DOWN, LK_AUTODOWN };

short kbdinitstring[] = {		/* reset any random keyboard stuff */
	LK_AR_ENABLE,			/* we want autorepeat by default */
	LK_CL_ENABLE,			/* keyclick */
	0x84,				/* keyclick volume */
	LK_KBD_ENABLE,			/* the keyboard itself */
	LK_BELL_ENABLE, 		/* keyboard bell */
	0x84,				/* bell volume */
	LK_LED_DISABLE, 		/* keyboard leds */
	LED_ALL };
#define KBD_INIT_LENGTH sizeof(kbdinitstring)/sizeof(short)

#define TOY ((time.tv_sec * 100) + (time.tv_usec / 10000))

int	qv_events;
int	qv_ipl_lo = 1;			/* IPL low flag 		*/
int	mouseon = 0;			/* mouse channel is enabled when 1*/
int	qv_open_flag = 0;		/* graphics device is open when not 0 */
struct proc *rsel;			/* process waiting for select */

int	qvstart(), qvputc(),  ttrstrt();

/*
 * Keyboard translation and font tables
 */
extern	char q_key[],q_shift_key[],*q_special[],q_font[];
extern	short q_cursor[];

/*
 * See if the qvss will interrupt.
 */

/*ARGSUSED*/
qvprobe(reg, ctlr)
	caddr_t reg;
	int ctlr;
{
	register struct qvdevice *qvaddr = (struct qvdevice *)reg;
	static int tvec, ovec;

	/*
	 * Allocate the next two vectors
	 */
	tvec = 0360;
	ovec = cvec;
	/*
	 * Turn on the keyboard and vertical interrupt vectors.
	 */
	qvaddr->qv_intcsr = 0;		/* init the interrupt controler */
	qvaddr->qv_intcsr = 0x40;	/* reset irr			*/
	qvaddr->qv_intcsr = 0x80;	/* specify individual vectors	*/
	qvaddr->qv_intcsr = 0xc0;	/* preset autoclear data	*/
	qvaddr->qv_intdata = 0xff;	/* all setup as autoclear	*/

	qvaddr->qv_intcsr = 0xe0;	/* preset vector address 1	*/
	qvaddr->qv_intdata = tvec;	/* give it the keyboard vector	*/
	qvaddr->qv_intcsr = 0x28;	/* enable tx/rx interrupt	*/

	qvaddr->qv_intcsr = 0xe1;	/* preset vector address 2	*/
	qvaddr->qv_intdata = tvec+4;	/* give it the vertical sysnc	*/
	qvaddr->qv_intcsr = 0x29;	/* enable			*/

	qvaddr->qv_intcsr = 0xa1;	/* arm the interrupt ctrl	*/

	qvaddr->qv_uartcmd = 0x15;	/* set mode pntr/enable rx/tx	*/
	qvaddr->qv_uartmode = 0x17;	/* noparity, 8-bit		*/
	qvaddr->qv_uartmode = 0x07;	/* 1 stop bit			*/
	qvaddr->qv_uartstatus = 0x99;	/* 4800 baud xmit/recv		*/
	qvaddr->qv_uartintstatus = 2;	/* enable recv interrupts	*/

	qvaddr->qv_csr |= QV_INT_ENABLE | QV_CUR_MODE;

	DELAY(20000);

	qvaddr->qv_csr &= ~QV_INT_ENABLE;

	/*
	 * If the qvss did interrupt it was the second vector not
	 * the first so we have to return the first so that they
	 * will be setup properly
	 */
	if( ovec == cvec ) {
		return 0;
	} else
		cvec = tvec;
	/* can't do this until we know we have memory to write in */
	qv_scn->qvaddr = qvaddr;
	return (sizeof (struct qvdevice));
}

/*
 * Routine called to attach a qv.
 */
qvattach(ui)
	struct uba_device *ui;
{
	register struct qvdevice *qvaddr;
	register i;
	register int *pte;
	char *qvssmem;

	qvaddr = (struct qvdevice *)ui->ui_addr;

	/*
	 * If not the console then we have to setup the screen
	 */
	if( v_consputc != qvputc )
		qv_setup( qvaddr );

	/*
	 * Map the qvss memory for use by users.
	 */
	qvssmem = (char *)((qvaddr->qv_csr & QV_MEM_BANK) << 7);
	pte = (int *)(QMEMmap[0] + btop( qvssmem ));
	for( i=0 ; i<512 ; i++, pte++ )
		*pte = (*pte & ~PG_PROT) | PG_UW | PG_V;
}


/*ARGSUSED*/
qvopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit, qv;
	register struct qvdevice *qvaddr;
	register struct uba_device *ui;
	register struct qv_info *qp = qv_scn;

	unit = minor(dev);
	if( ( unit % 4 ) == QVMOUSECHAN ) {
		if (qv_open_flag)
		    return(EBUSY);
		else
		    qv_open_flag = 1;  /* mark the device not available */
	}
	qv = unit >> 2;
	if (unit >= nqv || (ui = qvinfo[qv])== 0 || ui->ui_alive == 0)
		return (ENXIO);
	tp = &qv_tty[unit];
	if (tp->t_state&TS_XCLUDE && u.u_uid!=0)
		return (EBUSY);
	qvaddr = (struct qvdevice *)ui->ui_addr;
	qv_scn->qvaddr = qvaddr;
	tp->t_addr = (caddr_t)qvaddr;
	tp->t_oproc = qvstart;

	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		tp->t_ispeed = B9600;
		tp->t_ospeed = B9600;
		if( unit == 0 ) {
			/* make sure keyboard is always back to default */
			qvkbdreset();
			tp->t_flags = XTABS|EVENP|ECHO|CRMOD;
		} else
			tp->t_flags = RAW;
	}
	qvaddr->qv_csr |= QV_INT_ENABLE;
	/*
	 * Process line discipline specific open if its not the
	 * mouse channel. For the mouse we init the ring ptr's.
	 */
	if( ( unit % 4 ) != QVMOUSECHAN )
		return ((*linesw[tp->t_line].l_open)(dev, tp));
	else {
		mouseon = 1;
		/* set up event queue for later */
		qp->ibuff = (vsEvent *)qp - QVMAXEVQ;
		qp->iqsize = QVMAXEVQ;
		qp->ihead = qp->itail = 0;
		return 0;
	}
}

/*
 * Close a QVSS line.
 */
/*ARGSUSED*/
qvclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register unit;
	register struct qvdevice *qvaddr;

	unit = minor(dev);
	tp = &qv_tty[unit];

	/*
	 * If this is the keyboard unit (0) shutdown the
	 * interface.
	 */
	qvaddr = (struct qvdevice *)tp->t_addr;
#ifdef notdef
	if( unit == 0 )
		qvaddr->qv_csr &= ~QV_INT_ENABLE;
#endif

	/*
	 * If unit is not the mouse channel call the line disc.
	 * otherwise clear the state flag, and put the keyboard into down/up.
	 */
	if( ( unit % 4 ) != QVMOUSECHAN ){
		(*linesw[tp->t_line].l_close)(tp);
		ttyclose(tp);
	} else {
		mouseon = 0;
		if (qv_open_flag != 1)
		    return(EBUSY);
		else
		    qv_open_flag = 0;  /* mark the graphics device available */
		qv_init( qvaddr );
	}
	tp->t_state = 0;
}

qvread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	int unit = minor( dev );

	if( (unit % 4) != QVMOUSECHAN ) {
		tp = &qv_tty[unit];
		return ((*linesw[tp->t_line].l_read)(tp, uio));
	}
	return (ENXIO);
}

qvwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	int unit = minor( dev );

	/*
	 * If this is the mouse we simply fake the i/o, otherwise
	 * we let the line disp. handle it.
	 */
	if( (unit % 4) == QVMOUSECHAN ){
		uio->uio_offset = uio->uio_resid;
		uio->uio_resid = 0;
		return 0;
	}
	tp = &qv_tty[unit];
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}


/*
 * Mouse activity select routine
 */
qvselect(dev, rw)
dev_t dev;
{
	register int s = spl5();
	register int unit = minor(dev);
	register struct qv_info *qp = qv_scn;

	if( (unit % 4) == QVMOUSECHAN )
		switch(rw) {
		case FREAD:			/* if events okay */
			if(qp->ihead != qp->itail) {
				splx(s);
				return(1);
			}
			rsel = u.u_procp;
			splx(s);
			return(0);
		case FWRITE:			/* can never write */
			splx(s);
			return(EACCES);
		}
	else
		return( ttselect(dev, rw) );
}

/*
 * QVSS keyboard interrupt.
 */
qvkint(qv)
	int qv;
{
	struct tty *tp;
	register c;
	struct uba_device *ui;
	register int key;
	register int i,j;
	int k,l;

	ui = qvinfo[qv];
	if (ui == 0 || ui->ui_alive == 0)
		return;
	tp = &qv_tty[qv<<2];
	/*
	 * Get a character from the keyboard.
	 */
	key = ((struct qvdevice *)ui->ui_addr)->qv_uartdata & 0xff;
	if( mouseon == 0) {
		/*
		 * Check for various keyboard errors
		 */
		if( key == LK_POWER_ERROR || key == LK_KDOWN_ERROR ||
		    key == LK_INPUT_ERROR || key == LK_OUTPUT_ERROR) {
				mprintf("qv%d: Keyboard error, code = %x\n",qv,key);
				return;
		}
		if( key < LK_LOWEST ) return;
		/*
		 * See if its a state change key
		 */
		switch ( key ) {
		case LOCK:
			qv_keyboard.lock ^= 0xffff;	/* toggle */
			if( qv_keyboard.lock )
				qv_key_out( LK_LED_ENABLE );
			else
				qv_key_out( LK_LED_DISABLE );
			qv_key_out( LED_3 );
			return;
		case SHIFT:
			qv_keyboard.shift ^= 0xffff;
			return;
		case CNTRL:
			qv_keyboard.cntrl ^= 0xffff;
			return;
		case ALLUP:
			qv_keyboard.cntrl = qv_keyboard.shift = 0;
			return;
		case REPEAT:
			c = qv_keyboard.last;
			break;
		default:
		/*
		 * Test for control characters. If set, see if the character
		 * is elligible to become a control character.
		 */
			if( qv_keyboard.cntrl ) {
				c = q_key[ key ];
				if( c >= ' ' && c <= '~' )
					c &= 0x1f;
			} else if( qv_keyboard.lock || qv_keyboard.shift )
				c = q_shift_key[ key ];
				else
				c = q_key[ key ];
			break;
		}

		qv_keyboard.last = c;

		/*
		 * Check for special function keys
		 */
		if( c & 0x80 ) {
			register char *string;
			string = q_special[ c & 0x7f ];
			while( *string )
			(*linesw[tp->t_line].l_rint)(*string++, tp);
		} else
			(*linesw[tp->t_line].l_rint)(c, tp);
	} else {
		/*
		 * Mouse channel is open put it into the event queue
		 * instead.
		 */
		register struct qv_info *qp = qv_scn;
		register vsEvent *vep;

		if ((i = EVROUND(qp->itail+1)) == qp->ihead) return;
		vep = &qp->ibuff[qp->itail];
		vep->vse_direction = VSE_KBTRAW;
		vep->vse_type = VSE_BUTTON;
		vep->vse_device = VSE_DKB;
		vep->vse_x = qp->mouse.x;
		vep->vse_y = qp->mouse.y;
		vep->vse_time = TOY;
		vep->vse_key = key;
		qp->itail = i;
		if(rsel) {
			selwakeup(rsel,0);
			rsel = 0;
		}
	}
}

/*
 * Ioctl for QVSS.
 */
/*ARGSUSED*/
qvioctl(dev, cmd, data, flag)
	dev_t dev;
	register caddr_t data;
{
	register struct tty *tp;
	register int unit = minor(dev) >> 2;
	register struct qv_info *qp = qv_scn;
	register struct qv_kpcmd *qk;
	register unsigned char *cp;
	struct uba_device *ui = qvinfo[unit];
	struct qv_softc *sc = &qv_softc[ui->ui_unit];
	struct devget *devget;
	int error;
	int minor_dev = minor(dev);

	/*
	 * Check for and process qvss specific ioctl's
	 */
	switch( cmd ) {
	case QIOCGINFO: 				/* return screen info */
		bcopy(qp, data, sizeof (struct qv_info));
		break;

	case QIOCSMSTATE:				/* set mouse state */
		qp->mouse = *((vsCursor *)data);
		qv_pos_cur( qp->mouse.x, qp->mouse.y );
		break;

	case QIOCINIT:					/* init screen	*/
		qv_init( qp->qvaddr );
		break;

	case QIOCKPCMD:
		qk = (struct qv_kpcmd *)data;
		if(qk->nbytes == 0) qk->cmd |= 0200;
		if(mouseon == 0) qk->cmd |= 1;	/* no mode changes */
		qv_key_out(qk->cmd);
		cp = &qk->par[0];
		while(qk->nbytes-- > 0) {	/* terminate parameters */
			if(qk->nbytes <= 0) *cp |= 0200;
			qv_key_out(*cp++);
		}
		break;
	case QIOCADDR:					/* get struct addr */
		*(struct qv_info **) data = qp;
		break;

	case QIOVIDEOON:				/* turn on the video */
		qp->qvaddr->qv_csr |= QV_VIDEO_ENA;
		break;

	case QIOVIDEOOFF:				/* turn off the video */
		qp->qvaddr->qv_csr &= ~QV_VIDEO_ENA;
		break;

	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));
		devget->category = DEV_TERMINAL;
		devget->bus = DEV_QB;
		bcopy(DEV_VCB01,devget->interface,
		      strlen(DEV_VCB01));
		bcopy(DEV_VR260,devget->device,
		      strlen(DEV_VR260));		/* terminal	*/
		devget->adpt_num = ui->ui_adpt; 	/* which adapter*/
		devget->nexus_num = ui->ui_nexus;	/* which nexus	*/
		devget->bus_num = ui->ui_ubanum;	/* which QB	*/
		devget->ctlr_num = unit;		/* which interf.*/
		devget->slave_num = unit;		/* which line	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "qv"	*/
		devget->unit_num = unit;		/* qv line?	*/
		devget->soft_count =
		      sc->sc_softcnt;			/* soft er. cnt.*/
		devget->hard_count =
		      sc->sc_hardcnt;			/* hard er cnt. */
		devget->stat = sc->sc_flags;		/* status	*/
		devget->category_stat =
		      sc->sc_category_flags;		/* cat. stat.	*/
		break;

	default:					/* not ours ??	*/
		tp = &qv_tty[minor_dev];
		error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
		if (error >= 0)
			return (error);
		error = ttioctl(tp, cmd, data, flag);
		if (error >= 0) {
			return (error);
		}
		break;
	}
	return (0);
}
/*
 * Initialize the screen and the scanmap
 */
qv_init(qvaddr)
struct qvdevice *qvaddr;
{
	register short *scanline;
	register int i;
	register short scan;
	register char *ptr;
	register struct qv_info *qp = qv_scn;

	/*
	 * Clear the bit map
	 */
	for( i=0 , ptr = qp->bitmap ; i<240 ; i += 2 , ptr += 2048)
		bzero( ptr, 2048 );
	/*
	 * Reinitialize the scanmap
	 */
	scan = qvaddr->qv_csr & QV_MEM_BANK;
	scanline = qp->scanmap;
	for(i = 0 ; i < qp->max_y ; i++ )
		*scanline++ = scan++;

	/*
	 * Home the cursor
	 */
	qp->row = qp->col = 0;

	/*
	 * Reset the cursor to the default type.
	 */
	for( i=0 ; i<16 ; i++ )
		qp->cursorbits[i] = q_cursor[i];
	qvaddr->qv_csr |= QV_CUR_MODE;
	/*
	 * Reset keyboard to default state.
	 */
	qvkbdreset();
}

qvreset()
{
}
qvkbdreset()
{
	register int i;
	qv_key_out(LK_DEFAULTS);
	for( i=1 ; i < 15 ; i++ )
		qv_key_out( divdefaults[i] | (i<<3));
	for (i = 0; i < KBD_INIT_LENGTH; i++)
		qv_key_out(kbdinitstring[i]);
}

#define abs(x) (((x) > 0) ? (x) : (-(x)))
/*
 * QVSS vertical sync interrupt
 */
qvvint(qv)
	int qv;
{
	extern int selwait;
	register struct qvdevice *qvaddr;
	struct uba_device *ui;
	register struct qv_info *qp = qv_scn;
	int unit;
	struct tty *tp0;
	int i;
	register int j;
	/*
	 * Mouse state info
	 */
	static ushort omouse = 0, nmouse = 0;
	static char omx=0, omy=0, mx=0, my=0, om_switch=0, m_switch=0;
	register int dx, dy;

	/*
	 * Test and set the qv_ipl_lo flag. If the result is not zero then
	 * someone else must have already gotten here.
	 */
	if( --qv_ipl_lo )
		return;
	spl4();
	ui = qvinfo[qv];
	unit = qv<<2;
	qvaddr = (struct qvdevice *)ui->ui_addr;
	tp0 = &qv_tty[(unit % 4 )+QVMOUSECHAN];
	/*
	 * See if the mouse has moved.
	 */
	if( omouse != (nmouse = qvaddr->qv_mouse) ) {
		omouse = nmouse;
		mx = nmouse & 0xff;
		my = nmouse >> 8;
		dy = my - omy; omy = my;
		dx = mx - omx; omx = mx;
		if( dy < 50 && dy > -50 && dx < 50 && dx > -50 ) {
			register vsEvent *vep;
			if( qp->mscale < 0 ) {	/* Ray Lanza's original */
				if( dy < 0 )
					dy = -( dy * dy );
				else
					dy *= dy;
				if( dx < 0 )
					dx = -( dx * dx );
				else
					dx *= dx;
			}
			else {			/* Vs100 style, see WGA spec */
			    int thresh = qp->mthreshold;
			    int scale  = qp->mscale;
			    if( abs(dx) > thresh ) {
				if ( dx < 0 )
				    dx = (dx + thresh)*scale - thresh;
				else
				    dx = (dx - thresh)*scale + thresh;
			    }
			    if( abs(dy) > thresh ) {
				if ( dy < 0 )
				    dy = (dy + thresh)*scale - thresh;
				else
				    dy = (dy - thresh)*scale + thresh;
			    }
			}
			qp->mouse.x += dx;
			qp->mouse.y -= dy;
			if( qp->mouse.x < 0 )
				qp->mouse.x = 0;
			if( qp->mouse.y < 0 )
				qp->mouse.y = 0;
			if( qp->mouse.x > qp->max_cur_x )
				qp->mouse.x = qp->max_cur_x;
			if( qp->mouse.y > qp->max_cur_y )
				qp->mouse.y = qp->max_cur_y;
			if( tp0->t_state & TS_ISOPEN )
				qv_pos_cur( qp->mouse.x, qp->mouse.y );
			if (qp->mouse.y < qp->mbox.bottom &&
			    qp->mouse.y >=  qp->mbox.top &&
			    qp->mouse.x < qp->mbox.right &&
			    qp->mouse.x >=  qp->mbox.left) goto switches;
			qp->mbox.bottom = 0;	/* trash box */
			if (EVROUND(qp->itail+1) == qp->ihead)
				goto switches;
			i = EVROUND(qp->itail - 1);
			if ((qp->itail != qp->ihead) && (i != qp->ihead)) {
				vep = & qp->ibuff[i];
				if(vep->vse_type == VSE_MMOTION) {
					vep->vse_x = qp->mouse.x;
					vep->vse_y = qp->mouse.y;
					goto switches;
				}
			}
			/* put event into queue and do select */
			vep = & qp->ibuff[qp->itail];
			vep->vse_type = VSE_MMOTION;
			vep->vse_time = TOY;
			vep->vse_x = qp->mouse.x;
			vep->vse_y = qp->mouse.y;
			qp->itail = EVROUND(qp->itail+1);
		}
	}
	/*
	 * See if mouse switches have changed.
	 */
switches:if( om_switch != ( m_switch = (qvaddr->qv_csr & QV_MOUSE_ANY) >> 8 ) ) {
		qp->mswitches = ~m_switch & 0x7;
		for (j = 0; j < 3; j++) {	/* check each switch */
			register vsEvent *vep;
			if ( ((om_switch>>j) & 1) == ((m_switch>>j) & 1) )
				continue;
			/* check for room in the queue */
			if ((i = EVROUND(qp->itail+1)) == qp->ihead) return;
			/* put event into queue and do select */
			vep = &qp->ibuff[qp->itail];
			vep->vse_type = VSE_BUTTON;
			vep->vse_key = 2 - j;
			vep->vse_direction = VSE_KBTDOWN;
			if ( (m_switch >> j) & 1)
				vep->vse_direction = VSE_KBTUP;
			vep->vse_device = VSE_MOUSE;
			vep->vse_time = TOY;
			vep->vse_x = qp->mouse.x;
			vep->vse_y = qp->mouse.y;
		}
		qp->itail =  i;
		om_switch = m_switch;
		qp->mswitches = m_switch;
	}
	/* if we have proc waiting, and event has happened, wake him up */
	if(rsel && (qp->ihead != qp->itail)) {
		selwakeup(rsel,0);
		rsel = 0;
	}
	/*
	 * Okay we can take another hit now
	 */
	qv_ipl_lo = 1;
}

/*
 * Start  transmission
 */
qvstart(tp)
	register struct tty *tp;
{
	register int unit, c;
	register struct tty *tp0;
	int s;

	unit = minor(tp->t_dev);
	tp0 = &qv_tty[(unit&0xfc)+1];
	unit &= 03;

	s = spl5();
	/*
	 * If it's currently active, or delaying, no need to do anything.
	 */
	if (tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	/*
	 * Display chars until the queue is empty, if the second subchannel
	 * is open direct them there. Drop characters from subchannels other
	 * than 0 on the floor.
	 */

	while( tp->t_outq.c_cc ) {
		c = getc(&tp->t_outq);
		if( (unit & 0x03)  == 0 )
			if( tp0->t_state & TS_ISOPEN ){
				(*linesw[tp0->t_line].l_rint)(c, tp0);
			} else
				qvputc( c & 0xff );
	}
	/*
	 * Position the cursor to the next character location.
	 */
	qv_pos_cur( qv_scn->col*8, qv_scn->row*15 );

	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, wake up the sleepers.
	 */
	if ( tp->t_outq.c_cc<=TTLOWAT(tp) ) {
		if (tp->t_state&TS_ASLEEP){
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
	}
	tp->t_state &= ~TS_BUSY;
out:
	splx(s);
}

/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
qvstop(tp, flag)
	register struct tty *tp;
{
	register int s;

	/*
	 * Block input/output interrupts while messing with state.
	 */
	s = spl5();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP)==0) {
			tp->t_state |= TS_FLUSH;
		} else
			tp->t_state &= ~TS_BUSY;
	}
	splx(s);
}

/*
 * Routine to display a character on the screen.  The model used is a
 * glass tty.  It is assummed that the user will only use this emulation
 * during system boot and that the screen will be eventually controlled
 * by a window manager.
 *
 */
qvputc( c )
register char c;
{

	register char *b_row, *f_row;
	register int i;
	register short *scanline;
	register int ote = 128;
	register struct qv_info *qp = qv_scn;

	/*
	 * This routine may be called in physical mode by the dump code
	 * so we check and punt if that's the case.
	 */
	if( (mfpr(MAPEN) & 1) == 0 )
		return;

	c &= 0x7f;

	switch ( c ) {
	case '\t':				/* tab		*/
		for( i = 8 - (qp->col & 0x7) ; i > 0 ; i-- )
			qvputc( ' ' );
		break;

	case '\r':				/* return	*/
		qp->col = 0;
		break;

	case '\010':				/* backspace	*/
		if( --qp->col < 0 )
			qp->col = 0;
		break;

	case '\n':				/* linefeed	*/
		if( qp->row+1 >= qp->max_row )
			qvscroll();
		else
			qp->row++;
		/*
		* Position the cursor to the next character location.
		*/
		qv_pos_cur( qp->col*8, qp->row*15 );
		break;

	case '\007':				/* bell 	*/
		/*
		 * We don't do anything to the keyboard until after
		 * autoconfigure.
		 */
		if( qp->qvaddr )
			qv_key_out( LK_RING_BELL );
		return;

	default:
		if( c >= ' ' && c <= '~' ) {
			scanline = qp->scanmap;
			b_row = qp->bitmap+(scanline[qp->row*15]&0x3ff)*128+qp->col;
			i = c - ' ';
			if( i < 0 || i > 95 )
				i = 0;
			else
				i *= 15;
			f_row = (char *)((int)q_font + i);

/*			for( i=0 ; i<15 ; i++ , b_row += 128, f_row++ )
				*b_row = *f_row;*/
			/* inline expansion for speed */
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;

			if( ++qp->col >= qp->max_col ) {
				qp->col = 0 ;
				if( qp->row+1 >= qp->max_row )
					qvscroll();
				else
					qp->row++;
			}
		}
		break;
	}
}


/*
 * QVSS keyboard interrupt.
 */
qvgetc()
{
	register struct qvdevice *qvaddr;
	register struct qv_info *qp = qv_scn;
	char *string;
	int c;
	int j;

	qvaddr = qp->qvaddr;
	/*
	 * Get a character from the keyboard.
	 */
loop:
	while( (qvaddr->qv_uartstatus & 0x01) == 0 )
		;
	j = qvaddr->qv_uartdata & 0xff;
	/*
	 * See if its a state change key
	 */
	switch ( j ) {
	case LOCK:
		qv_keyboard.lock ^= 0xffff;	/* toggle */
		if( qv_keyboard.lock )
			qv_key_out( LK_LED_ENABLE );
		else
			qv_key_out( LK_LED_DISABLE );
		qv_key_out( LED_3 );
		goto loop;
	case SHIFT:
		qv_keyboard.shift ^= 0xffff;
		goto loop;
	case CNTRL:
		qv_keyboard.cntrl ^= 0xffff;
		goto loop;
	case ALLUP:
		qv_keyboard.cntrl = qv_keyboard.shift = 0;
		goto loop;
	case REPEAT:
		c = qv_keyboard.last;
		break;
	default:
		/*
		 * Test for control characters. If set, see if the character
		 * is elligible to become a control character.
		 */
		if( qv_keyboard.cntrl ) {
			c = q_key[ j ];
			if( c >= ' ' && c <= '~' )
				c &= 0x1f;
		} else if( qv_keyboard.lock || qv_keyboard.shift )
			c = q_shift_key[ j ];
		else
			c = q_key[ j ];
		break;
	}

	qv_keyboard.last = c;

	/*
	 * Check for special function keys
	 */
	if( c & 0x80 )
		return 0;
	else
		return c;
}

/*
 * Position the cursor to a particular spot.
 */
qv_pos_cur( x, y)
register int x,y;
{
	register struct qvdevice *qvaddr;
	register struct qv_info *qp = qv_scn;
	register index;

	if( qvaddr = qp->qvaddr ) {
		if( y < 0 || y > qp->max_cur_y )
			y = qp->max_cur_y;
		if( x < 0 || x > qp->max_cur_x )
			x = qp->max_cur_x;
		qp->cursor.x = x;		/* keep track of real cursor*/
		qp->cursor.y = y;		/* position, indep. of mouse*/

		qvaddr->qv_crtaddr = 10;	/* select cursor start reg */
		qvaddr->qv_crtdata = y & 0xf;
		qvaddr->qv_crtaddr = 11;	/* select cursor end reg */
		qvaddr->qv_crtdata = y & 0xf;
		qvaddr->qv_crtaddr = 14;	/* select cursor y pos. */
		qvaddr->qv_crtdata = y >> 4;
		qvaddr->qv_xcur = x;		/* pos x axis	*/
		/*
		 * If the mouse is being used then we change the mode of
		 * cursor display based on the pixels under the cursor
		 */
		if( mouseon ) {
			index = y*128 + x/8;
			if( qp->bitmap[ index ] && qp->bitmap[ index+128 ] )
				qvaddr->qv_csr &= ~QV_CUR_MODE;
			else
				qvaddr->qv_csr |=  QV_CUR_MODE;
		}
	}
}
/*
 * Scroll the bitmap by moving the scanline map words. This could
 * be done by moving the bitmap but it's much too slow for a full screen.
 * The only drawback is that the scanline map must be reset when the user
 * wants to do graphics.
 */
qvscroll()
{
	short tmpscanlines[15];
	register char *b_row;
	register short *scanline;
	register struct qv_info *qp = qv_scn;

	/*
	 * If the mouse is on we don't scroll so that the bit map
	 * remains sane.
	 */
	if( mouseon ) {
		qp->row = 0;
		return;
	}
	/*
	 * Save the first 15 scanlines so that we can put them at
	 * the bottom when done.
	 */
	bcopy( qp->scanmap, tmpscanlines, sizeof tmpscanlines );

	/*
	 * Clear the wrapping line so that it won't flash on the bottom
	 * of the screen.
	 */
	scanline = qp->scanmap;
	b_row = qp->bitmap+(*scanline&0x3ff)*128;
	bzero( b_row, 1920 );

	/*
	 * Now move the scanlines down
	 */
	bcopy( qp->scanmap+15, qp->scanmap, (qp->row * 15) * sizeof (short) );

	/*
	 * Now put the other lines back
	 */
	bcopy( tmpscanlines, qp->scanmap+(qp->row * 15), sizeof tmpscanlines );

}

/*
 * Output to the keyboard. This routine status polls the transmitter on the
 * keyboard to output a code. The timer is to avoid hanging on a bad device.
 */
qv_key_out( c )
char c;
{
	int timer = 30000;
	register struct qv_info *qp = qv_scn;

	if( qp->qvaddr ) {
		while( (qp->qvaddr->qv_uartstatus & 0x4) == 0  && timer-- )
			;
		qp->qvaddr->qv_uartdata = c;
	}
}
/*
 * Virtual console initialization. This routine sets up the qvss so that it can
 * be used as the system console. It is invoked before autoconfig and has to do
 * everything necessary to allow the device to serve as the system console.
 * In this case it must map the q-bus and device areas and initialize the qvss
 * screen.
 */
qvcons_init()
{
	struct cpusw *cpup;		/* pointer to cpusw structure  */
	struct qvdevice *qvaddr;	/* device pointer		*/
	short *devptr;			/* vitual device space		*/
#define QVSSCSR 017200

	/*
	 * find the cpusw entry that matches this machine.
	 */
	cpup = &cpusw[cpu];
	if( cpup == NULL )
		return(0);

	/*
	 * Found an entry for this cpu. Because this device is Microvax specific
	 * we assume that there is a single q-bus and don't have to worry about
	 * multiple adapters.
	 *
	 * Map the bus memory and device registers.
	 */
	ubaaccess(((*cpup->v_umaddr)(0)), QMEMmap[0],
		cpup->pc_umsize, PG_V|PG_KW);
	ubaaccess(((*cpup->v_udevaddr)(0)), QMEMmap[0]+btop(cpup->pc_umsize),
		DEVSPACESIZE ,PG_V|PG_KW);

	/*
	 * See if the qvss is there.
	 */
	devptr = (short *)((char *)qmem[0]+cpup->pc_umsize);
	qvaddr = (struct qvdevice *)((u_int)devptr + ubdevreg(QVSSCSR));
	if( BADADDR( qvaddr, sizeof(short) ) )
		return(0);
	/*
	 * Okay the device is there lets set it up
	 */
	qv_setup( qvaddr );
	v_consputc = qvputc;
	v_consgetc = qvgetc;
	cdevsw[0] = cdevsw[QVSSMAJOR];
	return(1);
}
/*
 * Do the board specific setup
 */
qv_setup( qvaddr )
struct qvdevice *qvaddr;
{
	char *qvssmem;			/* pointer to the display mem	*/
	int i;				/* simple index 		*/
	register struct qv_info *qp;

	qvssmem = (char *)
		(( (u_int)(qvaddr->qv_csr & QV_MEM_BANK) <<7 ) + (u_int)qmem[0]);
	qv_scn = (struct qv_info *)
		((u_int)qvssmem + 251*1024);
	qp = qv_scn;
	if( (qvaddr->qv_csr & QV_19INCH) && qv_def_scrn == 0)
		qv_def_scrn = 1;
	*qv_scn = qv_scn_defaults[ qv_def_scrn ];
	qp->bitmap = qvssmem;
	qp->scanmap = (short *)((u_int)qvssmem + 254*1024);
	qp->cursorbits = (short *)((u_int)qvssmem + 256*1024-32);
	/* set up event queue for later */
	qp->ibuff = (vsEvent *)qp - QVMAXEVQ;
	qp->iqsize = QVMAXEVQ;
	qp->ihead = qp->itail = 0;

	/*
	 * Setup the crt controller chip.
	 */
	for( i=0 ; i<16 ; i++ ) {
		qvaddr->qv_crtaddr = i;
		qvaddr->qv_crtdata = qv_crt_parms[ qv_def_scrn ][ i ];
	}
	/*
	 * Setup the display.
	 */
	qv_init( qvaddr );

	/*
	 * Turn on the video
	 */
	qvaddr->qv_csr |= QV_VIDEO_ENA ;
}
#endif
