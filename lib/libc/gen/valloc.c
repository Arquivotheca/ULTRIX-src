#ifndef lint
static char sccsid[] = "@(#)valloc.c	4.2	(ULTRIX)	9/11/90";
#endif

char	*malloc();

char *
valloc(i)
	int i;
{
	int valsiz = getpagesize(), j;
	char *cp = malloc(i + (valsiz-1));

	j = ((int)cp + (valsiz-1)) &~ (valsiz-1);
	return ((char *)j);
}
