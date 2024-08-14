#ifdef lint
static char *sccsid = "@(#)newwin.c	4.1	(ULTRIX)	7/2/90";
#endif lint

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 * allocate space for and set up defaults for a new _window
 *
 * 1/26/81 (Berkeley).  This used to be newwin.c
 */

WINDOW *
newwin(nlines, ncols, by, bx)
register int	nlines, ncols, by, bx;
{
	register WINDOW	*win;
	register chtype	*sp;
	register int i;
	char *calloc();

	if (by + nlines > LINES)
		nlines = LINES - by;
	if (bx + ncols > COLS)
		ncols = COLS - bx;

	if (nlines == 0)
		nlines = LINES - by;
	if (ncols == 0)
		ncols = COLS - bx;

	if ((win = makenew(nlines, ncols, by, bx)) == NULL)
		return NULL;
	for (i = 0; i < nlines; i++)
		if ((win->_y[i] = (chtype *) calloc(ncols, sizeof (chtype))) == NULL) {
			register int j;

			for (j = 0; j < i; j++)
				free((char *)win->_y[j]);
			free((char *)win->_firstch);
			free((char *)win->_lastch);
			free((char *)win->_y);
			free((char *)win);
			return NULL;
		}
		else
			for (sp = win->_y[i]; sp < win->_y[i] + ncols; )
				*sp++ = ' ';
	return win;
}
