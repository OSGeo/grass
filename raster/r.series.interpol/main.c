
/****************************************************************************
 *
 * MODULE:       r.series.interpol
 * AUTHOR(S):    Soeren Gebbert <soerengebbert googlemail.com>
 *               Code is based on r.series from Glynn Clements <glynn gclements.plus.com> 
 *
 * PURPOSE:      Interpolate raster maps located (temporal or spatial) in between input raster maps at specific sampling positions
 * COPYRIGHT:    (C) 2011-2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
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
#define QUADRATIC_INTERPOLATION 2
#define CUBIC_INTERPOLATION 3

struct map_store
{
    const char *name;
    double pos;
    DCELL *buf;
    int fd;
};

double linear_position[2] = {0.0, 1.0};
double quadratic_position[3] = {0.0, 0.5, 1.0};
double cubic_position[4] = {0.0, 1.0/3.0, 2.0/3.0, 1.0};

static void linear_interpolation(struct map_store *inputs, struct map_store *out, int ncols);
static int start_interpolation(struct map_store *inputs, int num_inputs, int interpolation_method, struct map_store *out); 

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *input, *file, *output, *sampoints, *method;
    } parm;
    int i;
    int num_outputs;
    int num_inputs;
    int num_sampoints;
    struct map_store *inputs = NULL;
    struct map_store *outputs = NULL;
    int interpol_method = LINEAR_INTERPOLATION;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("series"));
    G_add_keyword(_("interpolation"));
    module->description =
	_("Interpolate raster maps located (temporal or spatial) "
          "in between input raster maps at specific sampling positions.");

    parm.input = G_define_standard_option(G_OPT_R_INPUTS);

    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->multiple = YES;
    parm.output->required = NO;

    parm.sampoints = G_define_option();
    parm.sampoints->key = "sampoints";
    parm.sampoints->type = TYPE_DOUBLE;
    parm.sampoints->required = NO;
    parm.sampoints->description = _("Sampling point for each input map,"
                                   " the point must in between the interval (0;1)");
    parm.sampoints->multiple = YES;

    parm.file = G_define_standard_option(G_OPT_F_INPUT);
    parm.file->key = "file";
    parm.file->description = _("Input file with output a raster map name and sample point per line,"
                               " field separator between name and sample point is |");
    parm.file->required = NO;

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

    if (parm.output->answer && parm.file->answer)
        G_fatal_error(_("output= and file= are mutually exclusive"));
 
    if (parm.sampoints->answer && parm.file->answer)
        G_fatal_error(_("sampoints= and file= are mutually exclusive"));
 

    if (!parm.output->answer && !parm.file->answer)
        G_fatal_error(_("Please specify output= or file="));
 
    if (parm.output->answer && !parm.sampoints->answer)
        G_fatal_error(_("Please specify output= and sampoints="));

    if(G_strncasecmp(parm.method->answer, "linear", 6))
        interpol_method = LINEAR_INTERPOLATION;

    /* process the input maps */
    for (i = 0; parm.input->answers[i]; i++)
	;
    num_inputs = i;

    if (num_inputs < 1)
	G_fatal_error(_("No input raster map not found"));

    if(interpol_method == LINEAR_INTERPOLATION)
        if (num_inputs != 2)
	    G_fatal_error(_("You need to specify two input maps for linear interpolation"));

    if(interpol_method == QUADRATIC_INTERPOLATION)
        if (num_inputs != 3)
	    G_fatal_error(_("You need to specify three input maps for quadratic interpolation"));

    if(interpol_method == CUBIC_INTERPOLATION)
        if (num_inputs != 4)
	    G_fatal_error(_("You need to specify 4 input maps for linear interpolation"));


    inputs = G_calloc(num_inputs, sizeof(struct map_store));

    for (i = 0; i < num_inputs; i++) {
	struct map_store *in = &inputs[i];

	in->name = parm.input->answers[i];	
        
        if(interpol_method == LINEAR_INTERPOLATION)
            in->pos = linear_position[i];
        if(interpol_method == QUADRATIC_INTERPOLATION)
            in->pos = quadratic_position[i];
        if(interpol_method == CUBIC_INTERPOLATION)
            in->pos = cubic_position[i];

	in->buf = Rast_allocate_d_buf();
	in->fd = Rast_open_old(in->name, "");
        G_verbose_message(_("Reading input raster map <%s> at sample point %g..."), in->name, in->pos);
    }

    /* process the output maps from the file */
    if (parm.file->answer) {
	FILE *in;
	int max_outputs;
        double left = 0.0;
        double right = 1.0;
    
        if(interpol_method == LINEAR_INTERPOLATION) {
            left = linear_position[0];
            right = linear_position[1];
        }

	in = fopen(parm.file->answer, "r");
	if (!in)
	    G_fatal_error(_("Unable to open input file <%s>"), parm.file->answer);
    
	num_outputs = 0;
	max_outputs = 0;

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
                if(pos < left || pos > right)
	            G_fatal_error(_("Wrong sampling point for output map <%s> in file <%s> "
                                    "near line %i, sampling point must be in between (%g:%g) not: %g"), 
                                     name, parm.file->answer, num_outputs + 1, left, right, pos);
            } else {
	        name = G_chop(buf);
            }

	    /* Ignore empty lines */
	    if (!*name)
		continue;
        
            if(pos == -1)
	        G_fatal_error(_("Missing sampling point for output map <%s>"
                                " in file <%s> near line %i"), 
                                name, parm.file->answer, num_outputs);
            

	    if (num_outputs >= max_outputs) {
		max_outputs += 100;
		outputs = G_realloc(outputs, max_outputs * sizeof(struct map_store));
	    }
	    p = &outputs[num_outputs++];

	    p->name = G_store(name);
            p->pos = pos;
            p->fd = -1;
            p->buf = NULL;
	}

        if (num_outputs < 1)
            G_fatal_error(_("No raster map name found in file <%s>"), parm.file->answer);
     
	fclose(in);
    }
    else {
    	for (i = 0; parm.output->answers[i]; i++)
	    ;
    	num_outputs = i;

    	if (num_outputs < 1)
	    G_fatal_error(_("No output raster map not found"));

        for (i = 0; parm.sampoints->answers[i]; i++)
	    ;
        num_sampoints = i;
    
        if (num_sampoints != num_outputs)
                G_fatal_error(_("input= and sampoints= must have the same number of values"));
        
    	outputs = G_malloc(num_outputs * sizeof(struct map_store));

    	for (i = 0; i < num_outputs; i++) {
	    struct map_store *p = &outputs[i];

	    p->name = parm.output->answers[i];
            p->pos = (DCELL)atof(parm.sampoints->answers[i]);
            p->fd = -1;
            p->buf = NULL;
        }
    }
  
    /* Start the interpolation for each output map */
    for (i = 0; i < num_outputs; i++) {
            G_verbose_message(_("Processing output raster map <%s> at sample point %g..."), outputs[i].name, outputs[i].pos);
            start_interpolation(inputs, num_inputs, interpol_method, &outputs[i]);
    }

    /* Close input maps */
   for (i = 0; i < num_inputs; i++)
        Rast_close(inputs[i].fd);

    exit(EXIT_SUCCESS);
}

int start_interpolation(struct map_store *inputs, int num_inputs, int interpol_method, struct map_store *out) { 

    struct History history;
    int row, i;
    int nrows = Rast_window_rows();
    int ncols = Rast_window_cols();

    out->fd = Rast_open_new(out->name, DCELL_TYPE);
    out->buf = Rast_allocate_d_buf();

    /* process the data */
    G_verbose_message(_("Percent complete..."));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

        for (i = 0; i < num_inputs; i++)
            Rast_get_d_row(inputs[i].fd, inputs[i].buf, row);

        if(interpol_method == LINEAR_INTERPOLATION)
            linear_interpolation(inputs, out, ncols);
        /* Add new interpolation methods here */

	Rast_put_d_row(out->fd, out->buf);
    }

    G_percent(row, nrows, 2);

    Rast_close(out->fd);

    Rast_short_history(out->name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out->name, &history);

    G_free(out->buf);

    return 1;
}

/* linear function v = (1 - pos/dist) * u1 + pos/dist * u2
 *
 * v    -> The value of the output map
 * pos  -> The normalized position of the output map (0;1)
 * u1   -> The value of the left input map
 * u2   -> The value of the right input map
 * dist -> The distance between the position of u1 and u2
 *
 * */
void linear_interpolation(struct map_store *inputs, struct map_store *out, int ncols)
{
    DCELL v;
    DCELL u1;
    DCELL u2;
    DCELL dist;
    int col;

    for (col = 0; col < ncols; col++) {
        u1 = inputs[0].buf[col];
        u2 = inputs[1].buf[col];
        dist =  fabs(inputs[1].pos -  inputs[0].pos);


        if(Rast_is_d_null_value(&u1) || Rast_is_d_null_value(&u2)) {
            Rast_set_d_null_value(&v, 1);
        } else {
            v = (1 - (out->pos/dist)) * u1 + (out->pos/dist) * u2;
        }
        out->buf[col] = v;
    }
    return;
}
