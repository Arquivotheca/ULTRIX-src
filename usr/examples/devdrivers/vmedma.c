#ifndef lint
static char *sccsid = "@(#)vmedma.c	4.1	(ULTRIX)	2/21/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
/********************************************************/
/* xx.c - DMA device driver                             */
/*                                                      */
/* Abstract:                                            */
/*                                                      */
/* This driver supports an XX device.  The XX device    */
/* is a simple DMA interface that uses the              */
/* 32-bit VMEbus.                                       */
/*                                                      */
/* Author: Digital Equipment Corporation                */
/*                                                      */


/********************************************************/
/*        INCLUDE FILES                                 */
/*                                                      */
/********************************************************/
/*                                                      */
/* Header files required by DMA device driver */
#include "../h/types.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../machine/cpu.h"
#include "../io/uba/ubavar.h"
#include "../h/uio.h"
#include "../../machine/common/cpuconf.h" /* Include for BADADDR */  
#include "../io/vme/vbareg.h" /* VMEbus definitions */ 
#include "xx.h" /* Driver header file generated by config */  


/**********************************************************/
/*         DECLARATIONS                                   */
/*                                                        */
/**********************************************************/

/********* Register Structure for XX device ***************/
/*                                                        */
/**********************************************************/
struct xx_reg {
    volatile char  csr;           /* One byte control/status register */
    volatile short count;         /* Short byte count */
    volatile unsigned int addr;   /* 32-bit transfer address */
};

/********* Bits for csr member ****************************/
/*                                                        */
/**********************************************************/
#define  IE         0001   /* Interrupt Enable */
#define  DMA_GO     0002   /* Start DMA */
#define  RESET      0010   /* Ready for data transfer */
#define  ERROR      0020   /* Indicate error */ 
#define  READ       0040   /* Indicate data transfer is read */


/********* Driver Routines ********************************/
/*                                                        */
/**********************************************************/
/* Declare DMA device driver routines */ 
int  xxprobe(), xxopen(), xxclose(), xxread(), xxwrite(),
     xxstrategy(), xxintr(); 


/********* buf, uba_device, and uba_driver Structures *******/
/*                                                          */
/************************************************************/
/* Declare an array of buf structures */ 
struct buf xxbuf[NXX]; 
/* Declare an array of pointers to uba_device structures */ 
struct uba_device *xxdinfo[NXX]; 
/* Declare and initialize uba_driver structure */ 
struct uba_driver xxdriver = {
        xxprobe,0,0,0,0,
        "xx",xxdinfo,0,0,0,0x5,VMEA32D32,0,0
}; 


/********* Unit Number Compare Variable *******************/
/*                                                        */
/**********************************************************/
/* Declare and initialize unit number compare variable */ 
int nNXX=NXX; 


/********** Maximum DMA size ******************************/
/*							  */
/**********************************************************/
/* Define for maximum DMA transfer			*/
#define	XXMAXPHYS (64 * 1024)

/********** Softc Structure *******************************/
/*                                                        */
/**********************************************************/
/* Declare an xx_softc structure */ 
struct xx_softc {
         char  sc_csr;         /* A copy of csr */
         int   sc_open;        /* XXOPEN, XXCLOSE */
#define  XXOPEN  1
#define  XXCLOSE  0
         int   sc_error;       /* Driver specific error code */
#define  EACCFAULT  200        /* Access violation */
#define  ENOMAPREG  201        /* No mapping registers */
#define  EBUFTOOBIG 202        /* Buffer too big */
         unsigned int vmeaddr; /* Return for vbasetup */ 
         struct buf *bp;       /* To save buffer pointer */
                               /* for use by xxintr */
} xx_softc[NXX]; 


/**********************************************************/
/*         AUTOCONFIGURATION                              */
/*                                                        */
/**********************************************************/

/********* Probe Routine **********************************/
/*                                                        */
/* The xxprobe routine is called from the ULTRIX          */
/* configuration code during the boot phase.  The xxprobe */
/* routine calls the BADADDR macro to determine           */ 
/* if the device is present.  If the device is present,   */
/* xxprobe returns the size of the device structure.      */
/* If the device is not present, xxprobe returns 0.       */
/**********************************************************/
xxprobe(unit, addr1)
int unit; /* Unit number for XX device */ 
caddr_t addr1; /* System Virtual Address for the XX device */ 
{
      /* Initialize pointer to an xx_reg structure */ 
      register struct xx_reg *reg = (struct xx_reg *) addr1; 

      /* Determine if device is present */ 
      if (BADADDR ((char *) &reg->csr, sizeof(char)) !=0) 
      {
          return(0); 
      }

      /* Reset the device */ 
      reg->csr = RESET; 

      wbflush(); /* Assure that write to I/O space completes */
       
      if (reg->csr & ERROR) /* If device error, return 0 */ 
      { 
          return(0); 
      }
      reg->csr = 0; /* Otherwise, initialize the csr */ 

      wbflush(); /* Assure that write to I/O space completes */

      return (sizeof(struct xx_reg)); /* Return size of xx_reg structure */
}

/**********************************************************/
/*         OPEN AND CLOSE                                 */
/*                                                        */
/**********************************************************/

/********* Open Routine ***********************************/
/*                                                        */
/* The xxopen routine is called from the ULTRIX           */
/* spec_open routine.  The xxopen routine checks          */
/* that the device is open uniquely.  In addition, it     */
/* initializes the flag variable.                         */
/**********************************************************/
xxopen(dev, flag)
dev_t  dev; /* Major/minor device number */ 
int flag;   /* Flags from /usr/sys/h/file.h */ 
{
      /* Initialize unit to the minor device number */ 
      register int unit = minor(dev); 
      /* Initialize pointer to uba_device structure */ 
      register struct uba_device  *devptr = xxdinfo[unit]; 
      /* Initialize pointer to xx_softc structure */ 
      register struct xx_softc *sc = &xx_softc[unit];
      /* Make sure that the unit number is no more than the */
      /* system configured */ 
      if (unit >= nNXX )
             return (EIO);

      /* Make sure the open is unique */ 
      if (sc->sc_open == XXOPEN)
             return (EBUSY);

      /* If device is initialized, set sc_open */ 
      /* and return 0 to indicate success. */  
      if ((devptr !=0) && (devptr->ui_alive == 1)) 
      {
             sc->sc_open = XXOPEN;
             return(0);
       }

      /* Otherwise, return an error. */ 
      else return(ENXIO);
}

/********* Close Routine **********************************/
/*                                                        */
/* The xxclose routine is called from the ULTRIX          */
/* spec_close routine.  The xxclose routine clears the    */
/* XXOPEN flag to allow other processes to use the        */ 
/* device.                                                */
/*                                                        */
/**********************************************************/
xxclose(dev, flag)
dev_t  dev; /* Major/minor device number */ 
int flag;   /* Flags from /usr/sys/h/file.h */ 
{
      /* Initialize unit to the minor device number */ 
      register int unit = minor(dev); 
      /* Initialize pointer to uba_device structure */ 
      register struct uba_device  *devptr = xxdinfo[unit]; 
      /* Initialize pointer to xx_softc structure */ 
      register struct xx_softc *sc = &xx_softc[unit];
      /* Initialize pointer to xx_reg structure */ 
      struct xx_reg *reg = (struct xx_reg *) devptr->ui_addr;

       
      sc->sc_open = XXCLOSE; /* Turn off the open bit. */
      reg->csr = 0; /* Turn off interrupts. */
      wbflush(); /* Assure write to I/O space completes. */
      return(0); /* Return success. */
}

/**********************************************************/
/*         READ AND WRITE                                 */
/*                                                        */
/**********************************************************/

/********* Read Routine ***********************************/
/*                                                        */
/* The xxread routine is called from the ULTRIX           */
/* spec_rwgp routine.  The xxread routine will call       */
/* the ULTRIX physio routine to perform the buffer        */
/* lock, buffer check, I/O package set up.                */
/* The physio routine calls the xxstrategy routine        */
/* to access the device.                                  */
/*                                                        */
/**********************************************************/
xxread(dev, uio)
dev_t  dev; /* Major/minor device number */ 
struct uio *uio; /* Pointer to uio structure */ 
{
      /* Initialize unit to the minor device number */ 
      register int unit = minor(dev); 

      /* Call physio to perform buffer lock, buffer check, and */
      /* I/O package set up. */ 
      return (physio(xxstrategy, &xxbuf[unit], dev, B_READ, minphys, uio));
}

/********* Write Routine **********************************/
/*                                                        */
/* The xxwrite routine is called from the ULTRIX          */
/* spec_rwgp routine.  The xxwrite routine will call      */
/* the ULTRIX physio routine to perform the buffer        */
/* lock, buffer check, I/O package set up.                */
/* The physio routine calls the xxstrategy routine        */
/* to access the device.                                  */
/*                                                        */
/**********************************************************/
xxwrite(dev, uio)
dev_t  dev; /* Major/minor device number */
struct uio *uio; /* Pointer to uio structure */
{
      /* Initialize unit to the minor device number */
      register int unit = minor(dev); 

      /* Call physio to perform buffer lock, buffer check, and */
      /* I/O package set up. */ 
      return (physio(xxstrategy, &xxbuf[unit], dev, B_WRITE, minphys, uio));
}

/**********************************************************/
/*         STRATEGY                                       */
/*                                                        */
/**********************************************************/
/*                                                        */
/********* Strategy Routine *******************************/
/*                                                        */
/* The xxstrategy routine is called from the ULTRIX       */
/* physio routine.  The xxstrategy routine first makes    */
/* sure that the user buffer is both readable and         */
/* writeable.  It then determines if the buffer size      */
/* is larger than XXMAXPHYS.                              */
/**********************************************************/
xxstrategy(bp)
struct buf  *bp; /* Pointer to buf structure */ 
{
      /**************************************************/
      /* Declare and initialize: unit variable, pointer */
      /* to uba_device structure, pointer to xx_softc   */
      /* structure, pointer to xx_reg structure, and    */
      /* csr variable.                                  */
      /************************************************/
      register int unit = minor(bp->b_dev); 
      register struct uba_device *devptr = xxdinfo[unit];  
      register struct xx_softc *sc = &xx_softc[unit]; 
      register struct xx_reg *reg = (struct xx_reg *) devptr->ui_addr; 
      short csr; 

       /* Determine if the user buffer is writeable */
       /* during write operations and readable      */
       /* during read operations.                   */ 
      if (useracc(bp->b_un.b_addr, (int) bp->b_bcount,
         ((bp->b_flags & B_READ)==B_READ?B_READ:B_WRITE)) 
           == NULL){
               
              bp->b_error = EACCFAULT; /* Access violation */
              sc->sc_error = bp->b_error; /* A copy to sc_error */
              bp->b_flags |= B_ERROR; /* Flag the error */
              
              /* to xxstrategy                         */ 
              iodone(bp); /* Complete the I/O and return execution */
	                  /* to xxstrategy                         */
              return;
        }

      /* Determine if the buffer size is larger than */
      /* XXMAXPHYS */ 
      if (bp->b_bcount > XXMAXPHYS) {
              bp->b_error = EBUFTOOBIG; /* Indicate error */
              sc->sc_error = bp->b_error; /* A copy to the xx_softc struct */
              bp->b_flags |= B_ERROR; 
              /* Complete the I/O and return execution */
              /* to xxstrategy                         */ 
              iodone(bp); 
              return;
        }

        /* Save bp for use in interrupt routine */ 
        sc->bp = bp;

        /* Set up the DMA mapping registers */ 
        sc->vmeaddr = vbasetup (devptr->ui_vbahd, bp,
                                VME_DMA | VMEA32D32 | VME_BS_NOSWAP,
                                0);

        /* If requested mapping could not be performed */ 
        if (sc->vmeaddr == 0) {
                bp->b_error = ENOMAPREG;
                sc->sc_error = bp->b_error;
                bp->b_flags |= B_ERROR;
                iodone(bp);
                return;
        }

         /* If requested mapping could be performed, */
         /* set up the device for transfer.          */ 
        reg->addr = sc->vmeaddr;
        reg->count = bp->b_bcount;
        if (bp->b_flags & B_READ)
                csr = READ | IE;
        else
                csr = IE;
        reg->csr = csr | DMA_GO;
        wbflush();
}

/**********************************************************/
/*         INTERRUPT                                      */
/*                                                        */
/**********************************************************/
/*                                                        */
/********* Interrupt Routine ******************************/
/*                                                        */
/*                                                        */
/* The xxintr routine is the interrupt service routine    */
/* for the XX device.  It releases VMEbus mapping         */
/* registers and flushes the cache if the operation was   */
/* a DMA read.  It then calls iodone to finish the I/O.   */ 
/**********************************************************/
xxintr(unit)
int  unit; /* Logical unit number for device */  
{
      /**************************************************/
      /* Declare and initialize: pointer to uba_device  */
      /* structure, pointer to xx_softc structure,      */
      /* and pointer to xx_reg structure.  Declare      */
      /* pointer to buf structure.                      */
      /**************************************************/
        register struct uba_device *devptr = xxdinfo[unit]; 
        register struct xx_softc *sc = &xx_softc[unit]; 
        register struct xx_reg *reg = (struct xx_reg *) devptr->ui_addr; 
        struct buf *bp; 

        bp = sc->bp; /* Retrieve saved buf pointer */

        /* To Be Supplied. Is reg->sc_csr correct ? */ 
        if (reg->csr & ERROR) {
                bp->b_error = EIO;
                bp->b_flags |= B_ERROR;
        }
        /* Record the number of bytes remaining */ 
        bp->b_resid = reg->count;       

        /* Release the mapping registers. */ 
        vbarelse(devptr->ui_vbahd, sc->vmeaddr);

        /* If the operation was a read, then it is necessary */
        /* to flush the data cache to ensure that the next */
        /* access will get the newly read data. */ 
        if (bp->b_flags & B_READ) bufflush(bp);
        iodone(bp); 
}

