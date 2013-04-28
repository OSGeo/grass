/*!
  \file lib/vector/Vlib/ascii.c
 
  \brief Vector library - GRASS ASCII vector format
 
  Higher level functions for reading/writing/manipulating vectors.
  
  (C) 2001-2009, 2011-2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
  \author Updated for GRASS 7 (SF support) by Martin Landa <landa.martin gmail.com>
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#define BUFFSIZE 128

static int srch(const void *, const void *);
static int get_cat(const struct line_cats *, const struct cat_list *,
		   const int *, int, int, int *);
static void free_col_arrays(int *, char *, char **);

/*!
  \brief Read data in GRASS ASCII vector format

  \param ascii    pointer to the input ASCII file
  \param[out] Map pointer to the output Map_info structure

  \return number of read features
  \return -1 on error
*/
int Vect_read_ascii(FILE *ascii, struct Map_info *Map)
{
    char ctype;
    char buff[BUFFSIZE];
    char east_str[256], north_str[256];
    double *xarray;
    double *yarray;
    double *zarray;
    double *x, *y, *z;
    int i, n_points, n_coors, n_cats, n_lines;
    int type, with_z, skip_feat, nskipped_3d;
    int alloc_points;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int catn, cat;

    /* Must always use this to create an initialized  line_pnts structure */
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /*alloc_points     = 1000 ; */
    alloc_points = 1;
    xarray = (double *)G_calloc(alloc_points, sizeof(double));
    yarray = (double *)G_calloc(alloc_points, sizeof(double));
    zarray = (double *)G_calloc(alloc_points, sizeof(double));

    n_lines = nskipped_3d = 0;

    with_z = Vect_is_3d(Map);

    while (G_getl2(buff, BUFFSIZE - 1, ascii) != 0) {
	n_cats = 0;
        skip_feat = FALSE;
	if (buff[0] == '\0') {
	    G_debug(3, "a2b: skipping blank line");
	    continue;
	}

	if (sscanf(buff, "%1c%d%d", &ctype, &n_coors, &n_cats) < 2 ||
	    n_coors < 0 || n_cats < 0) {
	    if (ctype == '#') {
		G_debug(2, "a2b: skipping commented line");
		continue;
	    }
	    G_warning(_("Error reading ASCII file: (bad type) [%s]"),
                      buff);
            return -1;
	}
	if (ctype == '#') {
	    G_debug(2, "a2b: Skipping commented line");
	    continue;
	}

	switch (ctype) {
	case 'A':
	    type = GV_BOUNDARY;
	    break;
	case 'B':
	    type = GV_BOUNDARY;
	    break;
	case 'C':
	    type = GV_CENTROID;
	    break;
	case 'L':
	    type = GV_LINE;
	    break;
	case 'P':
	    type = GV_POINT;
	    break;
	case 'F':
	    type = GV_FACE;
	    break;
	case 'K':
	    type = GV_KERNEL;
	    break;
	case 'a':
	case 'b':
	case 'c':
	case 'l':
	case 'p':
	    type = 0;		/* dead -> ignore */
	    break;
	default: {
	    G_warning(_("Error reading ASCII file: (unknown type) [%s]"),
                      buff);
            return -1;
        }
	}
	G_debug(5, "feature type = %d", type);
        
        if ((type & (GV_FACE | GV_KERNEL)) && !with_z) {
            skip_feat = TRUE;
            nskipped_3d++;
        }
        
	n_points = 0;
	x = xarray;
	y = yarray;
	z = zarray;

	/* Collect the points */
	for (i = 0; i < n_coors; i++) {
	    if (G_getl2(buff, BUFFSIZE - 1, ascii) == 0) {
		G_warning(_("End of ASCII file reached before end of coordinates"));
                return -1;
            }
	    if (buff[0] == '\0') {
		G_debug(3, "a2b: skipping blank line while reading vertices");
		i--;
		continue;
	    }

	    *z = 0;
	    if (sscanf(buff, "%lf%lf%lf", x, y, z) < 2) {
		if (sscanf(buff, " %s %s %lf", east_str, north_str, z) < 2) {
		    G_warning(_("Error reading ASCII file: (bad point) [%s]"),
                              buff);
                    return -1;
		} else {
		    if (!G_scan_easting(east_str, x, G_projection())) {
			G_warning(_("Unparsable longitude value: [%s]"),
                                  east_str);
                        return -1;
                    }
		    if (!G_scan_northing(north_str, y, G_projection())) {
			G_warning(_("Unparsable latitude value: [%s]"),
                                  north_str);
                        return -1;
                    }
		}
	    }

	    G_debug(5, "coor in: %s -> x = %f y = %f z = %f", G_chop(buff),
		    *x, *y, *z);

	    n_points++;
	    x++;
	    y++;
	    z++;

	    if (n_points >= alloc_points) {
		alloc_points = n_points + 1000;
		xarray =
		    (double *)G_realloc((void *)xarray,
					alloc_points * sizeof(double));
		yarray =
		    (double *)G_realloc((void *)yarray,
					alloc_points * sizeof(double));
		zarray =
		    (double *)G_realloc((void *)zarray,
					alloc_points * sizeof(double));
		x = xarray + n_points;
		y = yarray + n_points;
		z = zarray + n_points;
	    }
	}

	/* Collect the cats */
	Vect_reset_cats(Cats);
	for (i = 0; i < n_cats; i++) {
	    if (G_getl2(buff, BUFFSIZE - 1, ascii) == 0) {
		G_warning(_("End of ASCII file reached before end of categories"));
                return -1;
            }
	    if (buff[0] == '\0') {
		G_debug(3,
			"a2b: skipping blank line while reading category info");
		i--;
		continue;
	    }

	    if (sscanf(buff, "%u%u", &catn, &cat) != 2) {
		G_warning(_("Error reading categories: [%s]"), buff);
                return -1;
            }

	    Vect_cat_set(Cats, catn, cat);
	}

        if (skip_feat)
            continue;
        
	/* Allocation is handled for line_pnts */
	if (0 >
	    Vect_copy_xyz_to_pnts(Points, xarray, yarray, zarray, n_points)) {
	    G_warning(_("Unable to copy points"));
            return -1;
        }

	if (type > 0) {
	    if (-1 == Vect_write_line(Map, type, Points, Cats)) {
		return -1;
	    }
	    n_lines++;
	}
    }

    if (nskipped_3d > 0)
        G_warning(_("Vector map <%s> is 2D. %d 3D features (faces or kernels) skipped."),
                  Vect_get_name(Map), nskipped_3d);
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return n_lines;
}

/*!
  \brief Read header of GRASS ASCII vector format

  \param dascii pointer to the ASCII file
  \param Map    pointer to Map_info structure

  \return 0 on success
  \return -1 on error
*/
int Vect_read_ascii_head(FILE *dascii, struct Map_info *Map)
{
    char buff[1024];
    char *ptr;

    for (;;) {
	if (0 == G_getl2(buff, sizeof(buff) - 1, dascii))
	    return (0);

	/* Last line of header */
	if (strncmp(buff, "VERTI:", 6) == 0)
	    return (0);

	if (!(ptr = strchr(buff, ':'))) {
	    G_warning(_("Unexpected data in vector header:\n[%s]"), buff);
            return -1;
        }

	ptr++;			/* Search for the start of text */
	while (*ptr == ' ')
	    ptr++;

	if (strncmp(buff, "ORGANIZATION:", 12) == 0)
	    Vect_set_organization(Map, ptr);
	else if (strncmp(buff, "DIGIT DATE:", 11) == 0)
	    Vect_set_date(Map, ptr);
	else if (strncmp(buff, "DIGIT NAME:", 11) == 0)
	    Vect_set_person(Map, ptr);
	else if (strncmp(buff, "MAP NAME:", 9) == 0)
	    Vect_set_map_name(Map, ptr);
	else if (strncmp(buff, "MAP DATE:", 9) == 0)
	    Vect_set_map_date(Map, ptr);
	else if (strncmp(buff, "MAP SCALE:", 10) == 0)
	    Vect_set_scale(Map, atoi(ptr));
	else if (strncmp(buff, "OTHER INFO:", 11) == 0)
	    Vect_set_comment(Map, ptr);
	else if (strncmp(buff, "ZONE:", 5) == 0 ||
		 strncmp(buff, "UTM ZONE:", 9) == 0)
	    Vect_set_zone(Map, atoi(ptr));
	else if (strncmp(buff, "WEST EDGE:", 10) == 0) {
	}
	else if (strncmp(buff, "EAST EDGE:", 10) == 0) {
	}
	else if (strncmp(buff, "SOUTH EDGE:", 11) == 0) {
	}
	else if (strncmp(buff, "NORTH EDGE:", 11) == 0) {
	}
	else if (strncmp(buff, "MAP THRESH:", 11) == 0)
	    Vect_set_thresh(Map, atof(ptr));
	else {
	    G_warning(_("Unknown keyword <%s> in vector head"), buff);
	}
    }
    /* NOTREACHED */
}

/*!
  \brief Write data to GRASS ASCII vector format

  Prints message if some features without category are skipped.

  \param[out] ascii  pointer to the output ASCII file
  \param[out] att    att file (< version 5 only)
  \param Map    pointer to Map_info structure
  \param ver    version number 4 or 5
  \param format format GV_ASCII_FORMAT_POINT or GV_ASCII_FORMAT_STD
  \param dp     number of significant digits
  \param fs     field separator
  \param region_flag check region
  \param type   feature type filter
  \param field  field number
  \param Clist  list of categories to filter features or NULL
  \param where  SQL select where statement to filter features or NULL
  \param column_names array of columns to be included to the output or NULL
                 "*" as the first item in the array indicates all columns
  \param header TRUE to print also header

  \return number of written features
  \return -1 on error
*/
int Vect_write_ascii(FILE *ascii,
		     FILE *att, struct Map_info *Map, int ver,
		     int format, int dp, char *fs, int region_flag, int type,
		     int field, const struct cat_list *Clist, const char* where,
		     const char **column_names, int header)
{
    int ltype, ctype, i, cat, n_lines, line, left, right, found;
    double *xptr, *yptr, *zptr, x, y;
    static struct line_pnts *Points;
    struct line_cats *Cats, *ACats;
    char *xstring, *ystring, *zstring;
    size_t xsize, ysize, zsize;
    struct Cell_head window;
    struct ilist *fcats;
    int count, n_skipped;

    /* where || columns */
    struct field_info *Fi;
    dbDriver *driver;
    dbValue value;
    dbHandle handle;
    int *cats, ncats, more;
    dbTable *Table;
    dbString dbstring;
    dbColumn *Column;
    dbValue *Value;
    char *buf;
    size_t bufsize;
    dbCursor cursor;
    /* columns */
    char **columns;
    int *coltypes;
    char *all_columns;
    
    Fi = NULL;
    driver = NULL;
    columns = NULL;
    coltypes = NULL;
    all_columns = NULL;
    
    G_zero(&value, sizeof(dbValue));
    db_init_string(&dbstring);

    xstring = NULL;
    ystring = NULL;
    zstring = NULL;
    xsize = 0;
    ysize = 0;
    zsize = 0;
    buf = NULL;
    bufsize = 0;

    /* get the region */
    G_get_window(&window);

    count = n_lines = ncats = 0;
    xstring = ystring = zstring = NULL;
    cats = NULL;
    
    if (field > 0 && (where || column_names)) {
	Fi = Vect_get_field(Map, field);
	if (!Fi) {
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  field);
	}

	driver = db_start_driver(Fi->driver);
	if (!driver)
	    G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);
	
	db_init_handle(&handle);
	db_set_handle(&handle, Fi->database, NULL);
	
	if (db_open_database(driver, &handle) != DB_OK)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	
	/* select cats (sorted array) */
	ncats = db_select_int(driver, Fi->table, Fi->key, where, &cats);
	G_debug(3, "%d categories selected from table <%s>", ncats, Fi->table);

	if (!column_names) {
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	}
	else {
            int len_all = 0;
            
            if (column_names[0] && strcmp(column_names[0], "*") == 0) {
                int icol, ncols;
                const char *col_name;
                
                /* all columns */
                db_set_string(&dbstring, Fi->table);
                if (db_describe_table(driver, &dbstring, &Table) != DB_OK) {
                    G_warning(_("Unable to describe table <%s>"), Fi->table);
                    return -1;
                }
                
                ncols = db_get_table_number_of_columns(Table);
                /* key column skipped */
                columns = (char **) G_malloc(ncols * sizeof(char *));
                icol = 0;
                for (i = 0; i < ncols; i++) {
                    col_name = db_get_column_name(db_get_table_column(Table, i));
                    if (strcmp(Fi->key, col_name) != 0)
			columns[icol++] = G_store(col_name);
                }
                columns[ncols - 1] = NULL;
                
                db_zero_string(&dbstring);
                db_free_table(Table);
                Table = NULL;
            }
            else {
                int icol, ncols;
                const char *col_name;

		ncols = 0;
		while (column_names[ncols])
		    ncols++;

                columns = (char **) G_malloc((ncols + 1) * sizeof(char *));
		icol = 0;
                for (i = 0; i < ncols; i++) {
                    col_name = column_names[i];
		    /* key column skipped */
                    if (strcmp(Fi->key, col_name) != 0)
			columns[icol++] = G_store(col_name);
                }
                columns[icol] = NULL;
            }
            
            /* selected columns only */
            i = 0;
            while (columns[i])
                len_all += strlen(columns[i++]);
            
	    coltypes = G_malloc(i * sizeof(int));
	    
	    all_columns = G_malloc(len_all + i + 2);

	    i = 0;
	    strcpy(all_columns, columns[0]);
	    while (columns[i]) {
		/* get column types */
		coltypes[i] = db_column_Ctype(driver, Fi->table, columns[i]);
		if (coltypes[i] < 0) {
		    db_close_database(driver);
		    db_shutdown_driver(driver);
		    G_warning(_("Unknown type of column <%s>, export cancelled"),
		              columns[i]);
		    return -1;
		}
		if (i > 0) {
		    strcat(all_columns, ",");
		    strcat(all_columns, columns[i]);
		}
		i++;
	    }
	}
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    ACats = Vect_new_cats_struct();
    fcats = Vect_new_list();

    /* by default, read_next_line will NOT read Dead lines */
    /* but we can override that (in Level I only) by specifying */
    /* the type  -1, which means match all line types */

    Vect_rewind(Map);

    n_skipped = line = 0;
    while (TRUE) {
	ltype = Vect_read_next_line(Map, Points, Cats);
	if (ltype == -1 ) {      /* failure */
	    if (columns) {
		db_close_database(driver);
		db_shutdown_driver(driver);

                free_col_arrays(coltypes, all_columns,
                                column_names && strcmp(column_names[0], "*") == 0 ? columns : NULL);
	    }
	    
	    return -1;
	}

	if (ltype == -2)	{	/* EOF */
	    if (columns) {
		db_close_database(driver);
		db_shutdown_driver(driver);
                
                free_col_arrays(coltypes, all_columns,
                                column_names && strcmp(column_names[0], "*") == 0 ? columns : NULL);
	    }
	    break;
	}

	line++;

	if (!(ltype & type))
	    continue;

	if (format == GV_ASCII_FORMAT_POINT && !(ltype & GV_POINTS))
	    continue;

	found = get_cat(Cats, Clist, cats, ncats, field, &cat);

	if (!found && field > 0 && ltype == GV_BOUNDARY &&
	    type & GV_AREA && Vect_level(Map) > 1) {
	    Vect_get_line_areas(Map, line, &left, &right);
	    if (left < 0)
		left = Vect_get_isle_area(Map, abs(left));
	    if (left > 0) {
		Vect_get_area_cats(Map, left, ACats);
		found = get_cat(ACats, Clist, cats, ncats, field, &cat);
	    }
	    if (right < 0)
		right = Vect_get_isle_area(Map, abs(right));
	    if (!found && right > 0) {
		Vect_get_area_cats(Map, right, ACats);
		found = get_cat(ACats, Clist, cats, ncats, field, &cat);
	    }
	}
	
	if (!found) {
            if (Cats->n_cats < 1)
                n_skipped++;
            
	    continue;
	}

	if (ver < 5) {
	    Vect_cat_get(Cats, 1, &cat);
	}

	switch (ltype) {
	case GV_BOUNDARY:
	    if (ver == 5)
		ctype = 'B';
	    else
		ctype = 'A';
	    break;
	case GV_CENTROID:
	    if (ver < 5) {
		if (att != NULL) {
		    if (cat > 0) {
			G_rasprintf(&xstring, &xsize, "%.*f", dp, Points->x[0]);
			G_trim_decimal(xstring);
			G_rasprintf(&ystring, &ysize, "%.*f", dp, Points->y[0]);
			G_trim_decimal(ystring);
			fprintf(att, "A %s %s %d\n", xstring, ystring, cat);
		    }
		}
		continue;
	    }
	    ctype = 'C';
	    break;
	case GV_LINE:
	    ctype = 'L';
	    break;
	case GV_POINT:
	    ctype = 'P';
	    break;
	case GV_FACE:
	    ctype = 'F';
	    break;
	case GV_KERNEL:
	    ctype = 'K';
	    break;
	default:
	    ctype = 'X';
	    G_warning(_("Unknown feature type %d"), (int)ltype);
	    break;
	}

	if (format == GV_ASCII_FORMAT_POINT) {
	    if (region_flag) {
		if ((window.east < Points->x[0]) ||
		    (window.west > Points->x[0]))
		    continue;
	    }
	    G_rasprintf(&xstring, &xsize, "%.*f", dp, Points->x[0]);
	    G_trim_decimal(xstring);

	    if (region_flag) {
		if ((window.north < Points->y[0]) ||
		    (window.south > Points->y[0]))
		    continue;
	    }
	    G_rasprintf(&ystring, &ysize, "%.*f", dp, Points->y[0]);
	    G_trim_decimal(ystring);

	    Vect_field_cat_get(Cats, field, fcats);

	    /* print header */
	    if (header && count < 1) {
		count++;
		if (Map->head.with_z)
		    fprintf(ascii, "east%snorth%sheight%scat", fs, fs, fs);
		else
		    fprintf(ascii, "east%snorth%scat", fs, fs);
		if (columns) {
		    for (i = 0; columns[i]; i++) {
			if (db_select_value
			    (driver, Fi->table, Fi->key, cat,
			     columns[i], &value) < 0)
			    G_fatal_error(_("Unable to select record from table <%s> (key %s, column %s)"),
					  Fi->table, Fi->key, columns[i]);
			if (columns[i])
			    fprintf(ascii, "%s%s", fs, columns[i]);
			else
			    fprintf(ascii, "%s", columns[i]); /* can not happen */
		    }
		}
		fprintf(ascii, "\n");

	    }

	    if (Map->head.with_z && ver == 5) {
		if (region_flag) {
		    if ((window.top < Points->z[0]) ||
			(window.bottom > Points->z[0]))
			continue;
		}
		G_rasprintf(&zstring, &zsize, "%.*f", dp, Points->z[0]);
		G_trim_decimal(zstring);
		fprintf(ascii, "%s%s%s%s%s", xstring, fs, ystring, fs,
			zstring);
	    }
	    else {
		fprintf(ascii, "%s%s%s", xstring, fs, ystring);
	    }

	    
	    if (fcats->n_values > 0 && cat > -1) {
		if (fcats->n_values > 1) {
		    G_warning(_("Feature has more categories. Only one category (%d) "
				"is exported."), cat);
		}
		fprintf(ascii, "%s%d", fs, cat);
		
		/* print attributes */
		if (columns) {

		    G_rasprintf(&buf, &bufsize, "SELECT %s FROM %s WHERE %s = %d",
			    all_columns, Fi->table, Fi->key, cat);
		    G_debug(2, "SQL: %s", buf);
		    db_set_string(&dbstring, buf);

		    if (db_open_select_cursor
				    (driver, &dbstring, &cursor, DB_SEQUENTIAL) != DB_OK) {
			db_close_database(driver);
			db_shutdown_driver(driver);
			G_fatal_error(_("Cannot select attributes for cat = %d"),
			  cat);
		    }
		    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK) {
			db_close_database(driver);
			db_shutdown_driver(driver);
			G_fatal_error(_("Unable to fetch data from table"));
		    }

		    Table = db_get_cursor_table(&cursor);


		    for (i = 0; columns[i]; i++) {
			Column = db_get_table_column(Table, i);
			Value = db_get_column_value(Column);

			if (db_test_value_isnull(Value)) {
			    fprintf(ascii, "%s", fs);
			}
			else {
			    switch(coltypes[i])
			    {
			    case DB_C_TYPE_INT: {
				fprintf(ascii, "%s%d", fs, db_get_value_int(Value));
				break;
			    }
			    case DB_C_TYPE_DOUBLE: {
				fprintf(ascii, "%s%.*f", fs, dp, db_get_value_double(Value));
				break;
			    }
			    case DB_C_TYPE_STRING: {
				fprintf(ascii, "%s%s", fs, db_get_value_string(Value));
				break;
			    }
			    case DB_C_TYPE_DATETIME: {
				break;
			    }
			    case -1:
				G_fatal_error(_("Column <%s> not found in table <%s>"),
					      columns[i], Fi->table);
			    default: G_fatal_error(_("Column <%s>: unsupported data type"),
						   columns[i]);
			    }
			}
		    }
		    db_close_cursor(&cursor);
		}
	    }

	    fprintf(ascii, "\n");
	}
	else if (format == GV_ASCII_FORMAT_STD) {
	    /* FORMAT_STANDARD */
	    if (ver == 5 && Cats->n_cats > 0)
		fprintf(ascii, "%c  %d %d\n", ctype, Points->n_points,
			Cats->n_cats);
	    else
		fprintf(ascii, "%c  %d\n", ctype, Points->n_points);

	    xptr = Points->x;
	    yptr = Points->y;
	    zptr = Points->z;

	    while (Points->n_points--) {

		G_rasprintf(&xstring, &xsize, "%.*f", dp, *xptr++);
		G_trim_decimal(xstring);
		G_rasprintf(&ystring, &ysize, "%.*f", dp, *yptr++);
		G_trim_decimal(ystring);

		if (ver == 5) {
		    if (Map->head.with_z) {
			G_rasprintf(&zstring, &zsize, "%.*f", dp, *zptr++);
			G_trim_decimal(zstring);
			fprintf(ascii, " %-12s %-12s %-12s\n", xstring,
				ystring, zstring);
		    }
		    else {
			fprintf(ascii, " %-12s %-12s\n", xstring, ystring);
		    }
		}		/*Version 4 */
		else {
		    fprintf(ascii, " %-12s %-12s\n", ystring, xstring);
		}
	    }

	    if (ver == 5) {
		for (i = 0; i < Cats->n_cats; i++) {
		    fprintf(ascii, " %-5d %-10d\n", Cats->field[i],
			    Cats->cat[i]);
		}
	    }
	    else {
		if (cat > -1) {
		    if (ltype == GV_POINT) {
			G_rasprintf(&xstring, &xsize, "%.*f", dp, Points->x[0]);
			G_trim_decimal(xstring);
			G_rasprintf(&ystring, &ysize, "%.*f", dp, Points->y[0]);
			G_trim_decimal(ystring);
			fprintf(att, "P %s %s %d\n", xstring, ystring, cat);
		    }
		    else {
			x = (Points->x[1] + Points->x[0]) / 2;
			y = (Points->y[1] + Points->y[0]) / 2;

			G_rasprintf(&xstring, &xsize, "%.*f", dp, x);
			G_trim_decimal(xstring);
			G_rasprintf(&ystring, &ysize, "%.*f", dp, y);
			G_trim_decimal(ystring);
			fprintf(att, "L %s %s %d\n", xstring, ystring, cat);
		    }
		}
	    }
	}
	else if (format == GV_ASCII_FORMAT_WKT) {
	    if (ltype & (GV_BOUNDARY | GV_CENTROID | GV_FACE | GV_KERNEL))
		continue;
	    /* Well-Known Text */
	    Vect_sfa_line_astext(Points, ltype, Vect_is_3d(Map), dp, ascii);
	}
	else {
	    G_fatal_error(_("Unknown format"));
	}
	n_lines++;
    }

    if (format == GV_ASCII_FORMAT_WKT) {
	/* process areas - topology required */
	int i, area, nareas, isle, nisles;

	if (Vect_level(Map) < 2) {
	    G_warning(_("Topology not available, unable to process areas"));
	    nareas = 0;
	}
	else {
	    nareas = Vect_get_num_areas(Map);
	}
	for (area = 1; area <= nareas; area++) {
	    if (!Vect_area_alive(Map, area)) /* skip dead areas */
		continue;
	    if (Vect_get_area_cat(Map, area, field) < 0)
		continue;
	    /* get boundary -> linearring */
	    if (Vect_get_area_points(Map, area, Points) < 0) {
		G_warning(_("Unable to get boundary of area id %d"), area);
		continue;
	    }
	    fprintf(ascii, "POLYGON(");
	    /* write outter ring */
	    Vect_sfa_line_astext(Points, GV_BOUNDARY, 0, dp, ascii); /* boundary is always 2D */
	    /* get isles (holes) -> inner rings */
	    nisles = Vect_get_area_num_isles(Map, area);
	    for (i = 0; i < nisles; i++) {
		/* get isle boundary -> linearring */
		isle = Vect_get_area_isle(Map, area, i);
		if (Vect_get_isle_points(Map, isle, Points) < 0) {
		    G_warning(_("Unable to get boundary of isle id %d (area id %d)"), isle, area);
		    continue;
		}
		fprintf(ascii, ", ");
		/* write inner ring */
		Vect_sfa_line_astext(Points, GV_BOUNDARY, 0, dp, ascii); /* boundary is always 2D */
	    }
	    fprintf(ascii, ")\n");
	}
    }

    if (n_skipped > 0)
        G_important_message(_("%d features without category skipped. To export also "
                              "features without category use '%s=-1'."), n_skipped, "layer");
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_cats_struct(ACats);
    
    return n_lines;
}

int srch(const void *pa, const void *pb)
{
    int *p1 = (int *)pa;
    int *p2 = (int *)pb;
    
    if (*p1 < *p2)
	return -1;
    if (*p1 > *p2)
	return 1;
    return 0;
}

/*!
  \brief Write data to GRASS ASCII vector format

  \param[out] dascii pointer to the output ASCII file
  \param Map    pointer to Map_info structure
*/
void Vect_write_ascii_head(FILE *dascii, struct Map_info *Map)
{
    fprintf(dascii, "ORGANIZATION: %s\n", Vect_get_organization(Map));
    fprintf(dascii, "DIGIT DATE:   %s\n", Vect_get_date(Map));
    fprintf(dascii, "DIGIT NAME:   %s\n", Vect_get_person(Map));
    fprintf(dascii, "MAP NAME:     %s\n", Vect_get_map_name(Map));
    fprintf(dascii, "MAP DATE:     %s\n", Vect_get_map_date(Map));
    fprintf(dascii, "MAP SCALE:    %d\n", Vect_get_scale(Map));
    fprintf(dascii, "OTHER INFO:   %s\n", Vect_get_comment(Map));
    fprintf(dascii, "ZONE:         %d\n", Vect_get_zone(Map));
    fprintf(dascii, "MAP THRESH:   %f\n", Vect_get_thresh(Map));
}

/* check category */
int get_cat(const struct line_cats *Cats, const struct cat_list *Clist,
	      const int *cats, int ncats, int field, int *cat)
{
    int i;
    
    *cat = -1;
    
    if (field < 1)
	return TRUE;
    
    if (Clist && Clist->field == field) {
	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == field &&
		Vect_cat_in_cat_list(Cats->cat[i], Clist)) {
		*cat = Cats->cat[i];
		return TRUE;
	    }
	}
	return FALSE;
    }
    if (cats) {
	int *found;

	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == field) {
		found = (int *)bsearch((void *) &(Cats->cat[i]), cats, 
		                       ncats, sizeof(int), srch);
		if (found) {
		    /* found */
		    *cat = *found;
		    return TRUE;
		}
	    }
	}
	return FALSE;
    }
    if (!Clist && !cats && field > 0) {
	Vect_cat_get(Cats, field, cat);
	if (*cat > -1)
	    return TRUE;
    }
    
    return FALSE;
}

/* free column arrays, see Vect_write_ascii() */
void free_col_arrays(int *coltypes, char *all_columns, char **columns)
{
    G_free(coltypes);
    G_free(all_columns);
    if (columns) {
        int i = 0;

        while (columns[i])
            G_free(columns[i++]);
        G_free(columns);
    }
}
