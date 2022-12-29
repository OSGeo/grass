
/****************************************************************************
 *
 * MODULE:       r.series.interp
 * AUTHOR(S):    Soeren Gebbert <soerengebbert googlemail.com>
 *               Code is based on r.series from Glynn Clements <glynn gclements.plus.com> 
 *
 * PURPOSE:      Interpolate raster maps located (temporal or spatial) in between input raster maps at specific sampling positions
 * COPYRIGHT:    (C) 2011-2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the infile COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/stats.h>

#define LINEAR_INTERPOLATION 1
#define SPLINE_INTERPOLATION 2

struct map_store
{
    const char *name;
    double pos;
    DCELL *buf;
    int fd;
    int has_run;
};

void selection_sort(struct map_store **array, int num); 
static struct map_store *get_parameter_input(const char *type, char **map_names, char **positions, char *file, int *number_of_maps); 
static void linear_interpolation(struct map_store **inp, int num_inputs, struct map_store **outp, int num_outputs);
static void interpolate_row_linear(struct map_store *left, struct map_store *right, struct map_store *out, int ncols);
static void start_interpolation(struct map_store *inputs, int num_inputs, struct map_store *outputs, int num_outputs, int interpol_method); 

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *input, *datapos, *infile, *output, *samplingpos, *outfile, *method;
    } parm;
    int num_outputs;
    int num_inputs;
    struct map_store *inputs = NULL;
    struct map_store *outputs = NULL;
    int interpol_method = LINEAR_INTERPOLATION;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("series"));
    G_add_keyword(_("interpolation"));
    /* TODO: re-phrase the description */
    module->description =
	_("Interpolates raster maps located (temporal or spatial) "
          "in between input raster maps at specific sampling positions.");

    parm.input = G_define_standard_option(G_OPT_R_INPUTS);
    parm.input->required = NO;
 
    parm.datapos = G_define_option();
    parm.datapos->key = "datapos";
    parm.datapos->type = TYPE_DOUBLE;
    parm.datapos->required = NO;
    parm.datapos->description = _("Data point position for each input map");
    parm.datapos->multiple = YES;

    parm.infile = G_define_standard_option(G_OPT_F_INPUT);
    parm.infile->key = "infile";
    parm.infile->description = _("Input file with one input raster map name and data point position per line,"
                               " field separator between name and sample point is |");
    parm.infile->required = NO;

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->multiple = YES;
    parm.output->required = NO;

    parm.samplingpos = G_define_option();
    parm.samplingpos->key = "samplingpos";
    parm.samplingpos->type = TYPE_DOUBLE;
    parm.samplingpos->required = NO;
    parm.samplingpos->multiple = YES;
    parm.samplingpos->description = _("Sampling point position for each output map");

    parm.outfile = G_define_standard_option(G_OPT_F_INPUT);
    parm.outfile->key = "outfile";
    parm.outfile->description = _("Input file with one output raster map name and sample point position per line,"
                             " field separator between name and sample point is |");
    parm.outfile->required = NO;

    parm.method = G_define_option();
    parm.method->key = "method";
    parm.method->type = TYPE_STRING;
    parm.method->required = NO;
    parm.method->options = "linear";
    parm.method->answer = "linear";
    parm.method->description = _("Interpolation method, currently only linear interpolation is supported");
    parm.method->multiple = NO;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (parm.output->answer && parm.outfile->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
			parm.output->key, parm.outfile->key);
 
    if (parm.samplingpos->answer && parm.outfile->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
			parm.samplingpos->key, parm.outfile->key);
 
    if (!parm.output->answer && !parm.outfile->answer)
        G_fatal_error(_("Please specify %s= or %s="),
			parm.output->key, parm.outfile->key);
 
    if (parm.output->answer && !parm.samplingpos->answer)
        G_fatal_error(_("Please specify %s= and %s="),
			parm.output->key, parm.samplingpos->key);

    if (parm.input->answer && parm.infile->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
			parm.input->key, parm.infile->key);
 
    if (parm.datapos->answer && parm.infile->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
			parm.datapos->key, parm.infile->key);
 
    if (!parm.input->answer && !parm.infile->answer)
        G_fatal_error(_("Please specify %s= or %s="),
			parm.input->key, parm.infile->key);
 
    if (parm.input->answer && !parm.datapos->answer)
        G_fatal_error(_("Please specify %s= and %s="),
			parm.input->key, parm.datapos->key);

    if(G_strncasecmp(parm.method->answer, "linear", 6) == 0)
        interpol_method = LINEAR_INTERPOLATION;

    if(G_strncasecmp(parm.method->answer, "spline", 6) == 0)
        interpol_method = SPLINE_INTERPOLATION;

    inputs = get_parameter_input("input", parm.input->answers, parm.datapos->answers, parm.infile->answer, &num_inputs);
    outputs = get_parameter_input("output", parm.output->answers, parm.samplingpos->answers, parm.outfile->answer, &num_outputs);

    start_interpolation(inputs, num_inputs, outputs, num_outputs, interpol_method);

    exit(EXIT_SUCCESS);
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

struct map_store *get_parameter_input(const char *type, char **map_names, char **positions, char *file, int *number_of_maps) 
{
    struct map_store *maps = NULL;
    int max_maps;
    int num_maps;
    int num_points;

    /* process the output maps from the infile */
    if (file) {
	FILE *in;
    
	in = fopen(file, "r");
	if (!in)
	    G_fatal_error(_("Unable to open %s file <%s>"), type, file);
    
	num_maps = 0;
	max_maps = 0;

	for (;;) {
	    char buf[GNAME_MAX + 50]; /* Name and position */
	    char tok_buf[GNAME_MAX + 50]; /* Name and position */
	    char *name;
            int ntokens;
            char **tokens;
	    struct map_store *p;
            double pos = -1;

	    if (!G_getl2(buf, sizeof(buf), in))
		break;

            strcpy(tok_buf, buf);
            tokens = G_tokenize(tok_buf, "|");
            ntokens = G_number_of_tokens(tokens);

            if(ntokens > 1) {
	        name = G_chop(tokens[0]);
	        pos = atof(G_chop(tokens[1]));
            } else {
	        name = G_chop(buf);
            }

	    /* Ignore empty lines */
	    if (!*name)
		continue;
        
            if(pos == -1)
	        G_fatal_error(_("Missing point position for %s map <%s>"
                                " in file <%s> near line %i"), 
                                type, name, file, num_maps);

	    if (num_maps >= max_maps) {
		max_maps += 100;
		maps = G_realloc(maps, max_maps * sizeof(struct map_store));
	    }
	    p = &maps[num_maps++];

	    p->name = G_store(name);
            p->pos = pos;
            p->fd = -1;
            p->buf = NULL;
            p->has_run = 0;
            G_verbose_message(_("Preparing %s map <%s> at position %g"), type, p->name, p->pos);
	}

        if (num_maps < 1)
            G_fatal_error(_("No raster map name found in %s file <%s>"), type, file);
     
	fclose(in);
    }
    else {
        int i;
    	for (i = 0; map_names[i]; i++)
	    ;
    	num_maps = i;

    	if (num_maps < 1)
	    G_fatal_error(_("No %s raster map not found"), type);

        for (i = 0; positions[i]; i++)
	    ;
        num_points = i;
    
        if (num_points != num_maps)
            G_fatal_error(_("The number of %s maps and %s point positions must be equal"), type, type);
        
    	maps = G_malloc(num_maps * sizeof(struct map_store));

    	for (i = 0; i < num_maps; i++) {
	    struct map_store *p = &maps[i];

	    p->name = map_names[i];
            p->pos = (DCELL)atof(positions[i]);
            p->fd = -1;
            p->buf = NULL;
            p->has_run = 0;
            G_verbose_message(_("Preparing %s map <%s> at position %g"), type, p->name, p->pos);
        }
    }
    *number_of_maps = num_maps;
    return maps;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

void start_interpolation(struct map_store *inputs, int num_inputs, struct map_store *outputs, int num_outputs, int interpol_method) 
{ 
    int i;
    struct map_store **inp = (struct map_store**) G_malloc(num_inputs * sizeof(struct map_store*));
    struct map_store **outp = (struct map_store**) G_malloc(num_outputs * sizeof(struct map_store*));

    G_verbose_message(_("Start interpolation run with %i input maps and %i output maps"), 
                      num_inputs, num_outputs);

    for(i = 0; i < num_inputs; i++) 
        inp[i] = &inputs[i];
    for(i = 0; i < num_outputs; i++)
        outp[i] = &outputs[i];

    /* Sort input and output pointer by their point position 
     * using brute force. :) 
     */

    selection_sort(inp, num_inputs);
    selection_sort(outp, num_outputs);

    if(interpol_method == LINEAR_INTERPOLATION) 
        linear_interpolation(inp, num_inputs, outp, num_outputs);

   for(i = 0; i < num_outputs; i++) {
       if(outp[i]->has_run == 0) 
           G_warning(_("map <%s> at position %g was not interpolated. Check the interpolation interval."), outp[i]->name, outp[i]->pos);
   }
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

void linear_interpolation(struct map_store **inp, int num_inputs, 
                          struct map_store **outp, int num_outputs)
{
   struct map_store *left;
   struct map_store *right;
   struct History history;
   int interval, l, row;
   int nrows = Rast_window_rows();
   int ncols = Rast_window_cols();
   int start = 0;

   if(num_inputs < 2)
       G_fatal_error(_("At least 2 input maps are required for linear interpolation")); 

   /* Interpolate for each interval */
   for(interval = 0; interval < num_inputs - 1; interval++) {

       left = inp[interval];
       right = inp[interval + 1];
       left->fd = Rast_open_old(left->name, "");
       right->fd = Rast_open_old(right->name, "");
       left->buf = Rast_allocate_d_buf();
       right->buf = Rast_allocate_d_buf();

       for(l = start; l < num_outputs; l++) {
           /* Check if the map is in the interval and process it */
           if(outp[l]->pos >= left->pos && outp[l]->pos <= right->pos) {
               outp[l]->fd = Rast_open_new(outp[l]->name, DCELL_TYPE);
               outp[l]->buf = Rast_allocate_d_buf();

               G_verbose_message(_("Interpolate map <%s> at position %g in interval (%g;%g)"), 
                                outp[l]->name, outp[l]->pos, left->pos, right->pos);
               G_verbose_message(_("Percent complete..."));

               for (row = 0; row < nrows; row++) {
                   G_percent(row, nrows, 2);

                   Rast_get_d_row(left->fd, left->buf, row);
                   Rast_get_d_row(right->fd, right->buf, row);

                   interpolate_row_linear(left, right, outp[l], ncols);
                   Rast_put_d_row(outp[l]->fd, outp[l]->buf);
               }

               G_percent(row, nrows, 2);

               Rast_close(outp[l]->fd);
               Rast_short_history(outp[l]->name, "raster", &history);
               Rast_command_history(&history);
               Rast_write_history(outp[l]->name, &history);

               G_free(outp[l]->buf);
               outp[l]->has_run = 1;
           
               start = l;
           }
       }
       Rast_close(left->fd);
       G_free(left->buf);
       Rast_close(right->fd);
       G_free(right->buf);
   }
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */
/* linear function v = (1 - pos/dist) * u1 + pos/dist * u2
 *
 * v    -> The value of the output map
 * pos  -> The normalized position of the output map
 * u1   -> The value of the left input map
 * u2   -> The value of the right input map
 * dist -> The distance between the position of u1 and u2
 *
 * */
void interpolate_row_linear(struct map_store *left, struct map_store *right, struct map_store *out, int ncols)
{
    DCELL v;
    DCELL u1;
    DCELL u2;
    DCELL dist;
    int col;

    for (col = 0; col < ncols; col++) {
        u1 = left->buf[col];
        u2 = right->buf[col];
        dist =  fabs(right->pos -  left->pos);

        if(Rast_is_d_null_value(&u1) || Rast_is_d_null_value(&u2)) {
            Rast_set_d_null_value(&v, 1);
        } else {
            v = (1 - ((out->pos - left->pos)/dist)) * u1 + ((out->pos - left->pos)/dist) * u2;
        }
        out->buf[col] = v;
    }
    return;
}

/* *************************************************************** */
/* *************************************************************** */
/* *************************************************************** */

void selection_sort(struct map_store **array, int num) 
{
    int i, j, b;
    struct map_store *min;

    for (i = 0; i < num - 1; i++) {
        b = i;
        min = array[b];

        for (j = i + 1; j < num; j++) {
            if (array[j]->pos < min->pos) {
                b = j;
                min = array[b];
            }
        }
    array[b] = array[i];
    array[i] = min;
    }
}

