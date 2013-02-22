/* Functions: do_map_header, read_header_file
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <unistd.h>
#include <string.h>
#include <grass/raster.h>
#include "header.h"
#include "local_proto.h"

static double x, y, dy, fontsize;
static void append(char, char *);
static void apply(const char *, const char *, char *);
static char *get_format(char *, char *);
static int output(char *, const char *);

int do_map_header(const char *date)
{
    char temp[100];

    /* set color and font */
    set_ps_color(&hdr.color);
    fontsize = (double)hdr.fontsize;
    fprintf(PS.fp, "(%s) FN %.1f SF\n", hdr.font, fontsize);

    /* set start of first line */
    dy = 1.2 * fontsize;
    y = 72.0 * (PS.page_height - PS.top_marg) - fontsize - 1.5;

    if (hdr.fp == NULL) {

	if (PS.celltitle[0]) {
	    fprintf(PS.fp, "/t (TITLE:  %s) def\n", PS.celltitle);
	    fprintf(PS.fp, "t SW pop %.1f XS D2 t exch %.1f MS\n",
		    72 * PS.page_width, y);
	    y -= dy;
	}
	strcpy(temp, G_myname());
	G_strip(temp);
	if (*temp == 0)
	    strcpy(temp, G_location());
	fprintf(PS.fp, "/t (LOCATION:  %s) def\n", temp);
	fprintf(PS.fp, "t SW pop %.1f XS D2 t exch %.1f MS\n",
		72 * PS.page_width, y);
	y -= 0.25 * dy;
	if (PS.min_y > y)
	    PS.min_y = y;
	return 0;
    }

    x = 72.0 * PS.left_marg + 1.5;
    read_header_file(date);
    y -= 0.25 * dy;
    if (PS.min_y > y)
	PS.min_y = y;

    return 0;
}


int read_header_file(const char *date)
{
    char buf[1024];

    while (G_getl2(buf, sizeof buf, hdr.fp))
	output(buf, date);
    fclose(hdr.fp);

    return 0;
}


static int output(char *line, const char *date)
{
    char text[1024];
    char fmt[30];
    char *buf;

    buf = line;
    *text = 0;
    while (*buf) {
	if (*buf == '%') {
	    buf++;
	    if (*buf == '%')
		strcat(text, "%");
	    else if (*buf == 'n') {
		if (*text)
		    show_text(x, y, text);
		y -= dy;
		return 0;
	    }
	    else if (*buf == '_') {
		fprintf(PS.fp, "BW ");
		draw_line(x, y + 0.2 * fontsize,
			  72.0 * (PS.page_width - PS.right_marg),
			  y + 0.2 * fontsize);
		y -= dy;
		set_ps_color(&hdr.color);
		return 0;
	    }
	    else {
		buf = get_format(buf, fmt);
		append('s', fmt);
		switch (*buf) {
		case 'd':
		    apply(date, fmt, text);
		    break;
		case 'l':
		    apply(G_location(), fmt, text);
		    break;
		case 'L':
		    apply(G_myname(), fmt, text);
		    break;
		case 'c':
		    if (PS.cell_fd >= 0) {
			char name[100];

			sprintf(name, "<%s> in mapset <%s>",
				PS.cell_name, PS.cell_mapset);
			apply(name, fmt, text);
		    }
		    else
			apply("none", fmt, text);
		    break;
		case 'm':
		    apply(G_mapset(), fmt, text);
		    break;
		case 'u':
		    apply(G_whoami(), fmt, text);
		    break;
		case 'x':
		    apply(Rast_mask_info(), fmt, text);
		    break;
		case 0:
		    continue;
		}
	    }
	}
	else
	    append(*buf, text);
	buf++;
    }
    if (*text)
	show_text(x, y, text);
    y -= dy;

    return 0;
}


static char *get_format(char *buf, char *fmt)
{
    strcpy(fmt, "%");
    if (*buf == '-') {
	append(*buf++, fmt);
	if (*buf < '1' || *buf > '9')
	    return (buf - 1);
    }
    while (*buf >= '0' && *buf <= '9')
	append(*buf++, fmt);
    return buf;
}


static void append(char c, char *buf)
{
    while (*buf)
	buf++;
    *buf++ = c;
    *buf = 0;
}

static void apply(const char *buf, const char *fmt, char *text)
{
    char temp[300];

    sprintf(temp, fmt, buf);
    strcat(text, temp);
}
