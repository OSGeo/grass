/****************************************************************************
 *
 * MODULE:       r.random.cells
 * AUTHOR(S):    Charles Ehlschlaeger; National Center for Geographic
 *                 Information and Analysis, University of California,
 *                 Santa Barbara (original contributor)
 *               Markus Neteler <neteler itc.it>
 *               Roberto Flor <flor itc.it>,
 *               Brad Douglas <rez touchofmadness.com>,
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      generates a random sets of cells that are at least
 *               some distance apart
 * COPYRIGHT:    (C) 1999-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "ransurf.h"
#include "local_proto.h"

double NS, EW;
int CellCount, Rs, Cs;
double MaxDist, MaxDistSq;
FLAG *Cells;
CELLSORTER *DoNext;
CELL **Out, *CellBuffer;
int Seed, OutFD;
int MaxCellsNum;
struct Flag *Verbose;
struct Option *Distance;
struct Option *Output;
struct Option *SeedStuff;
struct Option *MaxCells;

int main(int argc, char *argv[])
{
    struct GModule *module;

    G_gisinit(argv[0]);
    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("sampling"));
    G_add_keyword(_("random"));
    G_add_keyword(_("autocorrelation"));
    module->description =
        _("Generates random cell values with spatial dependence.");

    Output = G_define_standard_option(G_OPT_R_OUTPUT);

    Distance = G_define_option();
    Distance->key = "distance";
    Distance->type = TYPE_DOUBLE;
    Distance->required = YES;
    Distance->multiple = NO;
    Distance->description =
        _("Maximum distance of spatial correlation (value >= 0.0)");

    MaxCells = G_define_option();
    MaxCells->key = "ncells";
    MaxCells->type = TYPE_INTEGER;
    MaxCells->required = NO;
    MaxCells->options = "1-";
    MaxCells->description = _("Maximum number of cells to be created");

    SeedStuff = G_define_standard_option(G_OPT_M_SEED);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    Init();
    Indep();

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}
