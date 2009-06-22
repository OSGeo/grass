/* Function: get_font
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <grass/gis.h>

int get_font(char *data)
{
    char *dp;

    G_strip(data);

    /* replace spaces with dashes and make sure each
     ** word begins with a capital letter.
     */
    dp = data;
    if (*dp >= 'a' && *dp <= 'z')
	*dp = *dp - 'a' + 'A';
    while (*dp) {
	if (*dp == ' ') {
	    *dp++ = '-';
	    if (*dp >= 'a' && *dp <= 'z')
		*dp = *dp - 'a' + 'A';
	}
	else
	    dp++;
    }

    return 0;
}
