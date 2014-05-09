#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

void get_stp_proj(char string[])
{
    int code;
    char answer[50], buff[50];

    while ((code = get_stp_num()) == 0) {
	if (G_yes
	    ("Are you sure you want to exit without making any changes", 0))
	    leave(SP_NOCHANGE);
    }
    for (;;) {

	do {
	    fprintf(stderr, "\nSpecify State Plane 1927 or 1983\n");
	    fprintf(stderr, "Enter '27' or '83'\n");
	    fprintf(stderr, "Hit RETURN to cancel request\n");
	    fprintf(stderr, ">");
	} while (!G_gets(answer));

	G_strip(answer);
	if (strlen(answer) == 0) {
	    leave(SP_NOCHANGE);
	}
	else if (strcmp(answer, "27") == 0) {
	    sprintf(buff, STP1927PARAMS);
	    break;
	}
	else if (strcmp(answer, "83") == 0) {
	    sprintf(buff, STP1983PARAMS);
	    break;
	}
	else
	    fprintf(stderr, "\nInvalid Co-ordinate System Specification\n");
    }
    if (get_stp_code(code, string, buff) == 0)
	G_fatal_error(_("This should not happen. See your system admin."));

    return;
}

int get_stp_code(int code, char *string, char *paramfile)
{
    char nad27[256], buff[256], *p;
    int gotit = 0, stp;
    FILE *fp;


    sprintf(nad27, "%s%s", G_gisbase(), paramfile);
    fp = fopen(nad27, "r");
    if (fp == NULL) {
	sprintf(buff, "Can not open NAD27 file %s", nad27);
	G_fatal_error(buff);
    }
    while (!gotit) {
	if (fgets(buff, 200, fp) == NULL)
	    break;
	if (buff[0] != '#') {
	    sscanf(buff, "%d:", &stp);
	    if (stp == code) {
		p = strtok(buff, ":");
		p = strtok(NULL, "\n");
		while (*p == ' ')
		    p++;
		sprintf(string, "%s", p);
		gotit = 1;
	    }
	}
    }
    fclose(fp);
    return (gotit);
}





int get_stp_num(void)
{
    FILE *fipsfile;
    char FIPSfile[256], buff[256];
    int NUM_ZON, sfips, cfips, SFIPS = 0, CFIPS = 0;
    int record, icode, reccnt, special_case;
    char STabbr[50], COname[50];

    sprintf(FIPSfile, "%s/etc/proj/FIPS.code", G_gisbase());


    for (;;) {

	fipsfile = fopen(FIPSfile, "r");
	if (fipsfile == NULL) {
	    G_fatal_error(_("Unable to open FIPS code file"));
	}
	ask_fips(fipsfile, &SFIPS, &CFIPS, &special_case);
	if (special_case == -1) {
	    fclose(fipsfile);
	    return (0);
	}
	/* combine SFIPS and CFIPS to make lookup */
	/*DEBUG fprintf(stderr,"FIPS = %d %d\n",SFIPS,CFIPS); */

	for (record = 0;; ++record) {
	    icode = 0;
	    reccnt++;
	    if (fgets(buff, 80, fipsfile) == NULL)
		break;
	    sscanf(buff, "%d%d%s%s%d", &sfips, &cfips, STabbr, COname,
		   &NUM_ZON);
	    /* compare for match */
	    if (SFIPS == sfips && CFIPS == cfips) {
		icode = 1;
		break;
	    }
	}			/* end file search */
	if (icode != 0)
	    break;
	else {			/* no match */
	    G_warning(_("No match of FIPS state %d county %d"), SFIPS, CFIPS);
	    fclose(fipsfile);
	}
    }

/**** SPECIAL CASE FOR MICHIGAN ****, could be mercator or lambert */
    if (SFIPS == 26) {
	if (special_case == 2)
	    NUM_ZON = NUM_ZON + 10;
    }

/**** SPECIAL CASE FOR ALASKA *****  */
    if (SFIPS == 2) {
	NUM_ZON = NUM_ZON + special_case;
    }
    /* all done, good-bye */
    fclose(fipsfile);
    return (NUM_ZON);
}




int ask_fips(FILE * fp, int *s, int *c, int *sc)
{
    int ii, FIPS = 0, NUM_ZON, sfips, cfips;
    char STabbr[50], STabbr_prev[50], COname[50], answer[50], buff[256];
    char *Tmp_file1, *Tmp_file2, *a, *b;
    FILE *Tmp_fd1 = NULL, *Tmp_fd2 = NULL;
    int in_stat;
    struct Key_Value *sf_keys, *cf_keys;

    *sc = 0;
    *s = 0;
    *c = 0;
    Tmp_file1 = G_tempfile();
    if (NULL == (Tmp_fd1 = fopen(Tmp_file1, "w")))
	G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file1);
    Tmp_file2 = G_tempfile();
    if (NULL == (Tmp_fd2 = fopen(Tmp_file2, "w")))
	G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file2);
    while (fgets(buff, 80, fp) != NULL) {
	sscanf(buff, "%d%d%s%s%d", &sfips, &cfips, STabbr, COname, &NUM_ZON);
	if (strncmp(STabbr, STabbr_prev, 2) != 0) {	/* TODO CHECK THIS */
	    fprintf(Tmp_fd1, "%4d -- %s\n", sfips, STabbr);
	    fprintf(Tmp_fd2, "%d:%s\n", sfips, STabbr);
	}
	sprintf(STabbr_prev, "%s", STabbr);
    }
    fclose(Tmp_fd1);
    fclose(Tmp_fd2);

    sf_keys = G_read_key_value_file(Tmp_file2, &in_stat);
    if (in_stat != 0)
	G_fatal_error(_("Reading sf key_value temp file"));

    for (;;) {

	do {
	    fprintf(stderr, "\nSpecify State FIPS (numeric) code\n");
	    fprintf(stderr,
		    "Enter 'list' for the list of states with corresponding FIPS codes\n");
	    fprintf(stderr, "Hit RETURN to cancel request\n");
	    fprintf(stderr, ">");
	} while (!G_gets(answer));

	G_strip(answer);
	if (strlen(answer) == 0) {
	    *sc = -1;
	    return 0;
	}
	if (strcmp(answer, "list") == 0) {
	    char *pager;

	    pager = getenv("GRASS_PAGER");
	    if (!pager || strlen(pager) == 0)
		pager = "cat";

	    /* Always print interactive output to stderr */
	    sprintf(buff, "%s \"%s\" 1>&2", pager,
		    G_convert_dirseps_to_host(Tmp_file1));
	    G_system(buff);
	}
	else {
	    a = G_find_key_value(answer, sf_keys);
	    sprintf(buff, "You have chosen state %s, Correct", a);
	    if (a == NULL)
		G_warning(_("Invalid State FIPS code"));
	    else if (G_yes(buff, 1))
		break;
	}
    }
    rewind(fp);

    sscanf(answer, "%d", s);

    FIPS = *s;

/**** SPECIAL CASE FOR MICHIGAN ****, could be mercator or lambert */
    if (FIPS == 26) {
	/*
	   fprintf(stderr,"\nFor Michigan select- 1- East to West\n");
	   fprintf(stderr,"                     2- North to South\n: ");
	 */
	ii = 0;
	for (;;) {
	    do {
		fprintf(stderr, "\nFor Michigan select- 1- East to West\n");
		fprintf(stderr, "                     2- North to South\n: ");
		fprintf(stderr, "Hit RETURN to cancel request\n> ");

	    } while (!G_gets(answer));

	    G_strip(answer);
	    if (strlen(answer) == 0) {
		*sc = -1;
		return 0;
	    }
	    sscanf(answer, "%d", &ii);
	    if (ii != 1 && ii != 2)
		fprintf(stderr, "\n Invalid Entry\n ");
	    else
		break;
	}
	*sc = ii;
    }

/**** SPECIAL CASE FOR ALASKA *****  */
    if (FIPS == 2) {
	ii = 0;
	for (;;) {

	    do {
		fprintf(stderr,
			"\nFor Alaska enter the zone (1 through 9): \n");
		fprintf(stderr, "Hit RETURN to cancel request\n");
		fprintf(stderr, "> ");
	    } while (!G_gets(answer));

	    G_strip(answer);
	    if (strlen(answer) == 0) {
		*sc = -1;
		return 0;
	    }
	    sscanf(answer, "%d", &ii);
	    if (ii < 1 || ii > 9)
		fprintf(stderr, "\n Invalid Entry\n ");
	    else
		break;
	}
	*sc = ii;
    }
    unlink(Tmp_file1);
    unlink(Tmp_file2);

    Tmp_file1 = G_tempfile();
    if (NULL == (Tmp_fd1 = fopen(Tmp_file1, "w"))) {
	G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file1);
    }
    Tmp_file2 = G_tempfile();
    if (NULL == (Tmp_fd2 = fopen(Tmp_file2, "w"))) {
	G_fatal_error(_("Unable to open temporary file <%s>"), Tmp_file2);
    }
    while (fgets(buff, 80, fp) != NULL) {
	sscanf(buff, "%d%d%s%[A-Z ]%d", &sfips, &cfips, STabbr, COname,
	       &NUM_ZON);
	G_strip(COname);
	if (sfips == *s) {
	    fprintf(Tmp_fd1, "%4d -- %s\n", cfips, COname);
	    fprintf(Tmp_fd2, "%d:%s\n", cfips, COname);
	}			/* ADDED THESE BRACKETS - BB 5/2000 */
    }
    fclose(Tmp_fd1);
    fclose(Tmp_fd2);

    cf_keys = G_read_key_value_file(Tmp_file2, &in_stat);
    if (in_stat != 0)
	G_fatal_error(_("Reading cf key_value temp file"));

    for (;;) {
	do {
	    fprintf(stderr,
		    "\nSpecify County FIPS (numeric) code for state %s\n", a);
	    fprintf(stderr,
		    "Enter 'list' for the list of counties in %s with corresponding FIPS codes\n",
		    a);
	    fprintf(stderr, "Hit RETURN to cancel request\n");
	    fprintf(stderr, ">");
	} while (!G_gets(answer));

	G_strip(answer);
	if (strlen(answer) == 0) {
	    *sc = -1;
	    return 0;
	}
	if (strcmp(answer, "list") == 0) {
	    char *pager;

	    pager = getenv("GRASS_PAGER");
	    if (!pager || strlen(pager) == 0)
		pager = "cat";

	    /* Always print interactive output to stderr */
	    sprintf(buff, "%s \"%s\" 1>&2", pager,
		    G_convert_dirseps_to_host(Tmp_file1));
	    G_system(buff);
	}
	else {
	    b = G_find_key_value(answer, cf_keys);
	    sprintf(buff, "You have chosen %s county, correct", b);
	    if (b == NULL)
		G_warning(_("Invalid County FIPS code"));
	    else if (G_yes(buff, 1))
		break;
	}
    }
    sscanf(answer, "%d", c);
    rewind(fp);
    unlink(Tmp_file1);
    unlink(Tmp_file2);
    return 0;
}
