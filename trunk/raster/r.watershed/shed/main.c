
/****************************************************************************
 *
 * MODULE:       shed
 * AUTHOR(S):    Charles Ehlschlaeger, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, Roberto Flor <flor itc.it>,
 *               Brad Douglas <rez touchofmadness.com>,
 *		 Hamish Bowman <hamish_b yahoo.com>
 * PURPOSE:      Watershed determination
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "watershed.h"
#include "string.h"


int main(int argc, char *argv[])
{
    INPUT input;
    OUTPUT output;

    G_gisinit(argv[0]);
    G_set_program_name("r.watershed");
    G_get_window(&(output.window));
    intro();

    output.num_maps = 0;
    com_line_Gwater(&input, &output);	/* develops r.watershed command line */
    basin_maps(&input, &output);	/* organizes map layers output */
    if (input.fast || input.slow) {
	if (input.fast) {
	    if (G_system(input.com_line_ram)) {
		if (input.slow) {
		    G_message(_("Slow version of water analysis program starting now"));

		    if (G_system(input.com_line_seg)) {
			free_input(&input);
			free_output(&output);
			G_fatal_error(_("<<%s>> command line failed"),
				      input.com_line_seg);
		    }
		}
	    }
	}
	else if (G_system(input.com_line_seg)) {
	    free_input(&input);
	    free_output(&output);
	    G_fatal_error(_("<<%s>> command line failed"),
			  input.com_line_seg);
	}
    }

    /*
       ARMSED:  This section exists to create the stats part.
       input.ar_file_name could be used as a flag to determine this stuff
       should be run.
     */
#ifdef ARMSED
    ar_file_in(input.ar_file_name, &output);
    read_basins(input.haf_name, &output);
    valid_basins(input.accum_name, &output);
    free_input(&input);
    if ((output.out_file = fopen(output.file_name, "w")) == NULL) {
	free_output(&output);
	G_fatal_error(_("unable to open output file"));
    }
    if (output.do_basin) {
	fprintf(output.out_file,
		"\n\nThese values are accumulations within the basin itself\n");
	fprintf(output.out_file, "They do not include sub-basins\n\n");
	print_output(&output);
    }
    if (output.do_accum) {
	accum_down(&output);
	fprintf(output.out_file,
		"\n\nThese values are accumulations of basins and sub-basins\n");
	print_output(&output);
    }
    free_output(&output);

#endif
    /*
       end of ARMSED comment code
     */

    exit(EXIT_SUCCESS);
}
