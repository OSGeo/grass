/*
 ************************************************************
 * MODULE: r.le.setup/ask_group.c                           *
 *         Version 5.0beta            Oct. 1, 2001          *
 *                                                         *
 * AUTHOR: W.L. Baker, University of Wyoming                *
 *         BAKERWL@UWYO.EDU                                 *
 *                                                          *
 * PURPOSE: To set up sampling areas, which can can then    *
 *         be used to obtain data using the r.le.dist,      *
 *         r.le.patch, and r.le.pixel programs.  The        *
 *         ask_group.c code queries the user for input      *
 *         regarding attribute and index groups             *
 *                                                         *
 * COPYRIGHT: (C) 2001 by W.L. Baker                        *
 *                                                          *
 * This program is free software under the GNU General      *
 * Public License(>=v2).  Read the file COPYING that comes  *
 * with GRASS for details                                   *
 *                                                         *
 ************************************************************/

#include "setup.h"
#include <grass/config.h>
#include <grass/vask.h>
#include <unistd.h>
#include <grass/gis.h>


static void ask_limits(char *name, char *str);
static void get_index(FILE * fp);
static void ask_reclass(void);
static void get_1recl(char *buf, int singles);
static void ask_fromto(void);

				/* PROMPT THE USER TO SELECT THE GROUP/CLASS
				   LIMITS */

int ask_group(char **sel)
{

    /* show the option menu using GRASS VASK
       library function */

    V_clear();
    V_intrpt_msg("EXIT PROGRAM");

    V_line(2, "SELECT ATTRIBUTE GROUP OR INDEX CLASS TO SETUP:");
    V_line(3,
	   "Type 'x' to select; <CR> to go to the next; <space> over to exclude");
    V_line(5, "r.le.patch");
    V_line(6, "   Attribute Groups");
    V_line(7, "   Size Classes");
    V_line(8, "   Shape Classes");
    V_line(9, "      Perim./Area Index");
    V_line(10, "      Corr. Perim./Area Index");
    V_line(11, "      Rel. Circum. Circle Index");
    V_line(13, "r.le.dist");
    V_line(14, "   Distance Classes");
    V_line(15, "      Center to Center");
    V_line(16, "      Center to Edge");
    V_line(17, "      Edge to Edge");
    V_line(18, "   From & To Groups for di1=m7, m8, or m9");

    V_ques(sel[0], 's', 6, 50, 1);
    V_ques(sel[1], 's', 7, 50, 1);
    V_ques(sel[2], 's', 9, 50, 1);
    V_ques(sel[3], 's', 10, 50, 1);
    V_ques(sel[4], 's', 11, 50, 1);
    V_ques(sel[5], 's', 15, 50, 1);
    V_ques(sel[6], 's', 16, 50, 1);
    V_ques(sel[7], 's', 17, 50, 1);
    V_ques(sel[8], 's', 18, 50, 1);

    V_intrpt_ok();
    if (!V_call())
	return (-1);
    return (1);
}


/* DRIVER TO LOOK FOR OLD VERSION OF GROUP/CLASS FILES, SETUP NEW FILES,
   AND PRINT A MESSAGE ABOUT THESE FILES */

int get_group_drv(char **sel)
{

    if (sel[0][0] == 'x') {
	ask_reclass();
	fprintf(stderr,
		"\n    The attribute groups are saved in \"r.le.para/recl_tb\".");
	G_sleep(2);
    }
    if (sel[1][0] == 'x') {
	ask_limits("size", "    SIZE CLASSES");
	fprintf(stderr,
		"\n    The size classes are saved in \"r.le.para/size\".");
	G_sleep(2);
    }
    if (sel[2][0] == 'x') {
	ask_limits("shape_PA", "    SHAPE CLASSES: PERIMETER/AREA");
	fprintf(stderr,
		"\n    The shape (P/A) classes are saved in \"r.le.para/shape_PA\".");
	G_sleep(2);
    }
    if (sel[3][0] == 'x') {
	ask_limits("shape_CPA",
		   "    SHAPE CLASSES: CORRECTED PERIMETER/AREA");
	fprintf(stderr,
		"\n    The shape (CPA) classes are saved in \"r.le.para/shape_CPA\".");
	G_sleep(2);
    }
    if (sel[4][0] == 'x') {
	ask_limits("shape_RCC",
		   "    SHAPE CLASSES: RELATED CIRCUMSCRIBING CIRCLE");
	fprintf(stderr,
		"\n    The shape (RCC) classes are saved in \"r.le.para/shape_RCC\".");
	G_sleep(2);
    }
    if (sel[5][0] == 'x') {
	ask_limits("dist_cc", "    DISTANCE CLASSES: CENTER-CENTER");
	fprintf(stderr,
		"\n    The distance (CC) classes are saved in \"r.le.para/dist_cc\".");
	G_sleep(2);
    }
    if (sel[6][0] == 'x') {
	ask_limits("dist_ce", "    DISTANCE CLASSES: CENTER-EDGE");
	fprintf(stderr,
		"\n    The distance (CE) classes are saved in \"r.le.para/dist_ce\".");
	G_sleep(2);
    }
    if (sel[7][0] == 'x') {
	ask_limits("dist_ee", "    DISTANCE CLASSES: EDGE-EDGE");
	fprintf(stderr,
		"\n    The distance (EE) classes are saved in \"r.le.para/dist_ee\".");
	G_sleep(2);
    }
    if (sel[8][0] == 'x') {
	ask_fromto();
	fprintf(stderr,
		"\n    The attribute groups are saved in \"r.le.para/from_to\".");
	G_sleep(2);
    }
    return (0);
}



/* GET THE LOWER LIMITS OF THE MEASURE INDEX CLASSES FROM THE SCREEN,
   AND PUT THEM IN A FILE */

static void ask_limits(char *name, char *str)
{
    FILE *fp;
    char s[30];

    G_system("clear");
    fprintf(stderr, "\n%s \n", str);
    sprintf(s, "r.le.para/%s", name);
    fp = fopen0(s, "w");
    get_index(fp);
    fprintf(fp, " -999 - lower limits.\n");
    fclose(fp);
    return;
}



/* READ IN THE LOWER LIMITS OF THE MEASURE INDEX CLASSES FROM THE SCREEN */

static void get_index(FILE * fp)
{
    double low, tmp = -999;

    fprintf(stderr,
	    "\n    Enter the lower limits in ascending order, -999 to end.\n");
    fprintf(stderr, "    Example: 0 0.1 10 100 ..., -999.\n");
    fprintf(stderr, "\n  > ");
    for (;;) {
	numtrap(1, &low);
	if (low == -999)
	    break;
	if (low <= tmp) {
	    fprintf(stderr,
		    "A number not in ascending order was omitted; you\n");
	    fprintf(stderr, "may wish to setup these classes again");
	}
	else {
	    tmp = low;
	    fprintf(fp, " %.2f ", low);

	}
    }
    return;
}



/* GET THE ATTRIBUTE GROUP LIMITS FROM THE USER AND SAVE THEM IN FILE RECL_TB */

static void ask_reclass(void)
{
    char *line, str[5];
    FILE *fp;
    register int i, j;

    G_sleep_on_error(0);

    /* display a message on the screen about inputing attribute gps */

    G_system("clear");
    fprintf(stderr,
	    "\n\n    Please input attribute groups in table form just like in a");
    fprintf(stderr,
	    "\n      GRASS reclass table; Put a space before the = sign. After");
    fprintf(stderr,
	    "\n      each line of input, the program will confirm what you typed.");
    fprintf(stderr, "\n      Maximum number of groups is 25\n");
    fprintf(stderr, "    Example: 1 4 9  101 thru 120 = 1 forest\n");
    fprintf(stderr, "             10 thru 100 = 2  prairie\n");
    fprintf(stderr, "             end\n");


  back:
    fp = fopen0("r.le.para/recl_tb", "w");

    /* there is a max of 25 attribute gps */

    for (j = 0; j < 25; j++) {
	line = (char *)G_calloc(512, sizeof(char));
	fprintf(stderr, "  > ");


	/* read in 1 line of attribute gp rule */

	get_1recl(line, 0);
	fputs(line, fp);
	if (line[0] == 'e' && line[1] == 'n' && line[2] == 'd') {
	    G_free(line);
	    break;
	}
	G_free(line);
    }

    fclose(fp);
    return;
}




/* READ IN 1 LINE OF THE ATTRIBUTE GP RULE FROM THE SCREEN */

static void get_1recl(char *buf, int singles)
{
    register int i = 0;
    int number = 0;
    int c;

    while (i < 512) {
	while ((c = getchar()) != 't' && c != '=' && c != 'e' && !isdigit(c))
	    getchar();
	if (c == 't' && getchar() == 'h' && getchar() == 'r' &&
	    getchar() == 'u') {
	    buf[i] = 't';
	    buf[i + 1] = 'h';
	    buf[i + 2] = 'r';
	    buf[i + 3] = 'u';
	    buf[i + 4] = ' ';
	    getchar();
	    i += 5;
	}
	else if (c == '=' && !singles && number) {
	    buf[i] = '=';
	    i++;
	    buf[i] = ' ';
	    getchar();
	    i++;
	    while ((buf[i] = getchar()) != '\n')
		i++;
	    buf[i + 1] = '\0';
	    break;
	}
	else if (c == 'e' && getchar() == 'n' && getchar() == 'd') {
	    if (!singles)
		i = 0;
	    buf[i] = 'e';
	    buf[i + 1] = 'n';
	    buf[i + 2] = 'd';
	    if (!singles) {
		buf[i + 3] = '\n';
		buf[i + 4] = '\0';
		break;
	    }
	    buf[i + 3] = ' ';
	    i += 4;
	    while ((buf[i] = getchar()) != '\n')
		i++;
	    buf[i + 1] = '\0';
	    break;
	}
	else if (isdigit(c)) {
	    do
		buf[(i++)] = c;
	    while (isdigit(c = getchar()));
	    buf[i] = ' ';
	    number = 1;
	    i++;
	}
    }
    fprintf(stderr, "    Attribute group reclass rule is: %s\n", buf);

    return;
}




/* PUT THE FROM & TO ATTRIBUTE GPS FOR DISTANCE METHOD di2=M7 to M9 INTO THE
   FILE R.LE.PARA/FROM_TO */

static void ask_fromto(void)
{
    register int i;
    FILE *fp;
    char *buf;

    G_system("clear");
    fp = fopen0("r.le.para/from_to", "w");
    buf = G_malloc(513);

    fprintf(stderr,
	    "\n\n  Please enter \"FROM\" attribute group followed by \"0 end\"\n");
    for (i = 0; i < 2; i++) {
	fprintf(stderr,
		"    Example: 2 0 end     -- This selects 2 as the group\n");
	fprintf(stderr, "  > ");

	get_1recl(buf, 1);
	fputs(buf, fp);
	if (i == 0)
	    fprintf(stderr,
		    "\n\n  Please enter \"TO\" attribute group followed by \"0 end\"\n");
    }
    fclose(fp);
    G_free(buf);
    return;
}



/* FUNCTION TO OPEN A FILE AND DISPLAY AN ERROR MESSAGE IF THE FILE IS NOT FOUND */

FILE *fopen0(char *name, char *flag)
{
    FILE *fp;

    if (!(fp = fopen(name, flag))) {
	fprintf(stderr,
		"\nCan't open file \"%s\"; use r.le.setup for group/class limits\n",
		name);

	exit(1);
    }
    return fp;
}
