/* Function to draw scalebar on page */

#include <string.h>
#include <math.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "distance.h"

#define LEFT 0
#define RIGHT 1
#define LOWER 0
#define UPPER 1
#define CENTER 2

static char *nice_number(double);

int do_scalebar(void)
{
    double scale_size;
    double length, width;
    double x, x1, x2, y1, y2, y3;
    int seg, i, j, lab;
    int margin;
    char num[50];

    /* get scale size */
    scale_size =
	METERS_TO_INCHES * distance(PS.w.east, PS.w.west) / scale(PS.scaletext);

    /* convert scale size to map inches */
    length = (sb.length / scale_size) *
	G_database_units_to_meters_factor() * METERS_TO_INCHES;

    /* if(sb.units == SB_UNITS_AUTO) { do nothing } */
    if(sb.units == SB_UNITS_METERS)
	    length /= G_database_units_to_meters_factor();
    else if(sb.units == SB_UNITS_KM)
	    length *= KILOMETERS_TO_METERS / G_database_units_to_meters_factor();
    else if(sb.units == SB_UNITS_FEET)
	    length *= FEET_TO_METERS / G_database_units_to_meters_factor();
    else if(sb.units == SB_UNITS_MILES)
	    length *= MILES_TO_METERS / G_database_units_to_meters_factor();
    else if(sb.units == SB_UNITS_NMILES)
	    length *= NAUT_MILES_TO_METERS / G_database_units_to_meters_factor();

    width = sb.height;
    seg = sb.segment;
    j = 0;
    lab = 0;

    margin = (int)(0.2 * (double)sb.fontsize + 0.5);
    if (margin < 2)
	margin = 2;
    fprintf(PS.fp, "/mg %d def\n", margin);
    x = sb.x - (length / 2.);
    set_font_name(sb.font);
    set_font_size(sb.fontsize);
    set_line_width(sb.width);

    if (strcmp(sb.type, "f") == 0) {
	/* draw fancy scale bar */

	for (i = 0; i < seg; i++) {
	    /* draw a filled rectangle */
	    x1 = 72.0 * (x + (length / seg) * i) + 0.5;
	    y1 = 72.0 * (PS.page_height - sb.y);
	    x2 = 72.0 * (x + (length / seg) * (i + 1)) + 0.5;
	    y2 = (72.0 * (PS.page_height - sb.y)) + (width * 72.0);

	    /* Alternate black and white */
	    if (j == 0) {
		fprintf(PS.fp, "0.0 0.0 0.0 C\n");
		j = 1;
	    }
	    else {
		fprintf(PS.fp, "1.0 1.0 1.0 C\n");
		j = 0;
	    }
	    fprintf(PS.fp, "%.1f %.1f %.1f %.1f B\n", x1, y1, x2, y2);

	    /* set outline to black */
	    fprintf(PS.fp, "F 0.0 0.0 0.0 C\n");
	    fprintf(PS.fp, "D\n");

	    lab++;

	    /* do text */
	    if (i == 0 || lab == sb.numbers) {
		sprintf(num, "%s", nice_number((sb.length / sb.segment) * i));
		text_box_path(x1, y2 + margin, CENTER, LOWER, num, 0);
		if (sb.bgcolor) {	/* TODO: take bg color, not just [white|none] */
		    set_rgb_color(WHITE);
		    fprintf(PS.fp, "F ");
		}
		set_rgb_color(sb.color);
		fprintf(PS.fp, "TIB\n");
		lab = 0;
	    }

	    if ((lab > 0 && i == seg - 1) ||
		(sb.numbers == 1 && i == seg - 1)) {
		/* special case for last label */
		sprintf(num, "%s", nice_number(sb.length));
		text_box_path(x2, y2 + margin, CENTER, LOWER, num, 0);
		if (sb.bgcolor) {
		    set_rgb_color(WHITE);
		    fprintf(PS.fp, "F ");
		}
		set_rgb_color(sb.color);
		fprintf(PS.fp, "TIB\n");
	    }

	}

    }
    else {
	/* draw simple scalebar */

	x1 = 72.0 * x + 0.5;
	y1 = (72.0 * (PS.page_height - sb.y)) + (width * 72.0);
	x2 = 72.0 * x + 0.5;
	y2 = 72.0 * (PS.page_height - sb.y);

	fprintf(PS.fp, "%.1f %.1f %.1f %.1f L D\n", x1, y1, x2, y2);

	/* draw label */
	text_box_path(x1, y1 + margin, CENTER, LOWER, "0", 0);
	if (sb.bgcolor) {
	    set_rgb_color(WHITE);
	    fprintf(PS.fp, "F ");
	}
	set_rgb_color(sb.color);
	fprintf(PS.fp, "TIB\n");


	x1 = 72.0 * x + 0.5;
	y1 = 72.0 * (PS.page_height - sb.y);
	x2 = 72.0 * (x + length) + 0.5;
	y2 = 72.0 * (PS.page_height - sb.y);
	fprintf(PS.fp, "%.1f %.1f %.1f %.1f L D\n", x1, y1, x2, y2);

	x1 = 72.0 * (x + length) + 0.5;
	y2 = (72.0 * (PS.page_height - sb.y)) + (width * 72.0);
	x2 = 72.0 * (x + length) + 0.5;
	y1 = 72.0 * (PS.page_height - sb.y);
	fprintf(PS.fp, "%.1f %.1f %.1f %.1f L D\n", x1, y1, x2, y2);

	/* draw label */
	sprintf(num, "%s", nice_number(sb.length));
	text_box_path(x1, y2 + margin, CENTER, LOWER, num, 0);
	if (sb.bgcolor) {
	    set_rgb_color(WHITE);
	    fprintf(PS.fp, "F ");
	}
	set_rgb_color(sb.color);
	fprintf(PS.fp, "TIB\n");


	for (i = 1; i < seg; i++) {
	    x1 = 72.0 * (x + (length / seg) * i) + 0.5;
	    y1 = 72.0 * (PS.page_height - sb.y);
	    x2 = 72.0 * (x + (length / seg) * i) + 0.5;
	    y2 = (72.0 * (PS.page_height - sb.y)) + (width / 2. * 72.0);
	    y3 = (72.0 * (PS.page_height - sb.y)) + (width * 72.0);

	    fprintf(PS.fp, "%.1f %.1f %.1f %.1f L D\n", x1, y1, x2, y2);

	    lab++;

	    /* do text */
	    if (lab == sb.numbers) {
		sprintf(num, "%s", nice_number((sb.length / sb.segment) * i));

		text_box_path(x1, y3 + margin, CENTER, LOWER, num, 0);
		if (sb.bgcolor) {
		    set_rgb_color(WHITE);
		    fprintf(PS.fp, "F ");
		}
		set_rgb_color(sb.color);
		fprintf(PS.fp, "TIB\n");
		lab = 0;
	    }

	}
    }


    /* draw units label */
    if (sb.units == SB_UNITS_AUTO)
	strcpy(num, G_database_unit_name(TRUE));
    else if(sb.units == SB_UNITS_METERS)
	strcpy(num, _("meters"));
    else if(sb.units == SB_UNITS_KM)
	strcpy(num, _("kilometers"));
    else if(sb.units == SB_UNITS_FEET)
	strcpy(num, _("feet"));
    else if(sb.units == SB_UNITS_MILES)
	strcpy(num, _("miles"));
    else if(sb.units == SB_UNITS_NMILES)
	strcpy(num, _("nautical miles"));
    
    text_box_path(72.0 * (x + length/2), 72.0 * (PS.page_height - (sb.y + 0.075)),
	CENTER, UPPER, num, 0);

    if (sb.bgcolor) {
	set_rgb_color(WHITE);
	fprintf(PS.fp, "F ");
    }
    set_rgb_color(sb.color);
    fprintf(PS.fp, "TIB\n");


    return 0;
}

/* Make a nice number -- reduce to less than 3 decimal places */
static char *nice_number(double val)
{
    static char text[50];

    if (val == (int)val)
	sprintf(text, "%.0f", val);
    else if ((val * 10.) == (int)(val * 10))
	sprintf(text, "%.1f", val);
    else if ((val * 100.) == (int)(val * 100))
	sprintf(text, "%.2f", val);
    else
	sprintf(text, "%.3f", val);

    return text;
}
