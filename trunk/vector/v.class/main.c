
/***************************************************************
 *
 * MODULE:       v.class
 * 
 * AUTHOR(S):    Moritz Lennert
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Create data classes, mainly for thematic mapping
 *               
 * COPYRIGHT:    (C) 2004-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/arraystats.h>

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *map_opt, *field_opt, *col_opt, *where_opt;
    struct Option *algo_opt, *nbclass_opt;
    struct Flag *shell_flag;
    struct Map_info Map;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray Cvarr;
    int ofield;
    int nrec, ctype, nbclass, nbreaks, *frequencies;
    int ret, i;
    double finfo;
    double *classbreaks, min, max, *data;
    struct GASTATS stats;
    char *desc;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Classifies attribute data, e.g. for thematic mapping");

    map_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    col_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    col_opt->required = YES;
    col_opt->description = _("Column name or expression");

    where_opt = G_define_standard_option(G_OPT_DB_WHERE);

    algo_opt = G_define_option();
    algo_opt->key = "algorithm";
    algo_opt->type = TYPE_STRING;
    algo_opt->required = YES;
    algo_opt->multiple = NO;
    algo_opt->options = "int,std,qua,equ,dis";
    algo_opt->description = _("Algorithm to use for classification");
    desc = NULL;
    G_asprintf(&desc,
	       "int;%s;"
	       "std;%s;"
	       "qua;%s;"
	       "equ;%s",
	       /* "dis;%s" */
	       _("simple intervals"),
	       _("standard deviations"),
	       _("quantiles"),
	       _("equiprobable (normal distribution)"));
	       /* _("discontinuities"));currently disabled because of bugs */
    algo_opt->descriptions = desc;

    nbclass_opt = G_define_option();
    nbclass_opt->key = "nbclasses";
    nbclass_opt->type = TYPE_INTEGER;
    nbclass_opt->required = YES;
    nbclass_opt->multiple = NO;
    nbclass_opt->description = _("Number of classes to define");

    shell_flag = G_define_flag();
    shell_flag->key = 'g';
    shell_flag->description =
	_("Print only class breaks (without min and max)");

    G_gisinit(argv[0]);
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    /* open input vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&Map, map_opt->answer, "", field_opt->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), map_opt->answer);

    ofield = Vect_get_field_number(&Map, field_opt->answer);

    /* Read attributes */
    db_CatValArray_init(&Cvarr);
    Fi = Vect_get_field(&Map, ofield);

    if (Fi == NULL) {
	G_fatal_error(_("Unable to get layer info for vector map"));
    }
    Vect_close(&Map);

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    /* Note: do not check if the column exists in the table because it may be an expression */

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

    ret = db_CatValArray_sort_by_value(&Cvarr);
    if (ret == DB_FAILED)
	G_fatal_error(_("Unable to sort array of values"));


    data = (double *)G_malloc((nrec) * sizeof(double));
    for (i = 0; i < nrec; i++)
	data[i] = 0.0;

    if (ctype == DB_C_TYPE_INT) {
	for (i = 0; i < nrec; i++)
	    data[i] = Cvarr.value[i].val.i;
    }
    else {
	for (i = 0; i < nrec; i++)
	    data[i] = Cvarr.value[i].val.d;
    }



    nbclass = atoi(nbclass_opt->answer);
    nbreaks = nbclass - 1;	/* we need one less classbreaks (min and max exluded) than classes */

    classbreaks = (double *)G_malloc((nbreaks) * sizeof(double));
    for (i = 0; i < nbreaks; i++)
	classbreaks[i] = 0;

    /* Get classbreaks for given algorithm and number of classbreaks.
     * finfo takes any info coming from the classification algorithms
     * equ algorithm can alter number of class breaks */
    finfo =
        AS_class_apply_algorithm(AS_option_to_algorithm(algo_opt),
                                 data, nrec, &nbreaks,
                                 classbreaks);
    

    if (G_strcasecmp(algo_opt->answer, "dis") == 0 && finfo < 3.84148)
	G_warning(_("The discontinuities algorithm indicates that some "
		    "class breaks are not statistically significant at "
		    "alpha=0.05. You are advised to reduce the number of classes."));

    /*output to be piped to other modules ? */
    if (shell_flag->answer) {

	for (i = 0; i < nbreaks - 1; i++)
	    fprintf(stdout, "%f,", classbreaks[i]);
	fprintf(stdout, "%f", classbreaks[nbreaks - 1]);
	fprintf(stdout, "\n");

    }
    else {

	frequencies = (int *)G_malloc((nbreaks + 1) * sizeof(int));
	for (i = 0; i < nbreaks + 1; i++)
	    frequencies[i] = 0;

	ret = AS_class_frequencies(data, nrec, nbreaks, classbreaks, frequencies);
	AS_basic_stats(data, nrec, &stats);

	min = data[0];
	max = data[nrec - 1];

	/* as equ algorithm can modify number of breaks we recalculate number of
	 * classes
	 */
	fprintf(stdout, _("\nClassification of %s into %i classes\n"),
		col_opt->answer, nbreaks + 1);
	fprintf(stdout, _("Using algorithm: *** %s ***\n"), algo_opt->answer);
	fprintf(stdout, _("Mean: %f\tStandard deviation = %f\n"), stats.mean,
		stats.stdev);

	if (G_strcasecmp(algo_opt->answer, "dis") == 0) {
	    fprintf(stdout, _("Lowest chi2 = %f\n"), finfo);
	}
	if (G_strcasecmp(algo_opt->answer, "std") == 0)
	    fprintf(stdout, _("Stdev multiplied by %.4f to define step\n"),
		    finfo);
	fprintf(stdout, "\n");
	fprintf(stdout, _("%15s%15s%15s\n\n"), "From (excl.)", "To (incl.)",
		"Frequency");
	fprintf(stdout, "%15.5f%15.5f%15i\n", min, classbreaks[0],
		frequencies[0]);

	for (i = 1; i < nbreaks; i++) {
	    fprintf(stdout, "%15.5f%15.5f%15i\n",
		    classbreaks[i - 1], classbreaks[i], frequencies[i]);
	}
	fprintf(stdout, "%15.5f%15.5f%15i\n",
		classbreaks[nbreaks - 1], max, frequencies[nbreaks]);

	fprintf(stdout, _("\nNote: Minimum of first class is including\n\n"));
    }
    
    fflush(stdout);
    
    exit(EXIT_SUCCESS);
}
