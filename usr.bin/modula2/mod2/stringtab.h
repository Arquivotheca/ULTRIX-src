(*#@(#)stringtab.h	1.1	Ultrix	11/28/84 *)
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
$Header: stringtab.h,v 1.4 84/05/19 11:44:47 powell Exp $
 ****************************************************************************)
{ string table stuff }
const
    STRINGBLOCKSIZE = 10000;
    MAXSTRINGSIZE = 1000;
    STRINGHASHSIZE = 1357;
    SHORTSTRINGSIZE = 16;

type
    StringIndex = 0..STRINGBLOCKSIZE;
    StringLength = 0..MAXSTRINGSIZE;
    StringBlock = ^ StringBlockRec;
    StringBlockRec = record;
	next : StringBlock;
	s : array [StringIndex] of character;
    end;
    StringEntry = ^ StringEntryRec;
    StringEntryRec = record
	block : StringBlock;
	index : StringIndex;
	length : StringLength;
	hash : integer;			{ hash value for quick comparisons }
	next : StringEntry;
    end;
    String = StringEntry;
    ShortString = array [1..SHORTSTRINGSIZE] of char;

procedure InitStringTable; external;

function NewString : String; external;

procedure AddChar(c : char); external;

procedure AddCharX(c : character); external;

function GetChar(s : String; charNum : StringLength) : char; external;

function GetCharX(s : String; charNum : StringLength) : character; external;

procedure AddText(s : ShortString); external;

procedure AddString(str : String); external;

procedure WriteString(var f : text; s : String); external;

procedure WriteStringConst(var f : text; s : String); external;

procedure StringToFileName(s : String; var fn : FileName); external;

procedure DumpStringTab; external;

function EqualAnyCase(a,b : String) : boolean; external;
