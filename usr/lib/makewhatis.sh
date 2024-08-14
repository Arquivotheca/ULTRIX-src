#!/bin/sh
#
# @(#)makewhatis.sh	4.1	(ULTRIX)	7/2/90
#
# This script creates the /usr/lib/whatis file (the "whatis" database).  It is
# executed by "catman" by default, or explicitly by "catman -w".
#
# Constants
#
HOME=/;
PATH=/bin:/usr/ucb:/usr/bin
export HOME PATH
#
LIMIT=200			# number of files to process at a time
MANHOME=/usr/man		# reference pages home directory
WHATIS=/usr/lib/whatis		# location of "whatis" database
#
# remove temp files in case they already exist
rm -f /tmp/whatis /tmp/whatis2 /tmp/whatis$$*
#
# extract "name[,...] \- description" information from all reference pages
#   directories
# reference pages directories are assumed to be only in /usr/man, with names
#   beginning with "man"
#
cd $MANHOME
mandirs=`ls`
#
# for each file in $MANHOME/man?/*, getNAME extracts the .TH line of
# each reference page followed by the contents of the text between the first
# .SH in the reference page and the second .SH .
#
for i in ${mandirs}
do
	case $i in

		man*)

			# Some reference pages directories are very large
			# so "xargs" is used to scan reference pages in
			# smaller chunks at a time.
			#
			cd $i
			ls | xargs -n$LIMIT /usr/lib/getNAME
			cd ..
			;;

		*)
			;;
	esac
done >/tmp/whatis
#
# editing the output generated by getNAME
#
# getNAME returns lines have the general syntax:
#	.TH name section .... <tab>name[, name2[, name3 ...]..] $ description
# where $ = a separator string.  Valid separator strings are:
#	<blank>-<blank>
#	<blank>\-<blank>
#	<blank>\*-<blank>	where * = 1 or more blanks or characters
#
# ex performs the following operations on each line of output from getNAME:
#	1. replaces \-<blank>	with @
#	2. replaces \*-<blank>	with @
#	3. replaces -<blank>	with @
#	4. removes " VAX-11"
#	5. replaces ".TH <name> <section>[ <tab>]......<tab><list> ",
#		terminated by @ but not including the @ ,
#		with "<list> (<section>)<tab>-<blank>" . 
#	6. replaces @ (if any)	with -<blank>
#	7. replaces "<tab><space>" with "<tab>"
# The ex commands are embedded between !'s.  ex places the edited output in
# /tmp/whatis2 .
# 
ex - /tmp/whatis <<\!
g/\\- /s//@/
g/\\\*-/s//@/
g/- /s//@/
g/ VAX-11/s///
1,$s/^\.TH [^ ]* \([^ 	]*\).*	\([^@]*\)@/\2(\1)	- /
g/@/s//- /
g/	 /s//	/g
w /tmp/whatis2
q
!
#
# change tabs into blanks before sorting, sort with Upper/lower-case folding,
# then change blanks back to tabs before the final installation of the
# "whatis" database.
#
/usr/ucb/expand -24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100 /tmp/whatis2 | sort -f >/tmp/whatis$$
/usr/ucb/unexpand -a /tmp/whatis$$ > $WHATIS
chmod 644 $WHATIS
rm -f /tmp/whatis /tmp/whatis$$ /tmp/whatis2
