(*#@(#)error.h	1.1	Ultrix	11/28/84 *)
(****************************************************************************
 *									    *
 *  Copyright (c) 1984 by						    *
 *  DIGITAL EQUIPMENT CORPORATION, Maynard, Massachusetts.		    *
 *  All rights reserved.						    *
 * 									    *
 *  This software is furnished under a license and may be used and copied   *
 *  only in  accordance with  the  terms  of  such  license  and with the   *
 *  inclusion of the above copyright notice. This software or  any  other   *
 *  copies thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to and ownership of  the  software is  hereby   *
 *  transferred.							    *
 * 									    *
 *  The information in this software is  subject to change without notice   *
 *  and  should  not  be  construed as  a commitment by DIGITAL EQUIPMENT   *
 *  CORPORATION.							    *
 * 									    *
 *  DIGITAL assumes no responsibility for the use  or  reliability of its   *
 *  software on equipment which is not supplied by DIGITAL.		    *
 * 									    *
$Header: error.h,v 1.3 84/05/19 11:32:59 powell Exp $
 ****************************************************************************)
const
    ERRORSTRINGSIZE = 40;
type
    ErrorString = array [1..ERRORSTRINGSIZE] of char;

procedure Error(s:ErrorString); external;
