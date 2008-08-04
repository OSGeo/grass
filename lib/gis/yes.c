
/**
 * \file yes.c
 *
 * \brief GIS Library - Yes/No functions.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <stdio.h>
#include <grass/gis.h>


/**
 * \brief Ask a yes/no question.
 *
 * This routine prints a <b>question</b> to the user, and expects the user to
 * respond either yes or no. (Invalid responses are rejected and the process is
 * repeated until the user answers yes or no.)<br>
 * The <b>default</b> indicates what the RETURN key alone should mean. A
 * <b>default</b> of 1 indicates that RETURN means yes, 0 indicates that RETURN
 * means no. The <b>question</b> will be appended with ''(y/n) '', and, 
 * if <b>default</b> is not -1, with ''[y] '' or ''[n] '', depending on 
 * the <b>default.</b><br>
 *
 * \param[in] question
 * \param[in] dflt
 * \return 0 for No
 * \return 1 for Yes
 */

int G_yes(const char *question, int dflt)
{
    fflush(stdout);

    while (1) {
	char answer[100];

	fprintf(stderr, "%s", question);

	while (1) {
	    fprintf(stderr, "(y/n) ");
	    if (dflt >= 0)
		fprintf(stderr, dflt == 0 ? "[n] " : "[y] ");

	    fflush(stderr);
	    if (!G_gets(answer))
		break;
	    G_strip(answer);

	    switch (*answer) {
	    case 'y':
	    case 'Y':
		return (1);
	    case 'n':
	    case 'N':
		return (0);
	    case 0:
		if (dflt >= 0)
		    return (dflt);
	    }
	}
    }
}
