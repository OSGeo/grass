#include <string.h>
#include <grass/raster.h>
#include "global.h"

static int page = 0;
static const char *date = NULL;
static int max(int, int);

static int pbuf(char *buf)
{
    fprintf(stdout, "|%-*.*s|", page_width - 2, page_width - 2, buf);
    newline();

    return 0;
}

int header(int unit1, int unit2)
{
    int i, k;
    char buf[1024];
    char tbuf1[256], tbuf2[256];
    char north[50], south[50];
    char east[50], west[50];
    char ns_res[50], ew_res[50];
    int len1, len2;
    char *label;
    char *mask;

    nlines = page_length;
    if (date == NULL)
	date = G_date();

    divider("+");
    page++;

    if (page == 1 && with_headers) {
	lcr("", "RASTER MAP CATEGORY REPORT", "", buf, page_width - 2);
	pbuf(buf);
	sprintf(tbuf1, "LOCATION: %s", G_location());
	if (with_headers && (page_length > 0))
	    sprintf(tbuf2, "Page %d", page);
	else
	    *tbuf2 = 0;
	lcr(tbuf1, tbuf2, date, buf, page_width - 2);
	pbuf(buf);

	divider("|");

	G_format_northing(window.north, north, window.proj);
	G_format_easting(window.east, east, window.proj);
	G_format_northing(window.south, south, window.proj);
	G_format_easting(window.west, west, window.proj);
	G_format_resolution(window.ns_res, ns_res, window.proj);
	G_format_resolution(window.ew_res, ew_res, window.proj);
	len1 = max(strlen(north), strlen(south));
	len1 = max(len1, strlen(ns_res));
	len2 = max(strlen(east), strlen(west));
	len2 = max(len2, strlen(ew_res));

	sprintf(buf, "%-9s north: %*s    east: %*s",
		"", len1, north, len2, east);
	pbuf(buf);

	sprintf(buf, "%-9s south: %*s    west: %*s",
		"REGION", len1, south, len2, west);
	pbuf(buf);

	sprintf(buf, "%-9s res:   %*s    res:  %*s",
		"", len1, ns_res, len2, ew_res);
	pbuf(buf);

	divider("|");

	mask = maskinfo();
	label = "MASK:";
	len1 = strlen(label) + 1;
	while (mask) {
	    fprintf(stdout, "|%-*s", len1, label);
	    label = "";
	    mask = print_label(mask, page_width - len1 - 2, 1, 0, ' ');
	    fprintf(stdout, "|");
	    newline();
	}
	divider("|");

	/*if title is available, print it, otherwise, print (untitled). in
	   either case, print later name immediately following
	 */

	label = nlayers > 1 ? "MAPS:" : "MAP:";
	len1 = strlen(label) + 1;
	for (i = 0; i < nlayers; i++) {
	    char *title;

	    title = Rast_get_cats_title(&(layers[i].labels));
	    if (title)
		G_strip(title);
	    if (title == NULL || *title == 0)
		title = "(untitled)";
	    sprintf(buf, "%-*s%*s%s (%s in %s)", len1, label,
		    i * 2, "", title, layers[i].name, layers[i].mapset);

	    pbuf(buf);
	    label = "";
	}
	divider("|");
    }

    len1 = layers[0].clen + layers[0].nlen;
    for (k = 0; k < 2; k++) {
	if (k == 0)
	    lcr("", "Category Information", "", buf, len1);
	else {
	    sprintf(tbuf1, "%*s|description", layers[0].nlen, "#");
	    lcr(tbuf1, "", "", buf, len1);
	}
	fprintf(stdout, "|%s ", buf);
	for (i = unit1; i <= unit2; i++)
	    fprintf(stdout, "|%*s", unit[i].len, unit[i].label[k]);
	fprintf(stdout, "|");
	newline();
    }
    divider("|");

    return 0;
}

int divider(char *edge)
{
    int n;

    fprintf(stdout, "%s", edge);
    n = page_width - 2;
    while (--n >= 0)
	fprintf(stdout, "-");
    fprintf(stdout, "%s", edge);
    newline();

    return 0;
}

static int max(int a, int b)
{
    return a > b ? a : b;
}

int trailer(void)
{
    divider("+");
    while (nlines > 0)
	newline();
    if (use_formfeed)
	fprintf(stdout, "\f");

    return 0;
}

int newline(void)
{
    fprintf(stdout, "\n");
    nlines--;

    return 0;
}

int lcr(const char *left, const char *center, const char *right, char *buf, int n)
{
    int ll, lc, lr;
    int sc, sr;

    ll = strlen(left);
    lc = strlen(center);
    lr = strlen(right);

    sc = (n - lc) / 2 - ll;
    sr = n - lr - lc - (n - lc) / 2;
    
    sprintf(buf, "%s%*s%s%*s%s",
	    left,
	    sc > 0 ? sc : 0, "", center,
	    sc > 0 ? sr : sr + sc , "", right);

    return 0;
}
