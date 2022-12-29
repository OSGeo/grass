
/***************************************************************
 *
 * MODULE:       v.univar
 * 
 * AUTHOR(S):    Radim Blazek
 *               Hamish Bowman, University of Otago, New Zealand (r.univar2)
 *               Martin Landa (extended stats & OGR support)
 *               
 * PURPOSE:      Univariate Statistics for attribute
 *               
 * COPYRIGHT:    (C) 2004-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

/* TODO
 * - add flag to weigh by line/boundary length and area size
 *   Roger Bivand on GRASS devel ml on July 2 2004
 *   http://lists.osgeo.org/pipermail/grass-dev/2004-July/014976.html
 *   "[...] calculating weighted means, weighting by line length
 *   or area surface size [does not make sense]. I think it would be
 *   better to treat each line or area as a discrete, unweighted, unit
 *   unless some reason to the contrary is given, just like points/sites.
 *   It is probably more important to handle missing data gracefully
 *   than weight the means or other statistics, I think. There may be
 *   reasons to weigh sometimes, but most often I see ratios or rates of
 *   two variables, rather than of a single variable and length or area."
 *   MM 04 2013: Done
 * 
 * - use geodesic distances for the geometry option (distance to closest
 *   other feature)
 * 
 * - use sample instead of population mean/stddev
 * */



#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

static void select_from_geometry(void);
static void select_from_database(void);
static void summary(void);

struct Option *field_opt, *where_opt, *col_opt;
struct Flag *shell_flag, *ext_flag, *weight_flag, *geometry;
struct Map_info Map;
struct line_cats *Cats;
struct field_info *Fi;
dbDriver *Driver;
dbCatValArray Cvarr;
int otype, ofield;
int compatible = 1;		/* types are compatible: point+centroid or line+boundary or area */
int nmissing = 0;		/* number of missing attributes */
int nnull = 0;		/* number of null values */
int nzero = 0;		/* number of zero distances */
int first = 1;

/* Statistics */
int count = 0;		/* number of features with non-null attribute */
int nlines;          /* number of primitives */
double sum = 0.0;
double sumsq = 0.0;
double sumcb = 0.0;
double sumqt = 0.0;
double sum_abs = 0.0;
double min = 0.0 / 0.0;	/* init as nan */
double max = 0.0 / 0.0;
double mean, mean_abs, pop_variance, sample_variance, pop_stdev,
    sample_stdev, pop_coeff_variation, kurtosis, skewness;
double total_size = 0.0;	/* total size: length/area */

/* Extended statistics */
int perc;

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map_opt, *type_opt,
	*percentile;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("univariate statistics"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("geometry"));
    module->label =
	_("Calculates univariate statistics of vector map features.");
    module->description = _("Variance and standard "
			    "deviation is calculated only for points if specified.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line,boundary,centroid,area";
    type_opt->answer = "point,line,area";

    col_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    col_opt->required = NO;

    where_opt = G_define_standard_option(G_OPT_DB_WHERE);

    percentile = G_define_option();
    percentile->key = "percentile";
    percentile->type = TYPE_INTEGER;
    percentile->required = NO;
    percentile->options = "0-100";
    percentile->answer = "90";
    percentile->description =
	_("Percentile to calculate (requires extended statistics flag)");

    shell_flag = G_define_flag();
    shell_flag->key = 'g';
    shell_flag->description = _("Print the stats in shell script style");

    ext_flag = G_define_flag();
    ext_flag->key = 'e';
    ext_flag->description = _("Calculate extended statistics");

    weight_flag = G_define_flag();
    weight_flag->key = 'w';
    weight_flag->description = _("Weigh by line length or area size");

    geometry = G_define_flag();
    geometry->key = 'd';
    geometry->description = _("Calculate geometric distances instead of attribute statistics");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Only require col_opt answer if -g flag is not set */
    if (!col_opt->answer && !geometry->answer) {
	G_fatal_error(_("Required parameter <%s> not set:\n\t(%s)"), col_opt->key, col_opt->description);
    }

    otype = Vect_option_to_types(type_opt);
    perc = atoi(percentile->answer);

    Cats = Vect_new_cats_struct();

    /* open input vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&Map, map_opt->answer, "", field_opt->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), map_opt->answer);

    ofield = Vect_get_field_number(&Map, field_opt->answer);
    
    if ((otype & GV_POINT) && Vect_get_num_primitives(&Map, GV_POINT) == 0) {
	otype &= ~(GV_POINT);
	if (otype & GV_POINT)
	    G_fatal_error("Bye");
    }
    if ((otype & GV_CENTROID) && Vect_get_num_primitives(&Map, GV_CENTROID) == 0) {
	otype &= ~(GV_CENTROID);
    }
    if ((otype & GV_LINE) && Vect_get_num_primitives(&Map, GV_LINE) == 0) {
	otype &= ~(GV_LINE);
    }
    if ((otype & GV_BOUNDARY) && Vect_get_num_primitives(&Map, GV_BOUNDARY) == 0) {
	otype &= ~(GV_BOUNDARY);
    }
    if ((otype & GV_AREA) && Vect_get_num_areas(&Map) == 0) {
	otype &= ~(GV_AREA);
    }

    /* Check if types are compatible */
    if ((otype & GV_POINTS) && ((otype & GV_LINES) || (otype & GV_AREA)))
	compatible = 0;
    if ((otype & GV_LINES) && (otype & GV_AREA))
	compatible = 0;
    if (!compatible && geometry->answer)
	compatible = 1; /* distances is compatible with GV_POINTS and GV_LINES */

    if (!compatible && !weight_flag->answer)
	compatible = 1; /* attributes are always compatible without weight */

    if (geometry->answer && (otype & GV_AREA))
	G_fatal_error(_("Geometry distances are not supported for areas. Use '%s' instead."), "v.distance");

    if (!compatible) {
	G_warning(_("Incompatible vector type(s) specified, only number of features, minimum, maximum and range "
		   "can be calculated"));
    }

    if (ext_flag->answer && (!(otype & GV_POINTS) || geometry->answer)) {
	G_warning(_("Extended statistics is currently supported only for points/centroids"));
    }

    if (geometry->answer)
	select_from_geometry();
    else
	select_from_database();
    summary();

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}

void select_from_geometry(void)
{
    int i, j, k, ncats, *cats;
    int type;
    struct line_pnts *iPoints, *jPoints;

    iPoints = Vect_new_line_struct();
    jPoints = Vect_new_line_struct();

    if (where_opt->answer != NULL) {
	if (ofield < 1) {
	    G_fatal_error(_("'layer' must be > 0 for 'where'."));
	}
	Fi = Vect_get_field(&Map, ofield);
	if (!Fi) {
	    G_fatal_error(_("Database connection not defined for layer %d"),
			  ofield);
	}

	Driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (Driver == NULL)
	    G_fatal_error("Unable to open database <%s> by driver <%s>",
			  Fi->database, Fi->driver);
        db_set_error_handler_driver(Driver);

	ncats = db_select_int(Driver, Fi->table, Fi->key, where_opt->answer,
			      &cats);
	if (ncats == -1)
		G_fatal_error(_("Unable select categories from table <%s>"), Fi->table);

	db_close_database_shutdown_driver(Driver);

    }
    else
	ncats = 0;

    count = 0;

    nlines = Vect_get_num_lines(&Map);
    G_message(_("Calculating geometric distances between %d primitives..."), nlines);
    /* Start calculating the statistics based on distance to all other primitives.
       Use the centroid of areas and the first point of lines */
    for (i = 1; i <= nlines; i++) {

	G_percent(i, nlines, 2);
	type = Vect_read_line(&Map, iPoints, Cats, i);

	if (!(type & otype))
	    continue;

	if (where_opt->answer) {
	    int ok = FALSE;

	    for (j = 0; j < Cats->n_cats; j++) {
		if (Vect_cat_in_array(Cats->cat[j], cats, ncats)) {
		    ok = TRUE;
		    break;
		}
	    }
	    if (!ok)
	        continue;
	}
	for (j = i + 1; j < nlines; j++) {
	    /* get distance to this object */
	    double val = 0.0;

	    type = Vect_read_line(&Map, jPoints, Cats, j);

	    if (!(type & otype))
		continue;

	    /* now calculate the min distance between each point in line i with line j */
	    for (k = 0; k < iPoints->n_points; k++) {
		double dmin = 0.0;

		Vect_line_distance(jPoints, iPoints->x[k], iPoints->y[k], iPoints->z[k], 1,
				       NULL, NULL, NULL, &dmin, NULL, NULL);
		if((k == 0) || (dmin < val)) {
		    val = dmin;
		}
	    }
	    if (val > 0 && iPoints->n_points > 1) {
		for (k = 0; k < jPoints->n_points; k++) {
		    double dmin = 0.0;

		    Vect_line_distance(iPoints, jPoints->x[k], jPoints->y[k], jPoints->z[k], 1,
					   NULL, NULL, NULL, &dmin, NULL, NULL);
		    if(dmin < val) {
			val = dmin;
		    }
		}
	    }
	    if (val > 0 && iPoints->n_points > 1 && jPoints->n_points > 1) {
		if (Vect_line_check_intersection(iPoints, jPoints, Vect_is_3d(&Map)))
		    val = 0;
	    }
	    if (val == 0) {
		nzero++;
		continue;
	    }
	    if (first) {
		max = val;
		min = val;
		first = 0;
	    }
	    else {
		if (val > max)
		    max = val;
		if (val < min)
		    min = val;
	    }
	    sum += val;
	    sumsq += val * val;
	    sumcb += val * val * val;
	    sumqt += val * val * val * val;
	    sum_abs += fabs(val);
	    count++;
	    G_debug(3, "i=%d j=%d sum = %f val=%f", i, j, sum, val);
	}
    }
}

void select_from_database(void)
{
    int nrec, ctype, nlines, line, nareas, area;
    struct line_pnts *Points;

    Fi = Vect_get_field(&Map, ofield);
    if (Fi == NULL) {
	G_fatal_error(_(" Database connection not defined for layer <%s>"), field_opt->answer);
    }

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL)
	G_fatal_error("Unable to open database <%s> by driver <%s>",
		      Fi->database, Fi->driver);
    db_set_error_handler_driver(Driver);

    /* check if column exists */
    ctype = db_column_Ctype(Driver, Fi->table, col_opt->answer);
    if (ctype == -1)
        G_fatal_error(_("Column <%s> not found in table <%s>"),
                      col_opt->answer, Fi->table);
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Only numeric column type is supported"));
    
    /* Note do not check if the column exists in the table because it may be an expression */
    db_CatValArray_init(&Cvarr);
    nrec = db_select_CatValArray(Driver, Fi->table, Fi->key, col_opt->answer,
                                 where_opt->answer, &Cvarr);
    G_debug(2, "db_select_CatValArray() nrec = %d", nrec);
    if (nrec < 0)
	G_fatal_error(_("Unable to select data from table"));

    db_close_database_shutdown_driver(Driver);

    Points = Vect_new_line_struct();

    /* Lines */
    nlines = 0;
    if ((otype & GV_POINTS) || (otype & GV_LINES))
	nlines = Vect_get_num_lines(&Map);

    G_debug(1, "select_from_database: %d points", nlines);
    for (line = 1; line <= nlines; line++) {
	int i, type;

	G_debug(3, "line = %d", line);

	G_percent(line, nlines, 2);
	type = Vect_read_line(&Map, Points, Cats, line);
	if (!(type & otype))
	    continue;

	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == ofield) {
		double val = 0.0;
		dbCatVal *catval;

		G_debug(3, "cat = %d", Cats->cat[i]);

		if (db_CatValArray_get_value(&Cvarr, Cats->cat[i], &catval) !=
		    DB_OK) {
		    G_debug(3, "No record for cat = %d", Cats->cat[i]);
		    nmissing++;
		    continue;
		}

		if (catval->isNull) {
		    G_debug(3, "NULL value for cat = %d", Cats->cat[i]);
		    nnull++;
		    continue;
		}

		if (ctype == DB_C_TYPE_INT) {
		    val = catval->val.i;
		}
		else if (ctype == DB_C_TYPE_DOUBLE) {
		    val = catval->val.d;
		}

		count++;

		if (first) {
		    max = val;
		    min = val;
		    first = 0;
		}
		else {
		    if (val > max)
			max = val;
		    if (val < min)
			min = val;
		}

		if (compatible) {
		    if (type & GV_POINTS) {
			sum += val;
			sumsq += val * val;
			sumcb += val * val * val;
			sumqt += val * val * val * val;
			sum_abs += fabs(val);
		    }
		    else if (type & GV_LINES) {	/* GV_LINES */
			double l = 1.;

			if (weight_flag->answer)
			    l = Vect_line_length(Points);

			sum += l * val;
			sumsq += l * val * val;
			sumcb += l * val * val * val;
			sumqt += l * val * val * val * val;
			sum_abs += l * fabs(val);
			total_size += l;
		    }
		}
		G_debug(3, "sum = %f total_size = %f", sum, total_size);
	    }
	}
    }

    if (otype & GV_AREA) {
	nareas = Vect_get_num_areas(&Map);
	for (area = 1; area <= nareas; area++) {
	    int i, centr;

	    G_debug(3, "area = %d", area);

	    centr = Vect_get_area_centroid(&Map, area);
	    if (centr < 1)
		continue;

	    G_debug(3, "centr = %d", centr);
	    Vect_read_line(&Map, NULL, Cats, centr);

	    for (i = 0; i < Cats->n_cats; i++) {
		if (Cats->field[i] == ofield) {
		    double val = 0.0;
		    dbCatVal *catval;

		    G_debug(3, "cat = %d", Cats->cat[i]);

		    if (db_CatValArray_get_value
			(&Cvarr, Cats->cat[i], &catval) != DB_OK) {
			G_debug(3, "No record for cat = %d", Cats->cat[i]);
			nmissing++;
			continue;
		    }

		    if (catval->isNull) {
			G_debug(3, "NULL value for cat = %d", Cats->cat[i]);
			nnull++;
			continue;
		    }

		    if (ctype == DB_C_TYPE_INT) {
			val = catval->val.i;
		    }
		    else if (ctype == DB_C_TYPE_DOUBLE) {
			val = catval->val.d;
		    }

		    count++;

		    if (first) {
			max = val;
			min = val;
			first = 0;
		    }
		    else {
			if (val > max)
			    max = val;
			if (val < min)
			    min = val;
		    }

		    if (compatible) {
			double a = 1.;

			if (weight_flag->answer)
			    a = Vect_get_area_area(&Map, area);

			sum += a * val;
			sumsq += a * val * val;
			sumcb += a * val * val * val;
			sumqt += a * val * val * val * val;
			sum_abs += a * fabs(val);
			total_size += a;
		    }
		    G_debug(4, "sum = %f total_size = %f", sum, total_size);
		}
	    }
	}
    }

    G_debug(2, "sum = %f total_size = %f", sum, total_size);
}

void summary(void)
{
    if (compatible) {
	if (!geometry->answer && weight_flag->answer) {
	    mean = sum / total_size;
	    mean_abs = sum_abs / total_size;

	    /* Roger Bivand says it is wrong see GRASS devel list 7/2004 */
	    /*
	    pop_variance = (sumsq - sum * sum / total_size) / total_size;
	    pop_stdev = sqrt(pop_variance);
	    pop_coeff_variation = pop_stdev / (sqrt(sum * sum) / count);
	    */
	}
	else {
	    double n = count;

	    mean = sum / count;
	    mean_abs = sum_abs / count;
	    pop_variance = (sumsq - sum * sum / count) / count;
	    pop_stdev = sqrt(pop_variance);
	    pop_coeff_variation = pop_stdev / (sqrt(sum * sum) / count);
	    sample_variance = (sumsq - sum * sum / count) / (count - 1);
	    sample_stdev = sqrt(sample_variance);
	    kurtosis =
		(sumqt / count - 4 * sum * sumcb / (n * n) +
		 6 * sum * sum * sumsq / (n * n * n) -
		 3 * sum * sum * sum * sum / (n * n * n * n))
		/ (sample_stdev * sample_stdev * sample_stdev *
		   sample_stdev) - 3;
	    skewness =
		(sumcb / n - 3 * sum * sumsq / (n * n) +
		 2 * sum * sum * sum / (n * n * n))
		/ (sample_stdev * sample_stdev * sample_stdev);
	}
    }

    G_debug(3, "otype %d:", otype);

    if (shell_flag->answer) {
	fprintf(stdout, "n=%d\n", count);
	if (geometry->answer) {
	    fprintf(stdout, "nzero=%d\n", nzero);
	}
	else {
	    fprintf(stdout, "nmissing=%d\n", nmissing);
	    fprintf(stdout, "nnull=%d\n", nnull);
	}
	if (count > 0) {
	    fprintf(stdout, "min=%g\n", min);
	    fprintf(stdout, "max=%g\n", max);
	    fprintf(stdout, "range=%g\n", max - min);
	    fprintf(stdout, "sum=%g\n", sum);
	    if (compatible) {
		fprintf(stdout, "mean=%g\n", mean);
		fprintf(stdout, "mean_abs=%g\n", mean_abs);
		if (geometry->answer || !weight_flag->answer) {
		    fprintf(stdout, "population_stddev=%g\n", pop_stdev);
		    fprintf(stdout, "population_variance=%g\n", pop_variance);
		    fprintf(stdout, "population_coeff_variation=%g\n",
			    pop_coeff_variation);
		    fprintf(stdout, "sample_stddev=%g\n", sample_stdev);
		    fprintf(stdout, "sample_variance=%g\n", sample_variance);
		    fprintf(stdout, "kurtosis=%g\n", kurtosis);
		    fprintf(stdout, "skewness=%g\n", skewness);
		}
	    }
	}
    }
    else {
	if (geometry->answer) {
	    fprintf(stdout, "number of primitives: %d\n", nlines);
	    fprintf(stdout, "number of non zero distances: %d\n", count);
	    fprintf(stdout, "number of zero distances: %d\n", nzero);
	}
	else {
	    fprintf(stdout, "number of features with non NULL attribute: %d\n",
		    count);
	    fprintf(stdout, "number of missing attributes: %d\n", nmissing);
	    fprintf(stdout, "number of NULL attributes: %d\n", nnull);
	}
	if (count > 0) {
	    fprintf(stdout, "minimum: %g\n", min);
	    fprintf(stdout, "maximum: %g\n", max);
	    fprintf(stdout, "range: %g\n", max - min);
	    fprintf(stdout, "sum: %g\n", sum);
	    if (compatible) {
		fprintf(stdout, "mean: %g\n", mean);
		fprintf(stdout, "mean of absolute values: %g\n", mean_abs);
		if (geometry->answer || !weight_flag->answer) {
		    fprintf(stdout, "population standard deviation: %g\n",
			    pop_stdev);
		    fprintf(stdout, "population variance: %g\n", pop_variance);
		    fprintf(stdout, "population coefficient of variation: %g\n",
			    pop_coeff_variation);
		    fprintf(stdout, "sample standard deviation: %g\n",
			    sample_stdev);
		    fprintf(stdout, "sample variance: %g\n", sample_variance);
		    fprintf(stdout, "kurtosis: %g\n", kurtosis);
		    fprintf(stdout, "skewness: %g\n", skewness);
		}
	    }
	}
    }

    /* TODO: mode, skewness, kurtosis */
    /* Not possible to calculate for point distance, since we don't collect the population */
    if (ext_flag->answer && compatible && 
        ((otype & GV_POINTS) || !weight_flag->answer) && 
	!geometry->answer && count > 0) {
	double quartile_25 = 0.0, quartile_75 = 0.0, quartile_perc = 0.0;
	double median = 0.0;
	int qpos_25, qpos_75, qpos_perc;

	qpos_25 = (int)(count * 0.25 - 0.5);
	qpos_75 = (int)(count * 0.75 - 0.5);
	qpos_perc = (int)(count * perc / 100. - 0.5);

	if (db_CatValArray_sort_by_value(&Cvarr) != DB_OK)
	    G_fatal_error(_("Cannot sort the key/value array"));

	if (Cvarr.ctype == DB_C_TYPE_INT) {
	    quartile_25 = (Cvarr.value[qpos_25]).val.i;
	    if (count % 2)	/* odd */
		median = (Cvarr.value[(int)(count / 2)]).val.i;
	    else		/* even */
		median =
		    ((Cvarr.value[count / 2 - 1]).val.i +
		     (Cvarr.value[count / 2]).val.i) / 2.0;
	    quartile_75 = (Cvarr.value[qpos_75]).val.i;
	    quartile_perc = (Cvarr.value[qpos_perc]).val.i;
	}
	else {			/* must be DB_C_TYPE_DOUBLE */
	    quartile_25 = (Cvarr.value[qpos_25]).val.d;
	    if (count % 2)	/* odd */
		median = (Cvarr.value[(int)(count / 2)]).val.d;
	    else		/* even */
		median =
		    ((Cvarr.value[count / 2 - 1]).val.d +
		     (Cvarr.value[count / 2]).val.d) / 2.0;
	    quartile_75 = (Cvarr.value[qpos_75]).val.d;
	    quartile_perc = (Cvarr.value[qpos_perc]).val.d;
	}

	if (shell_flag->answer) {
	    fprintf(stdout, "first_quartile=%g\n", quartile_25);
	    fprintf(stdout, "median=%g\n", median);
	    fprintf(stdout, "third_quartile=%g\n", quartile_75);
	    fprintf(stdout, "percentile_%d=%g\n", perc, quartile_perc);
	}
	else {
	    fprintf(stdout, "1st quartile: %g\n", quartile_25);
	    if (count % 2)
		fprintf(stdout, "median (odd number of cells): %g\n", median);
	    else
		fprintf(stdout, "median (even number of cells): %g\n",
			median);
	    fprintf(stdout, "3rd quartile: %g\n", quartile_75);

	    if (perc % 10 == 1 && perc != 11)
		fprintf(stdout, "%dst percentile: %g\n", perc, quartile_perc);
	    else if (perc % 10 == 2 && perc != 12)
		fprintf(stdout, "%dnd percentile: %g\n", perc, quartile_perc);
	    else if (perc % 10 == 3 && perc != 13)
		fprintf(stdout, "%drd percentile: %g\n", perc, quartile_perc);
	    else
		fprintf(stdout, "%dth percentile: %g\n", perc, quartile_perc);
	}
    }
}
