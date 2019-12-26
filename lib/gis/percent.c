
/*!
  \file lib/gis/percent.c
  
  \brief GIS Library - percentage progress functions.
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author GRASS GIS Development Team
*/

#include <stdio.h>
#include <grass/gis.h>

static struct state {
    int prev;
    int first;
} state = {-1, 1};

static struct state *st = &state;
static int (*ext_percent) (int);

/*!
  \brief Print percent complete messages.
  
  This routine prints a percentage complete message to stderr. The
  percentage complete is <i>(<b>n</b>/<b>d</b>)*100</i>, and these are 
  printed only for each <b>s</b> percentage. This is perhaps best 
  explained by example:
  \code
  #include <stdio.h>
  #include <grass/gis.h>
  int row;
  int nrows;
  nrows = 1352; // 1352 is not a special value - example only

  G_message(_("Percent complete..."));
  for (row = 0; row < nrows; row++)
  {
      G_percent(row, nrows, 10);
      do_calculation(row);
  }
  G_percent(1, 1, 1);
  \endcode
 
  This example code will print completion messages at 10% increments;
  i.e., 0%, 10%, 20%, 30%, etc., up to 100%. Each message does not appear
  on a new line, but rather erases the previous message.
  
  Note that to prevent the illusion of the module stalling, the G_percent()
  call is placed before the time consuming part of the for loop, and an
  additional call is generally needed after the loop to "finish it off"
  at 100%.
  
  \param n current element
  \param d total number of elements
  \param s increment size
*/
void G_percent(long n, long d, int s)
{
    int x, format;

    format = G_info_format();

    x = (d <= 0 || s <= 0)
	? 100 : (int)(100 * n / d);

    /* be verbose only 1> */
    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
	return;

    if (n <= 0 || n >= d || x > st->prev + s) {
	st->prev = x;

	if (ext_percent) {
	    ext_percent(x);
	}
	else {
	    if (format == G_INFO_FORMAT_STANDARD) {
		fprintf(stderr, "%4d%%\b\b\b\b\b", x);
	    }
	    else {
		if (format == G_INFO_FORMAT_PLAIN) {
		    if (x == 100)
			fprintf(stderr, "%d\n", x);
		    else
			fprintf(stderr, "%d..", x);
		}
		else {		/* GUI */
		    if (st->first) {
			fprintf(stderr, "\n");
		    }
		    fprintf(stderr, "GRASS_INFO_PERCENT: %d\n", x);
		    fflush(stderr);
		    st->first = 0;
		}
	    }
	}
    }

    if (x >= 100) {
	if (ext_percent) {
	    ext_percent(100);
	}
	else if (format == G_INFO_FORMAT_STANDARD) {
	    fprintf(stderr, "\n");
	}
	st->prev = -1;
	st->first = 1;
    }
}

/*!
  \brief Reset G_percent() to 0%; do not add newline.
*/
void G_percent_reset(void)
{
    st->prev = -1;
    st->first = 1;
}

/*!
  \brief Print progress info messages

  Use G_percent() when number of elements is defined.
  
  This routine prints a progress info message to stderr. The value
  <b>n</b> is printed only for each <b>s</b>. This is perhaps best
  explained by example:
  \code
  #include <grass/vector.h>

  int line;

  G_message(_("Reading features..."));
  line = 0;
  while(TRUE)
  {
      if (Vect_read_next_line(Map, Points, Cats) < 0)
          break;
      line++;
      G_progress(line, 1e3);
  }
  G_progress(1, 1);
  \endcode
 
  This example code will print progress in messages at 1000
  increments; i.e., 1000, 2000, 3000, 4000, etc., up to number of
  features for given vector map. Each message does not appear on a new
  line, but rather erases the previous message.
  
  \param n current element
  \param s increment size
  
  \return always returns 0
*/
void G_progress(long n, int s)
{
    int format;
    
    format = G_info_format();
    
    /* be verbose only 1> */
    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
	return;
    
    if (n == s && n == 1) {
	if (format == G_INFO_FORMAT_PLAIN)
	    fprintf(stderr, "\n");
        else if (format != G_INFO_FORMAT_GUI)
	    fprintf(stderr, "\r");
	return;
    }

    if (n % s == 0) {
	if (format == G_INFO_FORMAT_PLAIN)
	    fprintf(stderr, "%ld..", n);
	else if (format == G_INFO_FORMAT_GUI)
            fprintf(stderr, "GRASS_INFO_PROGRESS: %ld\n", n);
        else
	    fprintf(stderr, "%10ld\b\b\b\b\b\b\b\b\b\b", n);
    }
}

/*!
  \brief Establishes percent_routine as the routine that will handle
  the printing of percentage progress messages.
  
  \param percent_routine routine will be called like this: percent_routine(x)
*/
void G_set_percent_routine(int (*percent_routine) (int))
{
    ext_percent = percent_routine;
}

/*!
  \brief After this call subsequent percentage progress messages will
  be handled in the default method.
  
  Percentage progress messages are printed directly to stderr.
*/
void G_unset_percent_routine(void)
{
    ext_percent = NULL;
}
