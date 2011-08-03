#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <grass/gis.h>
#include <grass/site.h>

#include "local_proto.h"


#define isnull(c) (c=='\0')

static char *my_next_att(char *);
int is_decimal(char *);

Site *get_site(FILE * fd, int dims, char *fs, int *has_cat)
{
    static char *ibuf = NULL, *buf = NULL, *save = NULL;
    char *b;
    char ebuf[256], nbuf[256];
    static int first = 1;
    static unsigned int line = 0;
    static int tty;
    static int proj;
    int i, err, itmp;
    int n = 0;			/* number of categories */
    int d = 0;			/* number of floating point attributes */
    int c = 0;			/* number of string attributes */
    float ftmp;
    static Site *site;

    if (first) {
	site = G_site_new_struct(-1, dims, 0, 0);

	ibuf = G_malloc(1024 * sizeof(char));
	buf = G_malloc(1024 * sizeof(char));
	save = buf;
	if (ibuf == NULL || buf == NULL)
	    G_fatal_error("memory allocation errory");
	tty = isatty(fileno(fd));
	proj = G_projection();
	if (tty) {
	    fprintf(stdout, "Enter sites, one per line, in the format:\n");
	    fprintf(stdout, "east north ");
	    for (i = 3; i <= dims; ++i)
		fprintf(stdout, "dim%d ", i);
	    fprintf(stdout, "attributes\n");
	    fprintf(stdout, "When finished, type: end\n");
	}
	first = 0;
    }
    err = 1;
    while (err) {
	if (tty)
	    fprintf(stdout, "location attributes> ");
	else
	    line++;

	if (!G_getl(ibuf, 1024, fd))
	    return (Site *) NULL;

	/* G_squeeze cleans DOS newlines, but that makes Mac OS9 impossible
	   to check for, so we do this test first */
	if (strchr(ibuf, '\r') != NULL) {
	    if ('\r' == ibuf[strlen(ibuf) - 1]) {
		ibuf[strlen(ibuf) - 1] = '\0';	/* cleanse */
		if (strchr(ibuf, '\r') != NULL)	/* any left, eg a short Mac OS9 file */
		    G_fatal_error
			("Input file not in UNIX format (invalid newline)");
		else if (1 == line)	/* ie DOS '\r\n' newline */
		    G_warning
			("Input file is DOS format. Attempting anyway ..");
	    }
	    else		/* any others, eg Mac OS9 '\r' or a backwards DOS newline */
		/* fgets() gets it wrong, so there is little we can do.   */
		G_fatal_error
		    ("Line %d: Input file not in UNIX format (invalid newline)",
		     line);
	}

	buf = save;
	strcpy(buf, ibuf);
	G_squeeze(buf);

	if (*buf == 0)
	    return (Site *) NULL;
	if (strcmp(buf, "end") == 0)
	    return (Site *) NULL;
	if (fs) {
	    for (b = buf; *b; b++)
		if (*b == *fs)
		    *b = ' ';
	}
	if (sscanf(buf, "%s %s", ebuf, nbuf) != 2
	    || !G_scan_easting(ebuf, &site->east, proj)
	    || !G_scan_northing(nbuf, &site->north, proj)) {
	    if (!tty) {
		fprintf(stderr, "%s - line %d ", G_program_name(), line);
	    }
	    fprintf(stderr, "** invalid format **\n");
	    if (!tty)
		fprintf(stderr, "<%s>\n", ibuf);
	    /* return (Site *) NULL; */
	}
	else
	    err = 0;
    }

    buf += strlen(ebuf) + strlen(nbuf) + 1;
    dims -= 2;
    for (i = 0; i < dims; ++i) {
	sscanf(buf, "%s", ebuf);
	sscanf(ebuf, "%lf", &(site->dim[i]));
	buf += strlen(ebuf) + 1;
    }

    /* no more dimensions-now we parse attribute fields */
    while (isspace(*buf))
	buf++;

    while (buf != NULL) {
	switch (*buf) {
	    /* check for prefixed atts first, then without prefix */
	case '#':		/* category field */
	    if (n == 0) {
		sscanf(buf, "#%s ", ebuf);
		if (strstr(ebuf, ".") == NULL &&
		    sscanf(ebuf, "%d", &itmp) == 1) {
		    site->cattype = CELL_TYPE;
		    site->ccat = itmp;
		    n++;
		}
		else if (strstr(ebuf, ".") != NULL &&
			 sscanf(ebuf, "%f", &ftmp) == 1) {
		    site->cattype = FCELL_TYPE;
		    site->fcat = ftmp;
		    n++;
		}
		else
		    site->cattype = -1;
	    }
	    else
		G_warning
		    ("Only one category attribute allowed per record; ignoring");

	    /* move to beginning of next attribute */
	    buf = my_next_att(buf);
	    break;
	case '%':		/* decimal attribute */
	    if (d >= site->dbl_alloc) {
		site->dbl_alloc++;
		site->dbl_att = (double *)G_realloc(site->dbl_att,
						    site->dbl_alloc *
						    sizeof(double));
	    }
	    if ((err = sscanf(buf, "%%%lf", &(site->dbl_att[d++]))) < 1)
		G_warning("error scanning floating point attribute: [%s]",
			  buf);
	    buf = my_next_att(buf);
	    break;
	case '@':		/* string attribute */
	    if (isnull(*buf) || isnull(*(buf + 1))) {
		*buf = '\0';
		break;
	    }			/* moved copy of commented out code back into @ case 2/00 rsb */
	    else {
		buf++;

		if (c >= site->str_alloc) {
		    site->str_alloc++;
		    site->str_att = (char **)G_realloc(site->str_att,
						       site->str_alloc *
						       sizeof(char *));
		    if (site->str_att == NULL)
			G_fatal_error("memory allocation error");
		    site->str_att[site->str_alloc - 1] =
			(char *)G_malloc(MAX_SITE_STRING * sizeof(char));
		    if (site->str_att[site->str_alloc - 1] == NULL)
			G_fatal_error("memory allocation error");
		}

		if ((err = cleanse_string(buf)) > 0) {
		    strncpy(site->str_att[c++], buf, err);
		    buf += err;
		}
		else
		    *buf = '\0';
		buf = my_next_att(buf);
		break;		/* end of modification 2/00 rsb */
	    }
	case '\0':		/* EOL, null encountered */
	    buf = my_next_att(buf);
	    break;
	default:		/* changed to unprefixed decimals */
	    /* commented 12/99: default shall be decimal field! M.N. */
	    /* defaults to string attribute */
	    /* allow both prefixed and unprefixed strings */
	    /* 
	       if (c >= site->str_alloc) {
	       site->str_alloc++;
	       site->str_att = (char **) G_realloc (site->str_att,
	       site->str_alloc * sizeof (char *));
	       if (site->str_att == NULL) 
	       G_fatal_error ("memory allocation error");
	       site->str_att[site->str_alloc - 1] = 
	       (char *) G_malloc (MAX_SITE_STRING * sizeof (char));
	       if (site->str_att[site->str_alloc - 1] == NULL) 
	       G_fatal_error ("memory allocation error");
	       }

	       if ((err = cleanse_string (buf)) > 0) {
	       strncpy (site->str_att[c++], buf, err);
	       buf += err;
	       }
	       else *buf = '\0';
	       buf = my_next_att (buf);
	       break;
	       ** *//* end of comment (default=strings) */

	    /* default is unprefixed decimal attribute */
#ifdef USE_OTHER_CODE
	    if (d >= site->dbl_alloc) {
		site->dbl_alloc++;
		site->dbl_att = (double *)G_realloc(site->dbl_att,
						    site->dbl_alloc *
						    sizeof(double));
	    }
	    if ((err = sscanf(buf, "%lf", &(site->dbl_att[d++]))) < 1)
		G_warning("error scanning floating point attribute: <%s>",
			  buf);
	    buf = my_next_att(buf);
	    break;
#else
	    if (is_decimal(buf)) {	/* Convert as double */
		if (d >= site->dbl_alloc) {
		    site->dbl_alloc++;
		    site->dbl_att = (double *)G_realloc(site->dbl_att,
							site->dbl_alloc *
							sizeof(double));
		}
		if ((err = sscanf(buf, "%lf", &(site->dbl_att[d++]))) < 1)
		    G_warning("error scanning floating point attribute: '%s'",
			      buf);
		buf = my_next_att(buf);
	    }
	    else {		/* Convert as string */
		if (c >= site->str_alloc) {
		    site->str_alloc++;
		    site->str_att = (char **)G_realloc(site->str_att,
						       site->str_alloc *
						       sizeof(char *));
		    if (site->str_att == NULL)
			G_fatal_error("memory allocation error");
		    site->str_att[site->str_alloc - 1] =
			(char *)G_malloc(MAX_SITE_STRING * sizeof(char));
		    if (site->str_att[site->str_alloc - 1] == NULL)
			G_fatal_error("memory allocation error");
		}

		if ((err = cleanse_string(buf)) > 0) {
		    strncpy(site->str_att[c++], buf, err);
		    buf += err;
		}
		else
		    *buf = '\0';
		buf = my_next_att(buf);
	    }
#endif /* USE_OTHER_CODE */
	}			/* switch */
    }				/* while */
    *has_cat = n;
    return site;
}


static char *my_next_att(char *buf)
{
    while (!isspace(*buf) && !isnull(*buf))
	buf++;
    if (isnull(*buf) || isnull(*(buf + 1)))
	return (char *)NULL;
    else
	while (isspace(*(buf + 1)) && !isnull(*(buf + 1)))
	    buf++;
    buf++;
    return buf;
}

int is_decimal(char *att)
{
    char *p;

    p = att;
    while (*p != '\0' && !isspace(*p)) {
	if (!isdigit(*p) && *p != '.' && *p != '-' && *p != '+') {
	    return 0;
	}
	p++;
    }
    return 1;
}

/* vim: set softtabstop=4 shiftwidth=4 expandtab: */
