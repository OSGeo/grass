/* Function: multi_text_box_path, multi_lines
 **
 ** Author: Paul W. Carlson     March 1992
 */

#include "ps_info.h"

#define LEFT 0
#define RIGHT 1
#define LOWER 0
#define UPPER 1
#define CENTER 2

int multi_text_box_path(double x, double y,
			int xref, int yref, char *text, int fontsize,
			float rotate)
{
    int numlines;
    char *ptr;
    static int firsttime = 1;

    /* write procs if first time called */
    if (firsttime) {
	firsttime = 0;

	/* procs to get box for multiple text lines */
	fprintf(PS.fp, "/CMX {l sub r l sub D2 sub} BD\n");
	fprintf(PS.fp, "/CMY {t sub t b sub D2 add} BD\n");
	fprintf(PS.fp, "/LMX {l sub} BD\n");
	fprintf(PS.fp, "/LMY {b sub} BD\n");
	fprintf(PS.fp, "/RMX {r sub} BD\n");
	fprintf(PS.fp, "/UMY {t sub} BD\n");
	fprintf(PS.fp, "/MTBX {/y dely def\n");
	fprintf(PS.fp, "0 1 nlm1 { /i exch def\n");
	fprintf(PS.fp, "newpath /y y dely sub def\n");
	fprintf(PS.fp, "0 y moveto ta i get\n");
	fprintf(PS.fp, "false charpath flattenpath pathbbox\n");
	fprintf(PS.fp, "/tt XD /rr XD /bb XD /ll XD\n");
	fprintf(PS.fp, "tt t gt {/t tt def} if rr r gt {/r rr def} if\n");
	fprintf(PS.fp, "bb b lt {/b bb def} if ll l lt {/l ll def} if\n");
	fprintf(PS.fp, "} for\n");
	fprintf(PS.fp, "/t t mg add def /r r mg add def \n");
	fprintf(PS.fp, "/b b mg sub def /l l mg sub def} BD\n");
	fprintf(PS.fp, "/TBM {l b r t B} BD\n");

	/* proc to draw multiple text lines in text color */
	fprintf(PS.fp, "/DMT {/y dely def 0 1 nlm1 {\n");
	fprintf(PS.fp, "/i exch def /y y dely sub def\n");
	fprintf(PS.fp, "0 y moveto ta i get show } for grestore} BD\n");

	/* proc to draw multiple text lines in highlight color */
	fprintf(PS.fp, "/DMH {/y dely def 0 1 nlm1 {\n");
	fprintf(PS.fp, "/i exch def /y y dely sub def\n");
	fprintf(PS.fp, "newpath 0 y moveto ta i get\n");
	fprintf(PS.fp, "false charpath stroke} for} BD\n");
    }

    /* put text into array */
    numlines = 0;
    ptr = text;
    fprintf(PS.fp, "/ta [ (");
    while (*ptr) {
	if (*ptr == '\\' && *(ptr + 1) == 'n') {
	    fprintf(PS.fp, ")\n(");
	    ptr++;
	    numlines++;
	}
	else
	    fprintf(PS.fp, "%c", *ptr);
	ptr++;
    }
    fprintf(PS.fp, ") ] def\n");
    numlines++;

    /*initialize PostScript variables */
    fprintf(PS.fp, "/t -9999 def /r -9999 def /b 9999 def /l 9999 def\n");
    fprintf(PS.fp, "/dely %d def /nlm1 %d def\n", fontsize, numlines - 1);


    /* get relative box coordinates */
    fprintf(PS.fp, "MTBX\n");

    /* set box x coordinate */
    fprintf(PS.fp, "%.2f ", x);

    /* set box y coordinate */
    fprintf(PS.fp, " %.2f ", y);

    fprintf(PS.fp, "gsave TR %.2f rotate ", rotate);

    fprintf(PS.fp, " 0 ");

    switch (xref) {
    case LEFT:
	fprintf(PS.fp, "LMX");
	break;
    case RIGHT:
	fprintf(PS.fp, "RMX");
	break;
    case CENTER:
    default:
	fprintf(PS.fp, "CMX");
	break;
    }

    fprintf(PS.fp, " 0 ");

    switch (yref) {
    case UPPER:
	fprintf(PS.fp, "UMY");
	break;
    case LOWER:
	fprintf(PS.fp, "LMY");
	break;
    case CENTER:
    default:
	fprintf(PS.fp, "CMY");
	break;
    }
    fprintf(PS.fp, " TR TBM\n");

    return 0;
}

int multi_lines(char *text)
{
    char *ptr;

    ptr = text;
    while (*ptr) {
	if (*ptr == '\\' && *(ptr + 1) == 'n')
	    return 1;
	ptr++;
    }
    return 0;
}
