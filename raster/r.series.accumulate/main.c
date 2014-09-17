/****************************************************************************
 * 
 * MODULE:       r.series.accumulate
 * AUTHOR(S):    Markus Metz
 *               Soeren Gebbert
 *               based on r.series
 * PURPOSE:      Calculates (accumulated) raster value means, growing degree days
 *               (GDDs) or Winkler indices from several input maps.
 * COPYRIGHT:    (C) 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define METHOD_GDD 1
#define METHOD_MEAN 2
#define METHOD_WINKLER 3
#define METHOD_BEDD 4
#define METHOD_HUGLIN 5

struct map_info
{
    const char *name;
    int fd;
    DCELL *buf;
};

struct map_info_out
{
    const char *name;
    int fd;
    void *buf;
};

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
        struct Option *input, *basemap, *file, *output,
        *range, *scale, *shift, *lower,
        *upper, *limits, *method;
    } parm;
    struct
    {
        struct Flag *nulls, *lazy, *float_output;
    } flag;
    int i;
    int num_inputs, max_inputs;
    int method;
    struct map_info *inputs = NULL;
    struct map_info_out *out = NULL;
    struct map_info *basemap = NULL;
    struct map_info *map_lower = NULL;
    struct map_info *map_upper = NULL;
    struct History history;
    int nrows, ncols;
    int row, col;
    DCELL lo, hi, tscale, tshift, lower = 10.0, upper = 30.0;
    DCELL dcell_null;
    RASTER_MAP_TYPE out_type;
    int out_size;
    char *desc = NULL;
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("series"));
    G_add_keyword(_("accumulation"));
    module->description =
    _("Makes each output cell value a accumulation"
      "function of the values assigned to the corresponding cells "
      "in the input raster map layers.");

    parm.basemap = G_define_standard_option(G_OPT_R_INPUT);
    parm.basemap->key = "basemap";
    parm.basemap->description = _("Existing map to be added to output");
    parm.basemap->required = NO;

    parm.input = G_define_standard_option(G_OPT_R_INPUTS);
    parm.input->required = NO;

    parm.file = G_define_standard_option(G_OPT_F_INPUT);
    parm.file->key = "file";
    parm.file->description = _("Input file with raster map names, one per line");
    parm.file->required = NO;
    
    parm.output = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.output->multiple = NO;
    
    parm.scale = G_define_option();
    parm.scale->key = "scale";
    parm.scale->type = TYPE_DOUBLE;
    parm.scale->answer = "1.0";
    parm.scale->required = NO;
    parm.scale->description = _("Scale factor for input");
    
    parm.shift = G_define_option();
    parm.shift->key = "shift";
    parm.shift->type = TYPE_DOUBLE;
    parm.shift->answer = "0.0";
    parm.shift->required = NO;
    parm.shift->description = _("Shift factor for input");
    
    parm.lower = G_define_standard_option(G_OPT_R_INPUT);
    parm.lower->key = "lower";
    parm.lower->required = NO;
    parm.lower->description = _("The raster map specifying the lower accumulation limit, also called baseline");
    
    parm.upper = G_define_standard_option(G_OPT_R_INPUT);
    parm.upper->key = "upper";
    parm.upper->required = NO;
    parm.upper->description = _("The raster map specifying the upper accumulation limit, also called cutoff. Only applied to BEDD computation.");
    
    parm.range = G_define_option();
    parm.range->key = "range";
    parm.range->type = TYPE_DOUBLE;
    parm.range->key_desc = "min,max";
    parm.range->description = _("Ignore values outside this range");
    
    parm.limits = G_define_option();
    parm.limits->key = "limits";
    parm.limits->type = TYPE_DOUBLE;
    parm.limits->key_desc = "lower,upper";
    parm.limits->answer = "10,30";
    parm.limits->description = _("Use these limits in case lower and/or upper input maps are not defined");
    
    parm.method = G_define_option();
    parm.method->key = "method";
    parm.method->type = TYPE_STRING;
    parm.method->multiple = NO;
    parm.method->required = NO;
    parm.method->options = "gdd,bedd,huglin,mean";
    parm.method->answer = "gdd";
    parm.method->label = "This method will be applied to compute the accumulative values from the input maps";
    G_asprintf(&desc,
           "gdd;%s;mean;%s;bedd;%s;huglin;%s",
           _("Growing Degree Days or Winkler indices"),
           _("Mean: sum(input maps)/(number of input maps)"),
           _("Biologically Effective Degree Days"),
           _("Huglin Heliothermal index"));
    parm.method->descriptions = desc;

    flag.nulls = G_define_flag();
    flag.nulls->key = 'n';
    flag.nulls->description = _("Propagate NULLs");
    
    flag.lazy = G_define_flag();
    flag.lazy->key = 'z';
    flag.lazy->description = _("Do not keep files open");
    
    flag.float_output = G_define_flag();
    flag.float_output->key = 'f';
    flag.float_output->description = _("Create a FCELL map (floating point single precision) as output");
    
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);
    
    lo = -1.0 / 0.0;    /* -inf */
    hi =  1.0 / 0.0;    /* inf */
    
    method = METHOD_GDD;
    if (G_strncasecmp(parm.method->answer, "gdd", 3) == 0)
        method = METHOD_GDD;
    else if (G_strncasecmp(parm.method->answer, "mean", 4) == 0)
        method = METHOD_MEAN;
    else if (G_strncasecmp(parm.method->answer, "bedd", 4) == 0)
        method = METHOD_BEDD;
    else if (G_strncasecmp(parm.method->answer, "huglin", 7) == 0)
        method = METHOD_HUGLIN;

    if (parm.range->answer) {
        lo = atof(parm.range->answers[0]);
        hi = atof(parm.range->answers[1]);
    }
    
    if (parm.limits->answer) {
        lower = atof(parm.limits->answers[0]);
        upper = atof(parm.limits->answers[1]);
    }
    
    if (parm.scale->answer)
        tscale = atof(parm.scale->answer);
    else
        tscale = 1.;
    
    if (parm.shift->answer)
        tshift = atof(parm.shift->answer);
    else
        tshift = 0.;
    
    if (parm.input->answer && parm.file->answer)
        G_fatal_error(_("%s= and %s= are mutually exclusive"),
			parm.input->key, parm.file->key);
    
    if (!parm.input->answer && !parm.file->answer)
        G_fatal_error(_("Please specify %s= or %s="),
			parm.input->key, parm.file->key);
    
    max_inputs = 0;
    
    /* process the input maps from the file */
    if (parm.file->answer) {
        FILE *in;
        
        in = fopen(parm.file->answer, "r");
        if (!in)
            G_fatal_error(_("Unable to open input file <%s>"),
                          parm.file->answer);
            
            num_inputs = 0;
        
        for (;;) {
            char buf[GNAME_MAX];
            char *name;
            struct map_info *p;
            
            if (!G_getl2(buf, sizeof(buf), in))
                break;
            
            name = G_chop(buf);
            
            /* Ignore empty lines */
            if (!*name)
                continue;
            
            if (num_inputs >= max_inputs) {
                max_inputs += 100;
                inputs =
                G_realloc(inputs, max_inputs * sizeof(struct map_info));
            }
            p = &inputs[num_inputs++];
            
            p->name = G_store(name);
            G_verbose_message(_("Reading raster map <%s>..."), p->name);
            p->buf = Rast_allocate_d_buf();
            if (!flag.lazy->answer)
                p->fd = Rast_open_old(p->name, "");
        }
        
        if (num_inputs < 1)
            G_fatal_error(_("No raster map name found in input file"));
        
        fclose(in);
    }
    else {
        for (i = 0; parm.input->answers[i]; i++)
            ;
        num_inputs = i;
        
        if (num_inputs < 1)
            G_fatal_error(_("Raster map not found"));
        
        inputs = G_malloc(num_inputs * sizeof(struct map_info));
        
        for (i = 0; i < num_inputs; i++) {
            struct map_info *p = &inputs[i];
            
            p->name = parm.input->answers[i];
            G_verbose_message(_("Reading raster map <%s>..."), p->name);
            p->buf = Rast_allocate_d_buf();
            if (!flag.lazy->answer)
                p->fd = Rast_open_old(p->name, "");
        }
        max_inputs = num_inputs;
    }
    
    if (parm.basemap->answer) {
        basemap = G_malloc(1 * sizeof(struct map_info));
        basemap->name = parm.basemap->answer;
        G_verbose_message(_("Reading raster map <%s>..."), basemap->name);
        basemap->buf = Rast_allocate_d_buf();
        basemap->fd = Rast_open_old(basemap->name, "");
    }
    
    if (parm.lower->answer) {
        map_lower = G_malloc(1 * sizeof(struct map_info));
        map_lower->name = parm.lower->answer;
        G_verbose_message(_("Reading raster map <%s>..."), map_lower->name);
        map_lower->buf = Rast_allocate_d_buf();
        map_lower->fd = Rast_open_old(map_lower->name, "");
    }
    
    if (parm.upper->answer) {
        map_upper = G_malloc(1 * sizeof(struct map_info));
        map_upper->name = parm.upper->answer;
        G_verbose_message(_("Reading raster map <%s>..."), map_upper->name);
        map_upper->buf = Rast_allocate_d_buf();
        map_upper->fd = Rast_open_old(map_upper->name, "");
    }
    
    out = G_calloc(1, sizeof(struct map_info_out));
    out->name = parm.output->answer;
    out_type = flag.float_output->answer ? FCELL_TYPE : DCELL_TYPE;
    out->buf = Rast_allocate_buf(out_type);
    out_size = Rast_cell_size(out_type);
    out->fd = Rast_open_new(out->name, out_type);
    
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    
    Rast_set_d_null_value(&dcell_null, 1);
    
    /* process the data */
    G_verbose_message(_("Percent complete..."));
    
    for (row = 0; row < nrows; row++) {
        G_percent(row, nrows, 2);
        
        if (basemap)
            Rast_get_d_row(basemap->fd, basemap->buf, row);
        if (map_lower)
            Rast_get_d_row(map_lower->fd, map_lower->buf, row);
        if (map_upper)
            Rast_get_d_row(map_upper->fd, map_upper->buf, row);
        
        if (flag.lazy->answer) {
            /* Open the files only on run time */
            for (i = 0; i < num_inputs; i++) {
                inputs[i].fd = Rast_open_old(inputs[i].name, "");
                Rast_get_d_row(inputs[i].fd, inputs[i].buf, row);
                Rast_close(inputs[i].fd);
            }
        }
        else {
            for (i = 0; i < num_inputs; i++)
                Rast_get_d_row(inputs[i].fd, inputs[i].buf, row);
        }
        
        #pragma omp for schedule (static) private (col)
        for (col = 0; col < ncols; col++) {
            int null = 0, non_null = 0;
            DCELL min, max, avg, value;
            
            if (map_lower)
                lower = map_lower->buf[col];
            if (map_upper)
                upper = map_upper->buf[col];
            
            if (upper <= lower)
                G_fatal_error(_("'%s'=%f must be > '%s'=%f"), parm.upper->key, upper,
                              parm.lower->key, lower);
                
            min = dcell_null;
            max = dcell_null;
            avg = 0;
            
            for (i = 0; i < num_inputs; i++) {
                DCELL v = inputs[i].buf[col];
                
                if (Rast_is_d_null_value(&v))
                    null = 1;
                else {
                    v = v * tscale + tshift;
                    if (parm.range->answer && (v < lo || v > hi)) {
                        null = 1;
                    }
                    else  {
                        avg += v;
                        if (min > v || Rast_is_d_null_value(&min))
                            min = v;
                        if (max < v || Rast_is_d_null_value(&max))
                            max = v;
                        non_null++;
                    }
                }
            }

	    value = dcell_null;
            if (!non_null || (null && flag.nulls->answer)) {
                if (basemap)
                    value = basemap->buf[col];
            }
            else {
                /* Compute mean or index */
		avg /= non_null;

                switch(method) {
                    case METHOD_HUGLIN:
                        avg = (avg + max) / 2;
                        break;
                    case METHOD_BEDD:
                        if(avg > upper)
                            avg = upper;
                        break;
                    case METHOD_MEAN:
                        value = avg;
                        break;
                    default:
                        /* Winkler or GDD index computation is the default */
                        break;
                }
                if (method != METHOD_MEAN) {
		    value = avg - lower;

		    if (value < 0.)
			value = 0.;
		}

                if (basemap)
                    value += basemap->buf[col];
            }
            Rast_set_d_value((char *)out->buf + col * out_size, value, out_type);
        }
        Rast_put_row(out->fd, out->buf, out_type);
    }
    
    G_percent(row, nrows, 2);
    
    /* close output map */
    Rast_close(out->fd);
    
    Rast_short_history(out->name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out->name, &history);
    
    /* Close input maps */
    if (basemap)
        Rast_close(basemap->fd);
    if (map_lower)
        Rast_close(map_lower->fd);
    if (map_upper)
        Rast_close(map_upper->fd);
    
    if (!flag.lazy->answer) {
        for (i = 0; i < num_inputs; i++)
            Rast_close(inputs[i].fd);
    }
    
    if (method == METHOD_GDD) {
        struct Colors colors;
        
        Rast_init_colors(&colors);
        Rast_make_colors(&colors, "gdd", 0, 6000);
        Rast_write_colors(out->name, G_mapset(), &colors);
    }
    
    exit(EXIT_SUCCESS);
}
