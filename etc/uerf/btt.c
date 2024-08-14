#ifndef lint
static char sccsid[]  =  "@(#)btt.c	1.8   (ULTRIX)   12/16/86"; 
#endif  lint

/*
*	.TITLE	BTT - Does the final output processing
*	.IDENT	/1-001/
*
*
* COPYRIGHT (C) 1985 DIGITAL EQUIPMENT CORP.,
* CSSE SOFTWARE ENGINEERING
* MARLBOROUGH, MASSACHUSETTS
*
* THIS SOFTWARE IS FURNISHED UNDER A LICENSE FOR USE ONLY ON A 
* SINGLE COMPUTER SYSTEM AND MAY BE COPIED ONLY WITH THE INCLUSION
* OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE,  OR ANY OTHER
* COPIES THEREOF, MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE
* TO ANY OTHER PERSON EXCEPT FOR USE ON SUCH SYSTEM AND TO ONE WHO
* AGREES TO THESE LICENSE TERMS.  TITLE TO AND OWNERSHIP OF THE
* SOFTWARE SHALL AT ALL TIMES REMAIN IN DEC.
*
* THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
* NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
* EQUIPMENT CORPORATION.
*
* DEC ASSUMES NO RESPONSIBILITY FOR THE USE OR RELIABILITY OF
* ITS SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DEC.
*
*++
*
* FACILITY:		FMA - Event Information Management System
*
* ABSTRACT:
*
*	This module does the final output processing for the bit_to_text
*	process.  It receives each segment from UERF and transforms
*	it to its final output format and writes it to stdout.
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C
*
* AUTHOR:  Bob Winant,  CREATION DATE:  05-Dec-85
*
* MODIFIED BY:
*
*	Luis Arce	24-Jan-86	complete initial development
*
*
*--
*/

#include <stdio.h>
#include <sys/time.h>
#include "ueliterals.h"
#include "uestruct.h"
#include "btliterals.h"
#include "erms.h"
#include "generic_dsd.h"
#include "std_dsd.h"

DD$STD_DSD_CTX ue_dsd_ctx;


/*
*	.SBTTL	BT$PUT
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is the entry point to BTT output processing.
*	It receives a pointer to a standard segment and does
*	minimal processing.  The segment and a segment descriptor
*	is then passed to the segment processing routine to 
*	continue output processing.
*	
* CALLING SEQUENCE:		CALL BT$PUT (..See Below..)
*
* FORMAL PARAMETERS:		seg - A pointer to the segment to
*				be processed
*
*
* IMPLICIT INPUTS:		A standard EIMS record
*
* IMPLICIT OUTPUTS:		The final output form of the event
*
* ROUTINE VALUE:		invalid_field (error)
*				good_field (success)
*	
* SIDE EFFECTS:			Cleans out any garbage that might be in
*				the output buffer
*
*--
*/
/*...	ROUTINE BT$PUT (seg)				    */

long  bt$put (seg)
DD$STD_HEADER *seg;			/* use eis seg to get header	*/

{
long  status;

extern struct in_struc in_st;

ue_dsd_ctx.segment_ptr = seg;
if ((status = get_std_segment_dsd(&ue_dsd_ctx)) == TRUE)
    {
    if (in_st.out_form != UE$OUT_TERSE)
        ue_dsd_ctx.user_1 = BT$NEW_SEG;
    do
	{
	if (status == DD$SUCCESS)
	    output_fld(&ue_dsd_ctx);
	}
    while ((status = get_next_item_dsd(&ue_dsd_ctx))
	!= DD$END_OF_SEGMENT);
    }
return(status);
}

/*...	ENDROUTINE  BT$PUT				    */



/*
*	.SBTTL	BT$OPEN - sets up for processing
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine does necessary preparations for BTT
*	
* CALLING SEQUENCE:		CALL BT$OPEN (..See Below..)
*
* FORMAL PARAMETERS:		NONE
*
*
* IMPLICIT INPUTS:		Any structures or processes used by BTT
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		NONE
*	
* SIDE EFFECTS:			
*
*--
*/
/*...	ROUTINE  BT$OPEN()					    */

long  bt$open ()
{
return(BT$SUCC);   
}

/*...	ENDROUTINE BT$OPEN					    */

/*
*	.SBTTL	BT$CLOSE - Cleans up any thing necessary
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine closes and cleans up anything related to BTT
*	
* CALLING SEQUENCE:		CALL BT$CLOSE (..See Below..)
*
* FORMAL PARAMETERS:		NONE
*
*
* IMPLICIT INPUTS:		Any structures or processes used by BTT
*
* IMPLICIT OUTPUTS:		NONE
*
* ROUTINE VALUE:		NONE
*	
* SIDE EFFECTS:			
*
*--
*/
/*...	ROUTINE  BT$CLOSE()					    */

long  bt$close ()
{
return(UE$SUCC);   
}

/*...	ENDROUTINE BT$CLOSE					    */


/*
*	.SBTTL	PUT_TERSE
*++
* FUNCTIONAL DESCRIPTION:		
*
*	This routine is the entry point to BTT output processing with
*	terse type output.
*	It receives pointers to standard eis and dis segments and does
*	processing for the eis and dis.
*	
* CALLING SEQUENCE:		CALL PUT_TERSE  (..See Below..)
*
* FORMAL PARAMETERS:		eisseg - A pointer to the eis
* 				disseg - A pointer to the dis
*
*
* IMPLICIT INPUTS:		A standard EIMS record
*
* IMPLICIT OUTPUTS:		The final output form of the event
*
* ROUTINE VALUE:		
*				
*	
* SIDE EFFECTS:	
*
*--
*/
/*...	ROUTINE PUT_TERSE (peis,pdis)			*/

long  put_terse(peis,pdis)
DD$STD_HEADER *peis, *pdis;		/* use eis seg to get header	*/

{

long   status;
long  *datetime;
long  eventype;
char  *eventtype;
char  *devtype;
char  *syndrome;
char  *cont_lbl;
char  *unit_lbl;
short cont_val;
short unit_val;

/*******************  EIS  **************************************/

ue_dsd_ctx.segment_ptr = peis;
if ((status = get_std_segment_dsd(&ue_dsd_ctx)) != DD$SUCCESS)
    	return(status);
do
    {
    if (status == DD$SUCCESS)
	{
	switch (ue_dsd_ctx.item_DSD_ptr->ID)
	    {
	    case DD$eventtype:
		eventype  = *(long *) ue_dsd_ctx.item_ptr;
		eventtype = (char *) decode_register_field
			(&ue_dsd_ctx,eventype);
		break;
	    case DD$datetime:
		datetime  = (long *) ue_dsd_ctx.item_ptr;
		break;
            }
	}
     }
while ((status = get_next_item_dsd(&ue_dsd_ctx)) !=
		DD$END_OF_SEGMENT);

/*******************  DIS  **************************************/

ue_dsd_ctx.segment_ptr = pdis;
if ((status = get_std_segment_dsd(&ue_dsd_ctx)) != DD$SUCCESS)
    	return(status);
do
    {
    if (status == DD$SUCCESS)
	{
	switch (ue_dsd_ctx.item_DSD_ptr->ID)
	    {
	    case DD$devtype:
		devtype = (char *) decode_std_item
			(&ue_dsd_ctx,*ue_dsd_ctx.item_ptr);
		break;
	    case DD$controller:
		if (ue_dsd_ctx.item_VALID_code == DD$VALID)
		    {
		    cont_lbl = (char *) ue_dsd_ctx.item_DSD_ptr->LABEL;
		    cont_val = *((short *)ue_dsd_ctx.item_ptr);
		    }
		else
		    cont_val = -1;
		break;
	    case DD$unitnumber:
		if (ue_dsd_ctx.item_VALID_code == DD$VALID)
		    {
		    unit_lbl = (char *) ue_dsd_ctx.item_DSD_ptr->LABEL;
		    unit_val = *((short *)ue_dsd_ctx.item_ptr);
		    }
		else
		    unit_val = -1;
		break;
	    case DD$coarsesyndrome:
		syndrome = (char *) decode_std_item
			(&ue_dsd_ctx,*ue_dsd_ctx.item_ptr);
		break;
            }
	}
     }
while ((status = get_next_item_dsd(&ue_dsd_ctx)) !=
		DD$END_OF_SEGMENT);
ue_dsd_ctx.user_1 = BT$NEW_SEG;

if (eventype == 200 ||			/* Do not print		*/
    eventype == 250 ||			/* segment		*/
    eventype == 252 ||			/* header		*/
    eventype == 350 ||
    eventype == 351)
    {
    return(UE$SUCC);
    }

printf("%s:",eventtype);
printf("%*s",BT$F1_LEN-strlen(eventtype)-1,BT$SPACE);

if (eventype == 251 ||
    eventype == 310)
    {
    printf("   %-24.24s",ctime(datetime));
    return(UE$SUCC);
    }

if (eventype == 300 ||
    eventype == 301)
    {
    printf("   %-24.24s\n",ctime(datetime));
    return(UE$SUCC);
    }
printf("%*.*s",BT$F2_LEN,BT$F2_LEN,devtype);
printf("%*s",BT$F3_LEN,BT$SPACE);
printf("%-*.*s",BT$F4_LEN,BT$F4_LEN,syndrome);
if (cont_val != -1)
    {
    printf("\n%-*.*s",BT$F1_LEN,BT$F1_LEN,cont_lbl);
    printf("%*d.",BT$F2_LEN-1,cont_val);
    }
if (unit_val != -1)
    {
    printf("\n%-*.*s",BT$F1_LEN,BT$F1_LEN,unit_lbl);
    printf("%*s",BT$F2_LEN-5,BT$SPACE);
    printf("x%04X",unit_val);
    }
printf("\n");
}

/*...	ENDROUTINE  PUT_TERSE			*/


