#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* Determine if the string is integer, e.g. 123, +123, -123, 
 * return 1 if integer, 0 otherwise */
static int is_int(char *str)
{
    char *tail;

    if (strtol(str, &tail, 10), tail == str || *tail != '\0') {
	/* doesn't look like a number,
	   or has extra characters after what looks to be a number */
	return 0;
    }

    return 1;
}


/* Determine if the string is double, e.g. 123.456, +123.456, -123.456, 1.23456e2
 * return 1 if double, 0 otherwise */
static int is_double(char *str)
{
    char *tail;

    if (strtod(str, &tail), tail == str || *tail != '\0') {
	/* doesn't look like a number,
	   or has extra characters after what looks to be a number */
	return 0;
    }

    return 1;
}



/* Analyse points ascii file. Determine number of columns and column types.
 * ascii_tmp: write copy of tempfile to ascii_tmp:
 * rowlength: maximum row length
 * ncolumns: number of columns
 * minncolumns: minimum number of columns
 * nrows: number of rows
 * column_type: column types
 * column_length: column lengths (string only)
 */

int points_analyse(FILE * ascii_in, FILE * ascii, char *fs,
		   int *rowlength, int *ncolumns, int *minncolumns,
		   int *nrows, int **column_type, int **column_length,
		   int skip_lines, int xcol, int ycol, int region_flag)
{
    int i;
    int buflen;			/* buffer length */
    char *buf, *buf_raw;	/* buffer */
    int row = 1;		/* line number, first is 1 */
    int ncols = 0;		/* number of columns */
    int minncols = -1;
    int *coltype = NULL;	/* column types */
    int *collen = NULL;		/* column lengths */
    char **tokens;
    int ntokens;		/* number of tokens */
    int len, rowlen = 0;	/* maximum row length */
    struct Cell_head window;
    double northing = .0;
    double easting = .0;
    char *coorbuf, *tmp_token, *sav_buf;
    int skip = FALSE, skipped = 0;

    buflen = 4000;
    buf = (char *)G_malloc(buflen);
    buf_raw = (char *)G_malloc(buflen);
    coorbuf = (char *)G_malloc(256);
    tmp_token = (char *)G_malloc(256);
    sav_buf = NULL;

    G_message(_("Scanning input for column types..."));
    /* fetch projection for LatLong test */
    G_get_window(&window);

    while (1) {
	len = 0;		/* not really needed, but what the heck */
	skip = FALSE;		/* reset out-of-region check */

	if (G_getl2(buf, buflen - 1, ascii_in) == 0)
	    break;		/* EOF */

	if (row <= skip_lines) {
	    G_debug(3, "skipping header row %d : %d chars", row,
		    (int)strlen(buf));
	    /* this fn is read-only, write to hist with points_to_bin() */
	    fprintf(ascii, "%s\n", buf);
	    len = strlen(buf) + 1;
	    if (len > rowlen)
		rowlen = len;

	    row++;
	    continue;
	}

	if ((buf[0] == '#') || (buf[0] == '\0')) {
	    G_debug(3, "skipping comment row %d : %d chars", row,
		    (int)strlen(buf));
	    continue;
	}

	/* no G_chop() as first/last column may be empty fs=tab value */
	G_debug(3, "row %d : %d chars", row, (int)strlen(buf));

	/* G_tokenize() will modify the buffer, so we make a copy */
	strcpy(buf_raw, buf);

	len = strlen(buf) + 1;
	if (len > rowlen)
	    rowlen = len;

	tokens = G_tokenize(buf, fs);
	ntokens = G_number_of_tokens(tokens);
	if (ntokens > ncols) {
	    int c;
	    coltype = (int *)G_realloc(coltype, ntokens * sizeof(int));
	    collen = (int *)G_realloc(collen, ntokens * sizeof(int));
	    for (c = ncols; c < ntokens; c++) {
		coltype[c] = DB_C_TYPE_INT;	/* default type */
		collen[c] = 0;
	    }
	    ncols = ntokens;
	}

	if (minncols == -1 || minncols > ntokens)
	    minncols = ntokens;

	/* Determine column types */
	for (i = 0; i < ntokens; i++) {
	    if ((G_projection() == PROJECTION_LL)) {
		if (i == xcol || i == ycol) {
		    if (i == 0) {	/* Save position of original internal token buffer */
			/* Prevent memory leaks */
			sav_buf = tokens[0];
		    }
		    /* check if coordinates are DMS or decimal or not latlong at all */
		    sprintf(coorbuf, "%s", tokens[i]);
		    G_debug(4, "token: %s", coorbuf);

		    if (i == xcol) {
			if (G_scan_easting(coorbuf, &easting, window.proj)) {
			    G_debug(4, "is_latlong east: %f", easting);
			    sprintf(tmp_token, "%.12f", easting);
			    /* replace current DMS token by decimal degree */
			    tokens[i] = tmp_token;
			    if (region_flag) {
				if ((window.east < easting) ||
				    (window.west > easting))
				    skip = TRUE;
			    }
			}
			else {
                            fprintf(stderr, _("Current row %d:\n%s\n"), row, buf_raw);
			    G_fatal_error(_("Unparsable longitude value in column num %d: %s"),
					  i + 1, tokens[i]);
			}
		    }

		    if (i == ycol) {
			if (G_scan_northing(coorbuf, &northing, window.proj)) {
			    G_debug(4, "is_latlong north: %f", northing);
			    sprintf(tmp_token, "%.12f", northing);
			    /* replace current DMS token by decimal degree */
			    tokens[i] = tmp_token;
			    if (region_flag) {
				if ((window.north < northing) ||
				    (window.south > northing))
				    skip = TRUE;
			    }
			}
			else {
			    fprintf(stderr, _("Current row %d:\n%s\n"), row, buf_raw);
			    G_fatal_error(_("Unparsable latitude value in column num %d: %s"),
					  i, tokens[i]);
			}
		    }
		}		/* if (x or y) */

		if (i == ntokens - 1 && sav_buf != NULL) {
		    /* Restore original token buffer so free_tokens works */
		    /* Only do this if tokens[0] was re-assigned */
		    tokens[0] = sav_buf;
		    sav_buf = NULL;
		}
	    }			/* PROJECTION_LL */
	    else {
		if (region_flag) {
		    /* consider z range if -z flag is used? */
		    /* change to if(>= east,north){skip=1;} to allow correct tiling */
		    /* don't "continue;" so multiple passes will have the
		       same column types and length for patching */
		    if (i == xcol) {
			easting = atof(tokens[i]);
			if ((window.east < easting) ||
			    (window.west > easting))
			    skip = TRUE;
		    }
		    if (i == ycol) {
			northing = atof(tokens[i]);
			if ((window.north < northing) ||
			    (window.south > northing))
			    skip = TRUE;
		    }
		}
	    }

	    len = strlen(tokens[i]); 
	    /* do not guess column type for missing values */ 
	    if (len == 0) 
		continue; 

	    G_debug(4, "row %d col %d: '%s' is_int = %d is_double = %d",
		    row, i, tokens[i], is_int(tokens[i]),
		    is_double(tokens[i]));

	    if (is_int(tokens[i])) {
		continue;	/* integer */
	    }
	    if (is_double(tokens[i])) {	/* double */
		if (coltype[i] == DB_C_TYPE_INT) {
		    coltype[i] = DB_C_TYPE_DOUBLE;
		}
		continue;
	    }
	    /* string */
	    coltype[i] = DB_C_TYPE_STRING;
	    if (len > collen[i])
		collen[i] = len;
	}

	/* write dataline to tmp file */
	if (!skip)
	    fprintf(ascii, "%s\n", buf_raw);
	else
	    skipped++;

	G_free_tokens(tokens);
	row++;
    }

    *rowlength = rowlen;
    *ncolumns = ncols;
    *minncolumns = minncols;
    *column_type = coltype;
    *column_length = collen;
    *nrows = row - 1;		/* including skipped lines */

    G_free(buf);
    G_free(buf_raw);
    G_free(coorbuf);
    G_free(tmp_token);

    if (region_flag)
	G_message(_("Skipping %d of %d rows falling outside of current region"),
		  skipped, row - 1);

    return 0;
}


/* Import points from ascii file.
 *
 * fs: field separator
 * xcol, ycol, zcol, catcol: x,y,z,cat column in input file, first column is 1, 
 *                            zcol and catcol may be 0 (do not use)
 * rowlen: maximum row length
 * Note: column types (both in header or coldef) must be supported by driver
 */
int points_to_bin(FILE * ascii, int rowlen, struct Map_info *Map,
		  dbDriver * driver, char *table, char *fs, int nrows,
		  int ncols, int *coltype, int xcol, int ycol, int zcol,
		  int catcol, int skip_lines)
{
    char *buf, buf2[4000];
    int cat = 0;
    int row = 1;
    struct line_pnts *Points;
    struct line_cats *Cats;
    dbString sql, val;
    struct Cell_head window;

    G_message(_("Importing points..."));
    /* fetch projection for LatLong test */
    G_get_window(&window);

    rewind(ascii);
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* actually last 2 characters won't be read */
    buf = (char *)G_malloc(rowlen + 2);
    db_init_string(&sql);
    db_init_string(&val);

    if (skip_lines > 0) {
	sprintf(buf2, "HEADER: (%d lines)\n", skip_lines);
	Vect_hist_write(Map, buf2);
    }

    /* rowlen + 2 to read till the end of line on both UNIX and Windows */
    while (G_getl2(buf, rowlen + 2, ascii) != 0) {
	int i, len;
	double x, y, z;
	char **tokens;
	int ntokens;		/* number of tokens */

	if (row <= skip_lines) {
	    G_debug(4, "writing skip line %d to hist : %d chars", row,
		    (int)strlen(buf));
	    Vect_hist_write(Map, buf);
	    Vect_hist_write(Map, "\n");
	    row++;
	    continue;
	}

	len = strlen(buf);
	if (len == 0)
	    continue;		/* should not happen */

	G_debug(4, "row: %s", buf);

	tokens = G_tokenize(buf, fs);
	ntokens = G_number_of_tokens(tokens);

	if ((G_projection() == PROJECTION_LL)) {
	    G_scan_easting(tokens[xcol], &x, window.proj);
	    G_scan_northing(tokens[ycol], &y, window.proj);
	}
	else {
	    x = atof(tokens[xcol]);
	    y = atof(tokens[ycol]);
	}
	G_debug(4, "x: %f, y: %f", x, y);

	if (zcol >= 0)
	    z = atof(tokens[zcol]);
	else
	    z = 0.0;

	if (catcol >= 0)
	    cat = atof(tokens[catcol]);
	else
	    cat++;

	Vect_reset_line(Points);
	Vect_reset_cats(Cats);

	Vect_append_point(Points, x, y, z);
	Vect_cat_set(Cats, 1, cat);

	Vect_write_line(Map, GV_POINT, Points, Cats);

	/* Attributes */
	if (driver) {
	    sprintf(buf2, "insert into %s values ( ", table);
	    db_set_string(&sql, buf2);

	    if (catcol < 0) {
		sprintf(buf2, "%d, ", cat);
		db_append_string(&sql, buf2);
	    }

	    for (i = 0; i < ntokens; i++) {
		if (i > 0)
		    db_append_string(&sql, ", ");

		if (strlen(tokens[i]) > 0) {
		    if (coltype[i] == DB_C_TYPE_INT ||
			coltype[i] == DB_C_TYPE_DOUBLE) {
			if (G_projection() == PROJECTION_LL &&
			    (i == xcol || i == ycol)) {
			    if (i == xcol)
				sprintf(buf2, "%.15g", x);
			    else
				sprintf(buf2, "%.15g", y);
			}
			else
			    sprintf(buf2, "%s", tokens[i]);
		    }
		    else {
			db_set_string(&val, tokens[i]);
			/* TODO: strip leading and trailing "quotes" from input string */
			db_double_quote_string(&val);
			sprintf(buf2, "'%s'", db_get_string(&val));
		    }
		}
		else {
		    sprintf(buf2, "null");
		}
		db_append_string(&sql, buf2);
	    }
	    db_append_string(&sql, ")");
	    G_debug(3, db_get_string(&sql));

	    if (db_execute_immediate(driver, &sql) != DB_OK) {
		G_fatal_error(_("Unable to insert new record: %s"),
			      db_get_string(&sql));
	    }
	}

	G_percent(row, nrows, 2);

	G_free_tokens(tokens);

	row++;
    }

    return 0;
}
