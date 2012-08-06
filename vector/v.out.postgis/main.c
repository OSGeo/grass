/***************************************************************
 *
 * MODULE:       v.out.postgis
 *
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Converts GRASS vector map layer to PostGIS
 *
 * COPYRIGHT:    (C) 2012 by Martin Landa, and the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct params params;
    struct flags flags;
    
    int ret, field, do_areas;
    struct Map_info In, Out;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("export"));
    G_add_keyword(_("PostGIS"));

    module->description =
        _("Converts a vector map layer to PostGIS.");
    module->overwrite = TRUE;
    
    define_options(&params, &flags);
    
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* if olayer not given, use input as the name */
    if (!params.olayer->answer)
        params.olayer->answer = G_store(params.input->answer);
    
    /* open input for reading */
    ret = Vect_open_old2(&In, params.input->answer, "", params.layer->answer);
    if (ret == -1)
        G_fatal_error(_("Unable to open vector map <%s>"),
                      params.input->answer);
    if (ret < 2) 
        G_warning(_("Unable to open vector map <%s> on topological level"),
                  params.input->answer);
    
    /* create output for writing */
    create_pgfile(params.dsn->answer, flags.topo->answer ? TRUE : FALSE);
    
    if (-1 == Vect_open_new(&Out, params.olayer->answer, Vect_is_3d(&In)))
        G_fatal_error(_("Unable to create PostGIS layer <%s>"),
                      params.olayer->answer);
    
    /* define attributes */
    field = Vect_get_field_number(&In, params.layer->answer);
    if (!flags.table->answer)
        Vect_copy_map_dblinks(&In, &Out, TRUE);

    ret = 0;
    if (flags.topo->answer) {
        /* PostGIS topology export */
        G_message(_("Exporting features (topology)..."));
        ret = export_topo(&In, field, &Out);
    }
    else {
        /* simple features export */
        int do_areas;

        do_areas = FALSE;
        if (Vect_level(&In) >= 2) {
            if (Vect_get_num_areas(&In) > 0)
                do_areas = TRUE;
        }
        else {
            G_warning(_("Unable to process areas"));
        }
        
        if (!do_areas) {
            G_message(_("Exporting features..."));
            ret = export_lines(&In, field, &Out);
        }
        else {
            G_message(_("Exporting areas..."));
            ret = export_areas(&In, field, &Out);
        }
    }
    
    if (ret > 0)
        G_message(_("%d features exported"), ret);

    Vect_build(&Out);
    
    Vect_close(&In);
    Vect_close(&Out);
    
    exit(EXIT_SUCCESS);
}
