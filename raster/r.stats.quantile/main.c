
/****************************************************************************
 *
 * MODULE:       r.stats.quantile
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Markus Metz: dynamic bins to reduce memory consumptions
 * PURPOSE:      Compute category or object oriented quantiles using two passes
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

struct bin
{
    unsigned long origin;
    int base, count;
};

struct basecat
{
    unsigned int *slots;
    unsigned long total;
    int num_values;
    DCELL min, max, slot_size;
    int num_slots;
    unsigned char *slot_bins;
    int num_bins;
    struct bin *bins;
    DCELL *values;
    DCELL *quants;
};

static int num_quants;
static DCELL *quants;
static DCELL min, max;
static int num_slots;

static int rows, cols;

static CELL cmin, cmax;
static int num_cats;
static struct basecat *basecats;

static inline int get_slot(struct basecat *bc, DCELL c)
{
    int i;

    if (bc->num_slots == 0)
	return -1;

    i = (int)floor((c - bc->min) / bc->slot_size);

    if (i < 0)
	i = 0;
    if (i > bc->num_slots - 1)
	i = bc->num_slots - 1;
    return i;
}

static inline double get_quantile(struct basecat *bc, int n)
{
    if (n >= num_quants)
	return (double)bc->total + bc->total;

    return (double)bc->total * quants[n];
}

static void get_slot_counts(int basefile, int coverfile)
{
    CELL *basebuf = Rast_allocate_c_buf();
    DCELL *coverbuf = Rast_allocate_d_buf();
    struct basecat *bc;
    int row, col;
    int i;
    int allnull;

    G_message(_("Computing histograms"));

    for (i = 0; i < num_cats; i++) {
	bc = &basecats[i];
	bc->min = max;
	bc->max = min;
    }

    allnull = 1;
    for (row = 0; row < rows; row++) {
	G_percent(row, rows, 2);

	Rast_get_c_row(basefile, basebuf, row);
	Rast_get_d_row(coverfile, coverbuf, row);

	for (col = 0; col < cols; col++) {
	    if (Rast_is_c_null_value(&basebuf[col]))
		continue;

	    if (Rast_is_d_null_value(&coverbuf[col]))
		continue;

	    allnull = 0;

	    bc = &basecats[basebuf[col] - cmin];

	    bc->total++;

	    if (bc->min > coverbuf[col])
		bc->min = coverbuf[col];
	    if (bc->max < coverbuf[col])
		bc->max = coverbuf[col];
	}
    }
    G_percent(rows, rows, 2);

    if (allnull)
	G_fatal_error(_("No cells found where both base and cover are not NULL"));

    for (i = 0; i < num_cats; i++) {
	bc = &basecats[i];

	bc->num_slots = 0;
	bc->slot_size = 0;

	if (bc->max <= bc->min)
	    continue;

	bc->num_slots = 1;
	if (bc->total * 10 > (unsigned long) num_slots)
	    bc->num_slots = num_slots;

	bc->slots = G_calloc(bc->num_slots, sizeof(unsigned int));
	bc->slot_size = (bc->max - bc->min) / bc->num_slots;
    }

    for (row = 0; row < rows; row++) {
	G_percent(row, rows, 2);

	Rast_get_c_row(basefile, basebuf, row);
	Rast_get_d_row(coverfile, coverbuf, row);

	for (col = 0; col < cols; col++) {
	    if (Rast_is_c_null_value(&basebuf[col]))
		continue;

	    if (Rast_is_d_null_value(&coverbuf[col]))
		continue;

	    bc = &basecats[basebuf[col] - cmin];

	    if (bc->num_slots == 0)
		continue;

	    i = get_slot(bc, coverbuf[col]);
	    bc->slots[i]++;
	}
    }
    G_percent(rows, rows, 2);

    G_free(basebuf);
    G_free(coverbuf);
}

static void initialize_bins(void)
{
    int cat;

    G_message(_("Computing bins"));

    for (cat = 0; cat < num_cats; cat++) {
	struct basecat *bc = &basecats[cat];
	int slot;
	double next;
	int num_values = 0;
	int bin = 0;
	unsigned long accum = 0;
	int quant = 0;

	if (bc->num_slots == 0)
	    continue;

	bc->bins = G_calloc(num_quants, sizeof(struct bin));
	bc->slot_bins = G_calloc(bc->num_slots, sizeof(unsigned char));

	next = get_quantile(bc, quant);

	for (slot = 0; slot < bc->num_slots; slot++) {
	    unsigned int count = bc->slots[slot];
	    unsigned long accum2 = accum + count;

	    if (accum2 > next ||
	        (slot == bc->num_slots - 1 && accum2 == next)) {
		struct bin *b = &bc->bins[bin];

		bc->slot_bins[slot] = ++bin;

		b->origin = accum;
		b->base = num_values;
		b->count = 0;

		while (accum2 > next)
		    next = get_quantile(bc, ++quant);

		num_values += count;
	    }

	    accum = accum2;
	}

	bc->num_values = num_values;
	bc->num_bins = bin;

	G_free(bc->slots);

	bc->values = G_calloc(num_values, sizeof(DCELL));
    }
}

static void fill_bins(int basefile, int coverfile)
{
    CELL *basebuf = Rast_allocate_c_buf();
    DCELL *coverbuf = Rast_allocate_d_buf();
    int row, col;

    G_message(_("Binning data"));

    for (row = 0; row < rows; row++) {
	Rast_get_c_row(basefile, basebuf, row);
	Rast_get_d_row(coverfile, coverbuf, row);

	for (col = 0; col < cols; col++) {
	    struct basecat *bc;
	    int i, bin;
	    struct bin *b;

	    if (Rast_is_c_null_value(&basebuf[col]))
		continue;

	    if (Rast_is_d_null_value(&coverbuf[col]))
		continue;

	    bc = &basecats[basebuf[col] - cmin];

	    if (bc->num_slots == 0)
		continue;

	    i = get_slot(bc, coverbuf[col]);
	    if (!bc->slot_bins[i])
		continue;

	    bin = bc->slot_bins[i] - 1;
	    b = &bc->bins[bin];

	    bc->values[b->base + b->count++] = coverbuf[col];
	}

	G_percent(row, rows, 2);
    }
    G_percent(rows, rows, 2);

    G_free(basebuf);
    G_free(coverbuf);
}

static int compare_dcell(const void *aa, const void *bb)
{
    DCELL a = *(const DCELL *)aa;
    DCELL b = *(const DCELL *)bb;

    if (a < b)
	return -1;
    if (a > b)
	return 1;
    return 0;
}

static void sort_bins(void)
{
    int cat;

    G_message(_("Sorting bins"));

    for (cat = 0; cat < num_cats; cat++) {
	struct basecat *bc = &basecats[cat];
	int bin;

	if (bc->num_slots == 0)
	    continue;

	G_free(bc->slot_bins);

	for (bin = 0; bin < bc->num_bins; bin++) {
	    struct bin *b = &bc->bins[bin];

	    qsort(&bc->values[b->base], b->count, sizeof(DCELL), compare_dcell);
	}

	G_percent(cat, num_cats, 2);
    }

    G_percent(cat, num_cats, 2);
}

static void print_quantiles(char *fs, char *name, int table_frmt)
{
    int cat, quant;
    struct basecat *bc;

    G_message(_("Printing quantiles"));

    if (name != NULL && strcmp(name, "-") != 0) {
	if (NULL == freopen(name, "w", stdout)) {
	    G_fatal_error(_("Unable to open file <%s> for writing"), name);
	}
    }

    if (!table_frmt) {
	for (cat = 0; cat < num_cats; cat++) {
	    bc = &basecats[cat];

	    if (bc->total == 0)
		continue;

	    for (quant = 0; quant < num_quants; quant++)
		fprintf(stdout, "%d%s%d%s%f%s%f\n", cmin + cat, fs, quant, fs,
		       100 * quants[quant], fs, bc->quants[quant]);
	}
    }
    else {
	fprintf(stdout, "cat");
	for (quant = 0; quant < num_quants; quant++)
	    fprintf(stdout, "%s%f", fs, 100 * quants[quant]);
	fprintf(stdout, "\n");

	for (cat = 0; cat < num_cats; cat++) {
	    bc = &basecats[cat];

	    if (bc->total == 0)
		continue;

	    fprintf(stdout, "%d", cmin + cat);
	    for (quant = 0; quant < num_quants; quant++)
		fprintf(stdout, "%s%f", fs, bc->quants[quant]);
	    fprintf(stdout, "\n");
	}
    }

}

static void compute_quantiles(void)
{
    int cat;

    G_message(_("Computing quantiles"));

    for (cat = 0; cat < num_cats; cat++) {
	struct basecat *bc = &basecats[cat];
	int quant;

	if (bc->max < bc->min)
	    continue;

	bc->quants = G_malloc(num_quants * sizeof(DCELL));

	if (bc->max == bc->min) {
	    for (quant = 0; quant < num_quants; quant++)
		bc->quants[quant] = bc->min;
	}
	else {
	    struct bin *b = &bc->bins[0];

	    for (quant = 0; quant < num_quants; quant++) {
		double next = get_quantile(bc, quant);
		double k, v;
		int i0, i1;

		while (b->origin + b->count < next)
		    b++;

		k = next - b->origin;
		i0 = (int)floor(k);
		i1 = (int)ceil(k);

		if (i0 > b->count - 1)
		    i0 = b->count - 1;
		if (i1 > b->count - 1)
		    i1 = b->count - 1;

		v = (i0 == i1)
		    ? bc->values[b->base + i0]
		    : bc->values[b->base + i0] * (i1 - k) + bc->values[b->base + i1] * (k - i0);

		bc->quants[quant] = v;
	    }
	}
    }
}

static void do_reclass(const char *basemap, char **outputs)
{
    const char *tempfile = G_tempfile();
    char *input_arg = G_malloc(strlen(basemap) + 7);
    char *rules_arg = G_malloc(strlen(tempfile) + 7);
    int quant;

    G_message(_("Generating reclass maps"));

    sprintf(input_arg, "input=%s", basemap);
    sprintf(rules_arg, "rules=%s", tempfile);

    for (quant = 0; quant < num_quants; quant++) {
	const char *output = outputs[quant];
	char *output_arg = G_malloc(strlen(output) + 8);
	FILE *fp;
	int cat;

	sprintf(output_arg, "output=%s", output);

	fp = fopen(tempfile, "w");
	if (!fp)
	    G_fatal_error(_("Unable to open temporary file"));

	for (cat = 0; cat < num_cats; cat++) {
	    if (basecats[cat].total > 0)
		fprintf(fp, "%d = %d %f\n", cmin + cat, cmin + cat, basecats[cat].quants[quant]);
	}

	fclose(fp);

	G_spawn("r.reclass", "r.reclass", input_arg, output_arg, rules_arg, NULL);
    }

    remove(tempfile);
}

static void do_output(int base_fd, char **outputs, const char *covermap)
{
    int *out_fd = G_malloc(num_quants * sizeof(int));
    CELL *base_buf = Rast_allocate_c_buf();
    DCELL *out_buf = Rast_allocate_d_buf();
    const char *mapset = G_mapset();
    struct Colors colors;
    struct History history;
    int have_colors;
    int quant;
    int row, col;

    G_message(_("Writing output maps"));

    for (quant = 0; quant < num_quants; quant++) {
	const char *output = outputs[quant];

	out_fd[quant] = Rast_open_fp_new(output);
    }

    have_colors = Rast_read_colors(covermap, "", &colors) > 0;

    for (row = 0; row < rows; row++) {
	Rast_get_c_row(base_fd, base_buf, row);

	for (quant = 0; quant < num_quants; quant++) {
	    for (col = 0; col < cols; col++)
		if (Rast_is_c_null_value(&base_buf[col]))
		    Rast_set_d_null_value(&out_buf[col], 1);
		else if (basecats[base_buf[col] - cmin].total == 0)
		    Rast_set_d_null_value(&out_buf[col], 1);
		else
		    out_buf[col] = basecats[base_buf[col] - cmin].quants[quant];

	    Rast_put_d_row(out_fd[quant], out_buf);
	}

	G_percent(row, rows, 2);
    }

    G_percent(row, rows, 2);

    for (quant = 0; quant < num_quants; quant++) {
	Rast_close(out_fd[quant]);
	Rast_short_history(outputs[quant], "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(outputs[quant], &history);
	if (have_colors)
	    Rast_write_colors(outputs[quant], mapset, &colors);
    }
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *quant, *perc, *slots, *basemap, *covermap,
	              *output, *file, *fs;
    } opt;
    struct {
	struct Flag *r, *p, *t;
    } flag;
    const char *basemap, *covermap;
    char **outputs, *fs;
    int reclass, print;
    int cover_fd, base_fd;
    struct Range range;
    struct FPRange fprange;
    int i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("zonal statistics"));
    G_add_keyword(_("percentile"));
    G_add_keyword(_("quantile"));
    module->description = _("Compute category quantiles using two passes.");

    opt.basemap = G_define_standard_option(G_OPT_R_BASE);

    opt.covermap = G_define_standard_option(G_OPT_R_COVER);

    opt.quant = G_define_option();
    opt.quant->key = "quantiles";
    opt.quant->type = TYPE_INTEGER;
    opt.quant->required = NO;
    opt.quant->description = _("Number of quantiles");

    opt.perc = G_define_option();
    opt.perc->key = "percentiles";
    opt.perc->type = TYPE_DOUBLE;
    opt.perc->multiple = YES;
    opt.perc->description = _("List of percentiles");
    opt.perc->answer = "50";

    opt.slots = G_define_option();
    opt.slots->key = "bins";
    opt.slots->type = TYPE_INTEGER;
    opt.slots->required = NO;
    opt.slots->description = _("Number of bins to use");
    opt.slots->answer = "1000";

    opt.output = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.output->description = _("Resultant raster map(s)");
    opt.output->required = NO;
    opt.output->multiple = YES;

    opt.file = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.file->key = "file";
    opt.file->required = NO;
    opt.file->description =
	_("Name for output file (if omitted or \"-\" output to stdout)");

    opt.fs = G_define_standard_option(G_OPT_F_SEP);
    opt.fs->answer = ":";
    opt.fs->guisection = _("Formatting");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description =
	_("Create reclass map with statistics as category labels");

    flag.p = G_define_flag();
    flag.p->key = 'p';
    flag.p->description =
	_("Do not create output maps; just print statistics");

    flag.t = G_define_flag();
    flag.t->key = 't';
    flag.t->description =
	_("Print statistics in table format");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    basemap = opt.basemap->answer;
    covermap = opt.covermap->answer;
    outputs = opt.output->answers;
    reclass = flag.r->answer;
    print = flag.p->answer || flag.t->answer;

    if (!print && !opt.output->answers)
	G_fatal_error(_("Either -%c or %s= must be given"),
			flag.p->key, opt.output->key);

    if (print && opt.output->answers)
	G_fatal_error(_("-%c and %s= are mutually exclusive"),
			flag.p->key, opt.output->key);

    num_slots = atoi(opt.slots->answer);

    if (opt.quant->answer) {
	num_quants = atoi(opt.quant->answer) - 1;
	quants = G_calloc(num_quants, sizeof(DCELL));
	for (i = 0; i < num_quants; i++)
	    quants[i] = 1.0 * (i + 1) / (num_quants + 1);
    }
    else {
	for (i = 0; opt.perc->answers[i]; i++)
	    ;
	num_quants = i;
	quants = G_calloc(num_quants, sizeof(DCELL));
	for (i = 0; i < num_quants; i++)
	    quants[i] = atof(opt.perc->answers[i]) / 100;
	qsort(quants, num_quants, sizeof(DCELL), compare_dcell);
    }

    if (opt.output->answer) {
	for (i = 0; opt.output->answers[i]; i++)
	    ;
	if (i != num_quants)
	    G_fatal_error(_("Number of quantiles (%d) does not match number of output maps (%d)"),
			  num_quants, i);
    }

    base_fd = Rast_open_old(basemap, "");

    cover_fd = Rast_open_old(covermap, "");

    if (Rast_map_is_fp(basemap, "") != 0)
	G_fatal_error(_("The base map must be an integer (CELL) map"));

    if (Rast_read_range(basemap, "", &range) < 0)
	G_fatal_error(_("Unable to read range of base map <%s>"), basemap);

    Rast_get_range_min_max(&range, &cmin, &cmax);
    num_cats = cmax - cmin + 1;
    if (num_cats > 100000)
	G_warning(_("Base map <%s> has many categories (%d), computation might be slow and might need a lot of memory"),
		  basemap, num_cats);

    Rast_read_fp_range(covermap, "", &fprange);
    Rast_get_fp_range_min_max(&fprange, &min, &max);

    basecats = G_calloc(num_cats, sizeof(struct basecat));
    rows = Rast_window_rows();
    cols = Rast_window_cols();

    get_slot_counts(base_fd, cover_fd);
    initialize_bins();
    fill_bins(base_fd, cover_fd);
    sort_bins();
    compute_quantiles();

    if (print) {
	/* get field separator */
	fs = G_option_to_separator(opt.fs);
	
	print_quantiles(fs, opt.file->answer, flag.t->answer);
    }
    else if (reclass)
	do_reclass(basemap, outputs);
    else
	do_output(base_fd, outputs, covermap);

    Rast_close(cover_fd);
    Rast_close(base_fd);

    return (EXIT_SUCCESS);
}
