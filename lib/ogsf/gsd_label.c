/*
* $Id$
*
****************************************************************************
*
* MODULE: 	GRASS ogsf library
* AUTHOR(S):	Original author - Bill Brown - USA CERL 1991 - 1992
*   	    	<new author name here>
* PURPOSE: 	This file needs to be re-written in OpenGL
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/

#include <grass/gstypes.h>
#include "rgbpack.h"
#define MAX_LIST 20

static int first = 0;
GLuint label_base;
GLuint label_id;

void
gs_put_label(char *text, GLuint fontbase, int size, unsigned long color,
	     int*pt)
{
    int txt_width;
    GLint tmp[4];
    float labpt[2];
    int t, l, b, r;

    if (!first) {
	/* initialize display list */
	label_base = glGenLists(MAX_LIST);
	glListBase(label_base);
	label_id = label_base;
	first = 1;
    }

    if (label_id > (label_base + MAX_LIST)) {
	fprintf(stderr, "Max. number of labels reached!\n");
	return;
    }

    glNewList(label_id, GL_COMPILE_AND_EXECUTE);
    txt_width = gsd_get_txtwidth(text, size);

/* adjust to center text string */
    labpt[X] = (float) (pt[X] - txt_width / 2.);
    labpt[Y] = (float) pt[Y];

    glGetIntegerv(GL_VIEWPORT, tmp);
    l = tmp[0];
    r = tmp[0] + tmp[2];
    b = tmp[1];
    t = tmp[1] + tmp[3];

    gsd_bgn_legend_viewport(l, b, r, t);


    /* Set text color */
    gsd_color_func(color);

    do_label_display(fontbase, labpt, text);
    

    gsd_end_legend_viewport();

    glEndList();

    label_id++;

    return;
}


/****************************************************************
 * Remove current label 
****************************************************************/
void gsd_remove_curr(void)
{

    if (label_id) {
	glDeleteLists(label_id - 1, 1);
	label_id--;
    }

    return;

}


/******************************************************************
 * Remove all labels from display list 
******************************************************************/
void gsd_remove_all(void)
{

    glDeleteLists(label_base, MAX_LIST);
    label_id = label_base;

    return;
}

/*************************************************************************
 * Call display list and draw defined labels -- called from gsd_prim (gsd_call_lists)
**************************************************************************/
void gsd_call_label(void)
{
    int i;

    for (i = 0; i < MAX_LIST; i++) {
	glCallList(i + label_base);
	glFlush();
    }
    return;
}
