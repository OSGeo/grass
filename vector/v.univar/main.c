
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
 * COPYRIGHT:    (C) 2004-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map_opt, *type_opt, *field_opt, *col_opt, *where_opt,
	*percentile;
    struct Flag *shell_flag, *extended;
    struct Map_info Map;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray Cvarr;
    struct line_pnts *Points;
    struct line_cats *Cats;
    int otype, ofield;
    int compatible = 1;		/* types are compatible: point+centroid or line+boundary or area */
    int nrec, ctype, nlines, line, nareas, area;
    int nmissing = 0;		/* number of missing atttributes */
    int nnull = 0;		/* number of null values */
    int first = 1;

    /* Statistics */
    int count = 0;		/* number of features with non-null attribute */
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

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    module->label =
	_("Calculates univariate statistics for attribute.");
    module->description = _("Variance and standard "
			    "deviation is calculated only for points if specified.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "point,line,boundary,centroid,area";
    type_opt->answer = "point,line,area";

    col_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    col_opt->required = YES;

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

    extended = G_define_flag();
    extended->key = 'e';
    extended->description = _("Calculate extended statistics");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    otype = Vect_option_to_types(type_opt);
    perc = atoi(percentile->answer);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* open input vector */
    Vect_set_open_level(2);
    Vect_open_old2(&Map, map_opt->answer, "", field_opt->answer);
    ofield = Vect_get_field_number(&Map, field_opt->answer);

    /* Check if types are compatible */
    if ((otype & GV_POINTS) && ((otype & GV_LINES) || (otype & GV_AREA)))
	compatible = 0;
    if ((otype & GV_LINES) && (otype & GV_AREA))
	compatible = 0;

    if (!compatible) {
	G_warning(_("Incompatible vector type(s) specified, only number of features, minimum, maximum and range "
		   "can be calculated"));
    }

    if (extended->answer && !(otype & GV_POINTS)) {
	G_warning(_("Extended statistics is currently supported only for points/centroids"));
    }

    /* Read attributes */
    db_CatValArray_init(&Cvarr);
    Fi = Vect_get_field(&Map, ofield);
    if (Fi == NULL) {
	G_fatal_error(_(" Database connection not defined for layer <%s>"), field_opt->answer);
    }

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL)
	G_fatal_error("Unable to open database <%s> by driver <%s>",
		      Fi->database, Fi->driver);

    /* Note do not check if the column exists in the table because it may be an expression */

    nrec =
	db_select_CatValArray(Driver, Fi->table, Fi->key, col_opt->answer,
			      where_opt->answer, &Cvarr);
    G_debug(2, "nrec = %d", nrec);

    ctype = Cvarr.ctype;
    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Column type not supported"));

    if (nrec < 0)
	G_fatal_error(_("Unable to select data from table"));

    db_close_database_shutdown_driver(Driver);

    /* Lines */
    nlines = Vect_get_num_lines(&Map);

    for (line = 1; line <= nlines; line++) {
	int i, type;

	G_debug(3, "line = %d", line);

	type = Vect_read_line(&Map, Points, Cats, line);
	if (!(type & otype))
	    continue;

	for (i = 0; i < Cats->n_cats; i++) {
	    if (Cats->field[i] == ofield) {
		double val;
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
		    else {	/* GV_LINES */
			double l;

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
		    double val;
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
			double a;

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

    if (compatible) {
	if ((otype & GV_LINES) || (otype & GV_AREA)) {
	    mean = sum / total_size;
	    mean_abs = sum_abs / total_size;
	    /* Roger Bivand says it is wrong see GRASS devel list 7/2004 */
	    /*
	       pop_variance = (sumsq - sum*sum/total_size)/total_size;
	       pop_stdev = sqrt(pop_variance);
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
	fprintf(stdout, "nmissing=%d\n", nmissing);
	fprintf(stdout, "nnull=%d\n", nnull);
	if (count > 0) {
	    fprintf(stdout, "min=%g\n", min);
	    fprintf(stdout, "max=%g\n", max);
	    fprintf(stdout, "range=%g\n", max - min);
	    if (compatible && (otype & GV_POINTS)) {
		fprintf(stdout, "mean=%g\n", mean);
		fprintf(stdout, "mean_abs=%g\n", mean_abs);
		fprintf(stdout, "population_stddev=%g\n", pop_stdev);
		fprintf(stdout, "population_variance=%g\n", pop_variance);
		fprintf(stdout, "population_coeff_variation=%g\n",
			pop_coeff_variation);
		if (otype & GV_POINTS) {
		    fprintf(stdout, "sample_stddev=%g\n", sample_stdev);
		    fprintf(stdout, "sample_variance=%g\n", sample_variance);
		    fprintf(stdout, "kurtosis=%g\n", kurtosis);
		    fprintf(stdout, "skewness=%g\n", skewness);
		}
	    }
	}
    }
    else {
	fprintf(stdout, "number of features with non NULL attribute: %d\n",
		count);
	fprintf(stdout, "number of missing attributes: %d\n", nmissing);
	fprintf(stdout, "number of NULL attributes: %d\n", nnull);
	if (count > 0) {
	    fprintf(stdout, "minimum: %g\n", min);
	    fprintf(stdout, "maximum: %g\n", max);
	    fprintf(stdout, "range: %g\n", max - min);
	    if (compatible && (otype & GV_POINTS)) {
		fprintf(stdout, "mean: %g\n", mean);
		fprintf(stdout, "mean of absolute values: %g\n", mean_abs);
		fprintf(stdout, "population standard deviation: %g\n",
			pop_stdev);
		fprintf(stdout, "population variance: %g\n", pop_variance);
		fprintf(stdout, "population coefficient of variation: %g\n",
			pop_coeff_variation);
		if (otype & GV_POINTS) {
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
    if (extended->answer && compatible && (otype & GV_POINTS) && count > 0) {
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

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
