
/****************************************************************************
 *
 * MODULE:       r.statistics2
 *               
 * AUTHOR(S):    Glynn Clements; loosely based upon r.statistics, by
 *               Martin Schroeder, Geographisches Institut Heidelberg, Germany
 *
 * PURPOSE:      Category or object oriented statistics
 *
 * COPYRIGHT:    (C) 2007,2008 Martin Schroeder, Glynn Clements
 *                   and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/spawn.h>
#include <grass/glocale.h>

#define COUNT	   1		/* Count                 */
#define SUM	   2		/* Sum                   */
#define MIN	   3		/* Minimum               */
#define MAX	   4		/* Maximum               */
#define RANGE	   5		/* Range                 */
#define AVERAGE    6		/* Average (mean)        */
#define ADEV       7		/* Average deviation     */
#define VARIANCE1  8		/* Variance              */
#define STDDEV1    9		/* Standard deviation    */
#define SKEWNESS1 10		/* Skewness              */
#define KURTOSIS1 11		/* Kurtosis              */
#define VARIANCE2 12		/* Variance              */
#define STDDEV2   13		/* Standard deviation    */
#define SKEWNESS2 14		/* Skewness              */
#define KURTOSIS2 15		/* Kurtosis              */

struct menu
{
    const char *name;		/* method name */
    int val;			/* number of function */
    const char *text;		/* menu display - full description */
};

extern struct menu menu[];

/* modify this table to add new methods */
struct menu menu[] = {
    {"count",     COUNT,     "Count of values in specified objects"},
    {"sum",       SUM,       "Sum of values in specified objects"},
    {"min",       MIN,       "Minimum of values in specified objects"},
    {"max",       MAX,       "Maximum of values in specified objects"},
    {"range",     RANGE,     "Range of values (max - min) in specified objects"},
    {"average",   AVERAGE,   "Average of values in specified objects"},
    {"avedev",    ADEV,      "Average deviation of values in specified objects"},
    {"variance",  VARIANCE1, "Variance of values in specified objects"},
    {"stddev",    STDDEV1,   "Standard deviation of values in specified objects"},
    {"skewness",  SKEWNESS1, "Skewness of values in specified objects"},
    {"kurtosis",  KURTOSIS1, "Kurtosis of values in specified objects"},
    {"variance2", VARIANCE2, "(2-pass) Variance of values in specified objects"},
    {"stddev2",   STDDEV2,   "(2-pass) Standard deviation of values in specified objects"},
    {"skewness2", SKEWNESS2, "(2-pass) Skewness of values in specified objects"},
    {"kurtosis2", KURTOSIS2, "(2-pass) Kurtosis of values in specified objects"},
    {0, 0, 0}
};

int main(int argc, char **argv)
{
    static DCELL *count, *sum, *mean, *sumu, *sum2, *sum3, *sum4, *min, *max;
    DCELL *result;
    struct GModule *module;
    struct {
	struct Option *method, *basemap, *covermap, *output;
    } opt;
    struct {
	struct Flag *c, *r;
    } flag;
    char methods[2048];
    const char *basemap, *covermap, *output;
    int usecats;
    int reclass;
    int base_fd, cover_fd;
    struct Categories cats;
    CELL *base_buf;
    DCELL *cover_buf;
    struct Range range;
    CELL mincat, ncats;
    int method;
    int rows, cols;
    int row, col, i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    module->description =
	_("Calculates category or object oriented statistics (accumulator-based statistics).");

    opt.basemap = G_define_standard_option(G_OPT_R_BASE);

    opt.covermap = G_define_standard_option(G_OPT_R_COVER);

    opt.method = G_define_option();
    opt.method->key = "method";
    opt.method->type = TYPE_STRING;
    opt.method->required = YES;
    opt.method->description = _("Method of object-based statistic");

    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(methods, ",");
	else
	    *(methods) = 0;
	strcat(methods, menu[i].name);
    }
    opt.method->options = G_store(methods);

    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(methods, ";");
	else
	    *(methods) = 0;
	strcat(methods, menu[i].name);
	strcat(methods, ";");
	strcat(methods, menu[i].text);
    }
    opt.method->descriptions = G_store(methods);

    opt.output = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.output->description = _("Resultant raster map");
    opt.output->required = YES;

    flag.c = G_define_flag();
    flag.c->key = 'c';
    flag.c->description =
	_("Cover values extracted from the category labels of the cover map");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description =
	_("Create reclass map with statistics as category labels");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    basemap = opt.basemap->answer;
    covermap = opt.covermap->answer;
    output = opt.output->answer;
    usecats = flag.c->answer;
    reclass = flag.r->answer;

    for (i = 0; menu[i].name; i++)
	if (strcmp(menu[i].name, opt.method->answer) == 0)
	    break;

    if (!menu[i].name) {
	G_warning(_("<%s=%s> unknown %s"), opt.method->key, opt.method->answer,
		  opt.method->key);
	G_usage();
	exit(EXIT_FAILURE);
    }

    method = menu[i].val;

    base_fd = Rast_open_old(basemap, "");

    cover_fd = Rast_open_old(covermap, "");

    if (usecats && Rast_read_cats(covermap, "", &cats) < 0)
	G_fatal_error(_("Unable to read category file of cover map <%s>"), covermap);

    if (Rast_map_is_fp(basemap, "") != 0)
	G_fatal_error(_("The base map must be an integer (CELL) map"));

    if (Rast_read_range(basemap, "", &range) < 0)
	G_fatal_error(_("Unable to read range of base map <%s>"), basemap);

    mincat = range.min;
    ncats = range.max - range.min + 1;

    rows = Rast_window_rows();
    cols = Rast_window_cols();

    switch (method) {
    case COUNT:
	count = G_calloc(ncats, sizeof(DCELL));
	break;
    case SUM:
	sum = G_calloc(ncats, sizeof(DCELL));
	break;
    case MIN:
	min = G_malloc(ncats * sizeof(DCELL));
	break;
    case MAX:
	max = G_malloc(ncats * sizeof(DCELL));
	break;
    case RANGE:
	min = G_malloc(ncats * sizeof(DCELL));
	max = G_malloc(ncats * sizeof(DCELL));
	break;
    case AVERAGE:
    case ADEV:
    case VARIANCE2:
    case STDDEV2:
    case SKEWNESS2:
    case KURTOSIS2:
	count = G_calloc(ncats, sizeof(DCELL));
	sum = G_calloc(ncats, sizeof(DCELL));
	break;
    case VARIANCE1:
    case STDDEV1:
	count = G_calloc(ncats, sizeof(DCELL));
	sum = G_calloc(ncats, sizeof(DCELL));
	sum2 = G_calloc(ncats, sizeof(DCELL));
	break;
    case SKEWNESS1:
	count = G_calloc(ncats, sizeof(DCELL));
	sum = G_calloc(ncats, sizeof(DCELL));
	sum2 = G_calloc(ncats, sizeof(DCELL));
	sum3 = G_calloc(ncats, sizeof(DCELL));
	break;
    case KURTOSIS1:
	count = G_calloc(ncats, sizeof(DCELL));
	sum = G_calloc(ncats, sizeof(DCELL));
	sum2 = G_calloc(ncats, sizeof(DCELL));
	sum4 = G_calloc(ncats, sizeof(DCELL));
	break;
    }

    if (min)
	for (i = 0; i < ncats; i++)
	    min[i] = 1e300;
    if (max)
	for (i = 0; i < ncats; i++)
	    max[i] = -1e300;

    base_buf = Rast_allocate_c_buf();
    cover_buf = Rast_allocate_d_buf();

    G_message(_("First pass"));

    for (row = 0; row < rows; row++) {
	Rast_get_c_row(base_fd, base_buf, row);
	Rast_get_d_row(cover_fd, cover_buf, row);

	for (col = 0; col < cols; col++) {
	    int n;
	    DCELL v;

	    if (Rast_is_c_null_value(&base_buf[col]))
		continue;
	    if (Rast_is_d_null_value(&cover_buf[col]))
		continue;

	    n = base_buf[col] - mincat;

	    if (n < 0 || n >= ncats)
		continue;

	    v = cover_buf[col];
	    if (usecats)
		sscanf(Rast_get_c_cat((CELL *) &v, &cats), "%lf", &v);

	    if (count)
		count[n]++;
	    if (sum)
		sum[n] += v;
	    if (sum2)
		sum2[n] += v * v;
	    if (sum3)
		sum3[n] += v * v * v;
	    if (sum4)
		sum4[n] += v * v * v * v;
	    if (min && min[n] > v)
		min[n] = v;
	    if (max && max[n] < v)
		max[n] = v;
	}

	G_percent(row, rows, 2);
    }

    G_percent(row, rows, 2);

    result = G_calloc(ncats, sizeof(DCELL));

    switch (method) {
    case ADEV:
    case VARIANCE2:
    case STDDEV2:
    case SKEWNESS2:
    case KURTOSIS2:
	mean = G_calloc(ncats, sizeof(DCELL));
	for (i = 0; i < ncats; i++)
	    mean[i] = sum[i] / count[i];
	G_free(sum);
	break;
    }

    switch (method) {
    case ADEV:
	sumu = G_calloc(ncats, sizeof(DCELL));
	break;
    case VARIANCE2:
    case STDDEV2:
	sum2 = G_calloc(ncats, sizeof(DCELL));
	break;
    case SKEWNESS2:
	sum2 = G_calloc(ncats, sizeof(DCELL));
	sum3 = G_calloc(ncats, sizeof(DCELL));
	break;
    case KURTOSIS2:
	sum2 = G_calloc(ncats, sizeof(DCELL));
	sum4 = G_calloc(ncats, sizeof(DCELL));
	break;
    }

    if (mean) {
	G_message(_("Second pass"));

	for (row = 0; row < rows; row++) {
	    Rast_get_c_row(base_fd, base_buf, row);
	    Rast_get_d_row(cover_fd, cover_buf, row);

	    for (col = 0; col < cols; col++) {
		int n;
		DCELL v, d;

		if (Rast_is_c_null_value(&base_buf[col]))
		    continue;
		if (Rast_is_d_null_value(&cover_buf[col]))
		    continue;

		n = base_buf[col] - mincat;

		if (n < 0 || n >= ncats)
		    continue;

		v = cover_buf[col];
		if (usecats)
		    sscanf(Rast_get_c_cat((CELL *) &v, &cats), "%lf", &v);
		d = v - mean[n];

		if (sumu)
		    sumu[n] += fabs(d);
		if (sum2)
		    sum2[n] += d * d;
		if (sum3)
		    sum3[n] += d * d * d;
		if (sum4)
		    sum4[n] += d * d * d * d;
	    }

	    G_percent(row, rows, 2);
	}

	G_percent(row, rows, 2);
	G_free(mean);
	G_free(cover_buf);
    }

    switch (method) {
    case COUNT:
	for (i = 0; i < ncats; i++)
	    result[i] = count[i];
	break;
    case SUM:
	for (i = 0; i < ncats; i++)
	    result[i] = sum[i];
	break;
    case AVERAGE:
	for (i = 0; i < ncats; i++)
	    result[i] = sum[i] / count[i];
	break;
    case MIN:
	for (i = 0; i < ncats; i++)
	    result[i] = min[i];
	break;
    case MAX:
	for (i = 0; i < ncats; i++)
	    result[i] = max[i];
	break;
    case RANGE:
	for (i = 0; i < ncats; i++)
	    result[i] = max[i] - min[i];
    case VARIANCE1:
	for (i = 0; i < ncats; i++) {
	    double n = count[i];
	    double var = (sum2[i] - sum[i] * sum[i] / n) / (n - 1);
	    result[i] = var;
	}
	break;
    case STDDEV1:
	for (i = 0; i < ncats; i++) {
	    double n = count[i];
	    double var = (sum2[i] - sum[i] * sum[i] / n) / (n - 1);
	    result[i] = sqrt(var);
	}
	break;
    case SKEWNESS1:
	for (i = 0; i < ncats; i++) {
	    double n = count[i];
	    double var = (sum2[i] - sum[i] * sum[i] / n) / (n - 1);
	    double skew = (sum3[i] / n
			   - 3 * sum[i] * sum2[i] / (n * n)
			   + 2 * sum[i] * sum[i] * sum[i] / (n * n * n))
		/ (pow(var, 1.5));
	    result[i] = skew;
	}
	break;
    case KURTOSIS1:
	for (i = 0; i < ncats; i++) {
	    double n = count[i];
	    double var = (sum2[i] - sum[i] * sum[i] / n) / (n - 1);
	    double kurt = (sum4[i] / n
			   - 4 * sum[i] * sum3[i] / (n * n)
			   + 6 * sum[i] * sum[i] * sum2[i] / (n * n * n)
			   - 3 * sum[i] * sum[i] * sum[i] * sum[i] / (n * n * n * n))
		/ (var * var) - 3;
	    result[i] = kurt;
	}
	break;
    case ADEV:
	for (i = 0; i < ncats; i++)
	    result[i] = sumu[i] / count[i];
	break;
    case VARIANCE2:
	for (i = 0; i < ncats; i++)
	    result[i] = sum2[i] / (count[i] - 1);
	break;
    case STDDEV2:
	for (i = 0; i < ncats; i++)
	    result[i] = sqrt(sum2[i] / (count[i] - 1));
	break;
    case SKEWNESS2:
	for (i = 0; i < ncats; i++) {
	    double n = count[i];
	    double var = sum2[i] / (n - 1);
	    double sdev = sqrt(var);
	    result[i] = sum3[i] / (sdev * sdev * sdev) / n;
	}
	G_free(count);
	G_free(sum2);
	G_free(sum3);
	break;
    case KURTOSIS2:
	for (i = 0; i < ncats; i++) {
	    double n = count[i];
	    double var = sum2[i] / (n - 1);
	    result[i] = sum4[i] / (var * var) / n - 3;
	}
	G_free(count);
	G_free(sum2);
	G_free(sum4);
	break;
    }

    if (reclass) {
	const char *tempfile = G_tempfile();
	char *input_arg = G_malloc(strlen(basemap) + 7);
	char *output_arg = G_malloc(strlen(output) + 8);
	char *rules_arg = G_malloc(strlen(tempfile) + 7);
	FILE *fp;

	G_message(_("Generating reclass map"));

	sprintf(input_arg, "input=%s", basemap);
	sprintf(output_arg, "output=%s", output);
	sprintf(rules_arg, "rules=%s", tempfile);

	fp = fopen(tempfile, "w");
	if (!fp)
	    G_fatal_error(_("Unable to open temporary file"));

	for (i = 0; i < ncats; i++)
	    fprintf(fp, "%d = %d %f\n", mincat + i, mincat + i, result[i]);

	fclose(fp);

	G_spawn("r.reclass", "r.reclass", input_arg, output_arg, rules_arg, NULL);
    }
    else {
	int out_fd;
	DCELL *out_buf;
	struct Colors colors;

	G_message(_("Writing output map"));

	out_fd = Rast_open_fp_new(output);

	out_buf = Rast_allocate_d_buf();

	for (row = 0; row < rows; row++) {
	    Rast_get_c_row(base_fd, base_buf, row);

	    for (col = 0; col < cols; col++)
		if (Rast_is_c_null_value(&base_buf[col]))
		    Rast_set_d_null_value(&out_buf[col], 1);
		else
		    out_buf[col] = result[base_buf[col] - mincat];

	    Rast_put_d_row(out_fd, out_buf);

	    G_percent(row, rows, 2);
	}

	G_percent(row, rows, 2);

	Rast_close(out_fd);

	if (Rast_read_colors(covermap, "", &colors) > 0)
	    Rast_write_colors(output, G_mapset(), &colors);
    }

    return 0;
}

