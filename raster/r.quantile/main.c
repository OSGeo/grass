
/****************************************************************************
 *
 * MODULE:       r.quantile
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor),
 * PURPOSE:      Compute quantiles using two passes
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/* TODO: replace long with either size_t or a guaranteed 64 bit integer */
struct bin
{
    size_t origin;
    DCELL min, max;
    size_t base, count;
};

static int rows, cols;

static DCELL min, max;
static int num_quants;
static DCELL *quants;
static int num_slots;
static size_t *slots;
static DCELL slot_size;
/* total should be a 64bit integer */
static unsigned long total;
static size_t num_values;
static unsigned short *slot_bins;
static int num_bins;
static struct bin *bins;
static DCELL *values;

static inline int get_slot(DCELL c)
{
    int i = (int)floor((c - min) / slot_size);

    if (i < 0)
	i = 0;
    if (i > num_slots - 1)
	i = num_slots - 1;
    return i;
}

static inline double get_quantile(int n)
{
    if (n >= num_quants)
	return (double)total + total;

    return (double)total * quants[n];
}

static void get_slot_counts(int infile)
{
    DCELL *inbuf = Rast_allocate_d_buf();
    int row, col;

    G_message(_("Computing histogram"));

    total = 0;

    for (row = 0; row < rows; row++) {
	Rast_get_d_row(infile, inbuf, row);

	for (col = 0; col < cols; col++) {
	    int i;

	    if (Rast_is_d_null_value(&inbuf[col]))
		continue;

	    i = get_slot(inbuf[col]);

	    slots[i]++;
	    total++;
	}

	G_percent(row, rows, 2);
    }

    G_percent(rows, rows, 2);
    G_free(inbuf);
}

static void initialize_bins(void)
{
    int slot;
    double next;
    int bin = 0;
    size_t accum = 0;
    int quant = 0;

    G_message(_("Computing bins"));

    num_values = 0;
    next = get_quantile(quant);

    for (slot = 0; slot < num_slots; slot++) {
	size_t count = slots[slot];
	size_t accum2 = accum + count;

	if (accum2 > next ||
	    (slot == num_slots - 1 && accum2 == next)) {
	    struct bin *b = &bins[bin];

	    slot_bins[slot] = ++bin;

	    b->origin = accum;
	    b->base = num_values;
	    b->count = 0;
	    b->min = min + slot_size * slot;
	    b->max = min + slot_size * (slot + 1);

	    while (accum2 > next)
		next = get_quantile(++quant);

	    num_values += count;
	}

	accum = accum2;
    }

    num_bins = bin;

    G_debug(1, "Number of bins: %d", num_bins);
    G_debug(1, "Number of values: %lu", num_values);
}

static void fill_bins(int infile)
{
    DCELL *inbuf = Rast_allocate_d_buf();
    int row, col;

    G_message(_("Binning data"));

    for (row = 0; row < rows; row++) {
	Rast_get_d_row(infile, inbuf, row);

	for (col = 0; col < cols; col++) {
	    int i, bin;
	    struct bin *b;

	    if (Rast_is_d_null_value(&inbuf[col]))
		continue;

	    i = get_slot(inbuf[col]);
	    if (!slot_bins[i])
		continue;

	    bin = slot_bins[i] - 1;
	    b = &bins[bin];

	    values[b->base + b->count++] = inbuf[col];
	}

	G_percent(row, rows, 2);
    }

    G_percent(rows, rows, 2);
    G_free(inbuf);
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
    int bin;

    G_message(_("Sorting bins"));

    for (bin = 0; bin < num_bins; bin++) {
	struct bin *b = &bins[bin];

	qsort(&values[b->base], b->count, sizeof(DCELL), compare_dcell);
    }
}

static void compute_quantiles(int recode)
{
    int bin = 0;
    double prev_v = min;
    int quant;

    G_message(_("Computing quantiles"));

    for (quant = 0; quant < num_quants; quant++) {
	struct bin *b = &bins[bin];
	double next = get_quantile(quant);
	double k, v;
	size_t i0, i1;

	for (; bin < num_bins; bin++) {
	    b = &bins[bin];
	    if (b->origin + b->count >= next)
		break;
	}

	if (bin < num_bins) {
	    k = next - b->origin;
	    i0 = (size_t)floor(k);
	    i1 = (size_t)ceil(k);

	    if (i0 > b->count - 1)
		i0 = b->count - 1;
	    if (i1 > b->count - 1)
		i1 = b->count - 1;

	    v = (i0 == i1)
		? values[b->base + i0]
		: values[b->base + i0] * (i1 - k) +
		  values[b->base + i1] * (k - i0);
	}
	else
	    v = max;

	if (recode)
	    fprintf(stdout, "%f:%f:%i\n", prev_v, v, quant + 1);
	else
	    fprintf(stdout, "%d:%f:%f\n", quant, 100 * quants[quant], v);

	prev_v = v;
    }

    if (recode)
	printf("%f:%f:%i\n", prev_v, max, num_quants + 1);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *input, *quant, *perc, *slots, *file;
    } opt;
    struct {
	struct Flag *r;
    } flag;
    int recode;
    int infile;
    struct FPRange range;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("percentile"));
    G_add_keyword(_("quantile"));
    module->description = _("Compute quantiles using two passes.");

    opt.input = G_define_standard_option(G_OPT_R_INPUT);

    opt.quant = G_define_option();
    opt.quant->key = "quantiles";
    opt.quant->type = TYPE_INTEGER;
    opt.quant->required = NO;
    opt.quant->description = _("Number of quantiles");
    opt.quant->answer = "4";

    opt.perc = G_define_option();
    opt.perc->key = "percentiles";
    opt.perc->type = TYPE_DOUBLE;
    opt.perc->required = NO;
    opt.perc->multiple = YES;
    opt.perc->description = _("List of percentiles");

    opt.slots = G_define_option();
    opt.slots->key = "bins";
    opt.slots->type = TYPE_INTEGER;
    opt.slots->required = NO;
    opt.slots->description = _("Number of bins to use");
    opt.slots->answer = "1000000";

    opt.file = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.file->key = "file";
    opt.file->required = NO;
    opt.file->description =
	_("Name for output file (if omitted or \"-\" output to stdout)");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description = _("Generate recode rules based on quantile-defined intervals");
 
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    num_slots = atoi(opt.slots->answer);
    recode = flag.r->answer;

    if (opt.file->answer != NULL && strcmp(opt.file->answer, "-") != 0) {
	if (NULL == freopen(opt.file->answer, "w", stdout)) {
	    G_fatal_error(_("Unable to open file <%s> for writing"), opt.file->answer);
	}
    }

    if (opt.perc->answer) {
	int i;

	for (i = 0; opt.perc->answers[i]; i++) ;
	num_quants = i;
	quants = G_calloc(num_quants, sizeof(DCELL));
	for (i = 0; i < num_quants; i++)
	    quants[i] = atof(opt.perc->answers[i]) / 100;
	qsort(quants, num_quants, sizeof(DCELL), compare_dcell);
    }
    else {
	int i;

	num_quants = atoi(opt.quant->answer) - 1;
	quants = G_calloc(num_quants, sizeof(DCELL));
	for (i = 0; i < num_quants; i++)
	    quants[i] = 1.0 * (i + 1) / (num_quants + 1);
    }

    if (num_quants > 65535)
	G_fatal_error(_("Too many quantiles"));

    infile = Rast_open_old(opt.input->answer, "");

    Rast_read_fp_range(opt.input->answer, "", &range);
    Rast_get_fp_range_min_max(&range, &min, &max);

    slots = G_calloc(num_slots, sizeof(size_t));
    slot_bins = G_calloc(num_slots, sizeof(unsigned short));

    slot_size = (max - min) / num_slots;

    rows = Rast_window_rows();
    cols = Rast_window_cols();

    get_slot_counts(infile);

    bins = G_calloc(num_quants, sizeof(struct bin));
    initialize_bins();
    G_free(slots);

    values = G_calloc(num_values, sizeof(DCELL));
    fill_bins(infile);

    Rast_close(infile);
    G_free(slot_bins);

    sort_bins();
    compute_quantiles(recode);

    return (EXIT_SUCCESS);
}
