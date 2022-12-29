/*
 * Start up graphics processing.  Anything that needs to be assigned, set up,
 * started-up, or otherwise initialized happens here.  This is called only at
 * the startup of the graphics driver.
 *
 * The external variables define the pixle limits of the graphics surface.  The
 * coordinate system used by the applications programs has the (0,0) origin
 * in the upper left-hand corner.  Hence,
 *    screen_left < screen_right
 *    screen_top  < screen_bottom 
 *
 */

#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "driverlib.h"
#include "driver.h"
#include "htmlmap.h"

struct html_state html;

int HTML_Graph_set(void)
{
    char *file_name;
    char *p;

    G_gisinit("HTMLMAP driver");

    /*
     * set the minimum bounding box dimensions 
     */

    if (NULL != (p = getenv("GRASS_RENDER_HTMLMINBBOX"))) {
	html.BBOX_MINIMUM = atoi(p);
	if (html.BBOX_MINIMUM <= 0) {
	    html.BBOX_MINIMUM = DEF_MINBBOX;
	}
    }
    else {
	html.BBOX_MINIMUM = DEF_MINBBOX;
    }

    /*
     * set the maximum number of points
     */

    if (NULL != (p = getenv("GRASS_RENDER_HTMLMAXPOINTS"))) {
	html.MAX_POINTS = atoi(p);
	if (html.MAX_POINTS <= 0) {
	    html.MAX_POINTS = DEF_MAXPTS;
	}
    }
    else {
	html.MAX_POINTS = DEF_MAXPTS;
    }

    /*
     * set the minimum difference to keep a point
     */

    if (NULL != (p = getenv("GRASS_RENDER_HTMLMINDIST"))) {
	html.MINIMUM_DIST = atoi(p);
	if (html.MINIMUM_DIST <= 0) {
	    html.MINIMUM_DIST = DEF_MINDIST;
	}
    }
    else {
	html.MINIMUM_DIST = DEF_MINDIST;
    }


    /*
     * open the output file
     */

    if (NULL != (p = getenv("GRASS_RENDER_FILE"))) {
	if (strlen(p) == 0) {
	    p = FILE_NAME;
	}
    }
    else {
	p = FILE_NAME;
    }
    file_name = p;

    html.output = fopen(file_name, "w");
    if (html.output == NULL) {
	G_fatal_error("HTMLMAP: couldn't open output file %s", file_name);
	exit(EXIT_FAILURE);
    }


    G_verbose_message(_("html: collecting to file '%s'"), file_name);
    G_verbose_message(_("html: image size %dx%d"),
		      screen_width, screen_height);

    /*
     * check type of map wanted
     */

    if (NULL == (p = getenv("GRASS_RENDER_HTMLTYPE"))) {
	p = "CLIENT";
    }
    
    if (strcmp(p, "APACHE") == 0) {
	html.type = APACHE;
	G_verbose_message(_("html: type '%s'"), "apache");
    }
    else if (strcmp(p, "RAW") == 0) {
	html.type = RAW;
	G_verbose_message(_("html: type '%s'"), "raw");
    }
    else {
	html.type = CLIENT;
	G_verbose_message(_("html: type '%s'"), "client");
    }

    /*
     * initialize text memory and list pointers
     */
    
    html.last_text = (char *)G_malloc(INITIAL_TEXT + 1);
    html.last_text[0] = '\0';
    html.last_text_len = INITIAL_TEXT;

    html.head = NULL;
    html.tail = &html.head;

    return 0;
}
