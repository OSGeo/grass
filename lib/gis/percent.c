/**
 * \file percent.c
 *
 * \brief Percentage progress functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <stdio.h>
#include <grass/gis.h>


static int prev = -1;
static int first = 1;


/**
 * \fn int G_percent (long n, long d, int s)
 *
 * \brief Print percent complete messages.
 *
 * This routine prints a percentage complete message to stderr. The
 * percentage complete is <i>(<b>n</b>/<b>d</b>)*100</i>, and these are 
 * printed only for each <b>s</b> percentage. This is perhaps best 
 * explained by example:
\code
  #include <stdio.h>
  #include <grass/gis.h>
  int row;
  int nrows;
  nrows = 1352; // 1352 is not a special value - example only

  fprintf (stderr, "Percent complete: ");
  for (row = 0; row < nrows; row++)
  {
      G_percent (row, nrows, 10);
  }
  G_percent (row, nrows, 10);
\endcode
 * This will print completion messages at 10% increments; i.e., 10%, 20%, 30%,
 * etc., up to 100%. Each message does not appear on a new line, but rather erases
 * the previous message. After 100%, a new line is printed.
 *
 * \param[in] n current element
 * \param[in] d total number of elements
 * \param[in] s increment size
 * \return always returns 0
 */

int G_percent (long n,long d,int s)
{
    return ( G_percent2 ( n, d, s, stderr ) );
}


/**
 * \fn int G_percent2 (long n, long d, int s, FILE *out)
 *
 * \brief Print percent complete messages.
 *
 * This routine prints a percentage complete message to stderr. The
 * percentage complete is <i>(<b>n</b>/<b>d</b>)*100</i>, and these are 
 * printed only for each <b>s</b> percentage. This is perhaps best 
 * explained by example:
\code
  #include <stdio.h>
  #include <grass/gis.h>
  int row;
  int nrows;
  nrows = 1352; // 1352 is not a special value - example only
  fprintf (stderr, "Percent complete: ");
  for (row = 0; row < nrows; row++)
  {
      G_percent (row, nrows, 10);
  }
  G_percent (row, nrows, 10);
\endcode
 * This will print completion messages at 10% increments; i.e., 10%, 20%, 30%,
 * etc., up to 100%. Each message does not appear on a new line, but rather erases
 * the previous message. After 100%, a new line is printed.
 *
 * \param[in] n current element
 * \param[in] d total number of elements
 * \param[in] s increment size
 * \param[in,out] out file to print to
 * \return always returns 0
 */

int G_percent2 (long n,long d,int s, FILE *out)
{
    int x, format;

    format = G_info_format ();

    x = (d <= 0 || s <= 0)
	? 100
	: (int) (100 * n / d);

    /* be verbose only 1> */
    if (format == G_INFO_FORMAT_SILENT || G_verbose() < 1)
        return 0;

    if (n <= 0 || n >= d || x > prev + s)
    {
	prev = x;
        
	if ( format == G_INFO_FORMAT_STANDARD ) {
	    if ( out != NULL ) {
	        fprintf (out,"%4d%%\b\b\b\b\b",x);
	    }
	} else { /* GUI */
	    if ( out != NULL ) {
		if ( first ) {
		    fprintf (out,"\n");
		}
		fprintf (out,"GRASS_INFO_PERCENT: %d\n", x);
		fflush ( out );
	    }
	    first = 0;
	}
    }

    if (x >= 100)
    {
	if ( format == G_INFO_FORMAT_STANDARD ) {
	    if ( out != NULL ) {
	        fprintf (out,"\n");
	    }
	}
	prev = -1;
	first = 1;
    }

    return 0;
}


/**
 * \fn int G_percent_reset (void)
 *
 * \brief Reset G_percent() to 0%; do not add newline.
 *
 * \return always returns 0
 */

int G_percent_reset(void)
{
    prev = -1;
    first = 1;

    return 0;
}
