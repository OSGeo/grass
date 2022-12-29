
/****************************************************************************
 *
 * MODULE:       i.aster.toar
 * AUTHOR(S):    Yann Chemin - yann.chemin@gmail.com
 * PURPOSE:      Calculate TOA Reflectance for Aster from DN.
 * 		 Input 9 bands (VNIR and SWIR).
 *
 * COPYRIGHT:    (C) 2002-2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU Lesser General Public
 *   	    	 License. Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define MAXFILES 15

/* DN to radiance conversion factors */
double gain_aster(int band_number, int gain_code);

    /*Gain Code */
    /*0 - High (Not Applicable for band 10-14: TIR) */
    /*1 - Normal */
    /*2 - Low 1(Not Applicable for band 10-14: TIR) */
    /*3 - Low 2(Not Applicable for Band 1-3N/B & 10-14) */

/*sun exo-atmospheric irradiance */
#define KEXO1 1828.0
#define KEXO2 1559.0
#define KEXO3 1045.0
#define KEXO4 226.73
#define KEXO5 86.50
#define KEXO6 81.99
#define KEXO7 74.72
#define KEXO8 66.41
#define KEXO9 59.83

#define PI M_PI

double rad2ref_aster(double radiance, double doy, double sun_elevation,
                     double k_exo);

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;    /*region+header info */
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *input, *output;
    struct Option *input1, *input2;
    struct Flag *flag0, *flag1, *flag2;
    struct Flag *flag3, *flag4, *flag5;
    struct History history;     /*metadata */

    /************************************/
    char *name;                 /*input raster name */
    char *result;               /*output raster name */

    /*Prepare new names for output files */
    char result0[GNAME_MAX], result1[GNAME_MAX];
    char result2[GNAME_MAX], result3[GNAME_MAX];
    char result4[GNAME_MAX], result5[GNAME_MAX];
    char result6[GNAME_MAX], result7[GNAME_MAX];
    char result8[GNAME_MAX], result9[GNAME_MAX];
    char result10[GNAME_MAX], result11[GNAME_MAX];
    char result12[GNAME_MAX], result13[GNAME_MAX];
    char result14[GNAME_MAX];

    /*File Descriptors */
    int infd[MAXFILES];
    int outfd[MAXFILES];
    char **names, **ptr;

    /* For some strange reason infd[0] cannot be used later */
    /* So nfiles is initialized with nfiles = 1 */
    int nfiles = 0;
    int i = 0, j = 0;
    int radiance = 0;
    void *inrast[MAXFILES];
    DCELL *outrast[MAXFILES];
    RASTER_MAP_TYPE in_data_type[MAXFILES];
    RASTER_MAP_TYPE out_data_type = DCELL_TYPE; /* 0=numbers  1=text */
    double gain[MAXFILES]; /* , offset[MAXFILES]; */
    double kexo[MAXFILES];
    double doy, sun_elevation;

    /************************************/
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("radiometric conversion"));
    G_add_keyword(_("radiance"));
    G_add_keyword(_("reflectance"));
    G_add_keyword(_("brightness temperature"));
    G_add_keyword(_("satellite"));
    G_add_keyword(_("ASTER"));
    module->description =
        _("Calculates Top of Atmosphere Radiance/Reflectance/Brightness Temperature from ASTER DN.");

    /* Define the different options */
    input = G_define_standard_option(G_OPT_R_INPUTS);
    input->description = _("Names of ASTER DN layers (15 layers)");

    input1 = G_define_option();
    input1->key = "dayofyear";
    input1->type = TYPE_DOUBLE;
    input1->required = YES;
    input1->gisprompt = "value";
    input1->description = _("Day of Year of satellite overpass [0-366]");

    input2 = G_define_option();
    input2->key = "sun_elevation";
    input2->type = TYPE_DOUBLE;
    input2->required = YES;
    input2->gisprompt = "value";
    input2->description = _("Sun elevation angle (degrees, < 90.0)");

    output = G_define_standard_option(G_OPT_R_OUTPUT);
    output->description = _("Base name of the output layers (will add .x)");

    /* Define the different flags */
    flag0 = G_define_flag();
    flag0->key = 'r';
    flag0->description = _("Output is radiance (W/m2)");

    flag1 = G_define_flag();
    flag1->key = 'a';
    flag1->description = _("VNIR is High Gain");

    flag2 = G_define_flag();
    flag2->key = 'b';
    flag2->description = _("SWIR is High Gain");

    flag3 = G_define_flag();
    flag3->key = 'c';
    flag3->description = _("VNIR is Low Gain 1");

    flag4 = G_define_flag();
    flag4->key = 'd';
    flag4->description = _("SWIR is Low Gain 1");

    flag5 = G_define_flag();
    flag5->key = 'e';
    flag5->description = _("SWIR is Low Gain 2");

    /********************/
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    names = input->answers;
    ptr = input->answers;
    doy = atof(input1->answer);
    sun_elevation = atof(input2->answer);
    result = output->answer;

    radiance = (flag0->answer);

    /********************/
    /*Prepare the output file names */

    /********************/
    sprintf(result0, "%s%s", result, ".1");
    sprintf(result1, "%s%s", result, ".2");
    sprintf(result2, "%s%s", result, ".3N");
    sprintf(result3, "%s%s", result, ".3B");
    sprintf(result4, "%s%s", result, ".4");
    sprintf(result5, "%s%s", result, ".5");
    sprintf(result6, "%s%s", result, ".6");
    sprintf(result7, "%s%s", result, ".7");
    sprintf(result8, "%s%s", result, ".8");
    sprintf(result9, "%s%s", result, ".9");
    sprintf(result10, "%s%s", result, ".10");
    sprintf(result11, "%s%s", result, ".11");
    sprintf(result12, "%s%s", result, ".12");
    sprintf(result13, "%s%s", result, ".13");
    sprintf(result14, "%s%s", result, ".14");

    /********************/
    /*Prepare radiance boundaries */

    /********************/
    int gain_code = 1;

    for (i = 0; i < MAXFILES; i++) {
        /*0 - High (Not Applicable for band 10-14: TIR) */
        /*1 - Normal */
        /*2 - Low 1(Not Applicable for band 10-14: TIR) */
        /*3 - Low 2(Not Applicable for Band 1-3N/B & 10-14) */
        if (flag1->answer && i <= 3)
            gain_code = 0;
        if (flag2->answer && i >= 4 && i <= 9)
            gain_code = 0;
        if (flag3->answer && i <= 3)
            gain_code = 2;
        if (flag4->answer && i >= 4 && i <= 9)
            gain_code = 2;
        if (flag5->answer && i >= 4 && i <= 9)
            gain_code = 3;
        gain[i] = gain_aster(i, gain_code);
        /* Reset to NORMAL GAIN */
        gain_code = 1;
    }

    /********************/
    /*Prepare sun exo-atm irradiance */

    /********************/
    kexo[0] = KEXO1;
    kexo[1] = KEXO2;
    kexo[2] = KEXO3;
    kexo[3] = KEXO3;
    kexo[4] = KEXO4;
    kexo[5] = KEXO5;
    kexo[6] = KEXO6;
    kexo[7] = KEXO7;
    kexo[8] = KEXO8;
    kexo[9] = KEXO9;

    /********************/

    /********************/
    for (; *ptr != NULL; ptr++) {
        if (nfiles == MAXFILES)
            G_fatal_error(_("Too many input maps. Only %d allowed."),
                          MAXFILES);
        name = *ptr;
        /* Allocate input buffer */
        in_data_type[nfiles] = Rast_map_type(name, "");
        infd[nfiles] = Rast_open_old(name, "");

        Rast_get_cellhd(name, "", &cellhd);
        inrast[nfiles] = Rast_allocate_buf(in_data_type[nfiles]);
        nfiles++;
    }
    if (nfiles < MAXFILES)
        G_fatal_error(_("The input band number should be %d"), MAXFILES);

    /***************************************************/
    /* Allocate output buffer, use input map data_type */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    out_data_type = DCELL_TYPE;
    for (i = 0; i < MAXFILES; i++)
        outrast[i] = Rast_allocate_buf(out_data_type);

    outfd[0] = Rast_open_new(result0, 1);
    outfd[1] = Rast_open_new(result1, 1);
    outfd[2] = Rast_open_new(result2, 1);
    outfd[3] = Rast_open_new(result3, 1);
    outfd[4] = Rast_open_new(result4, 1);
    outfd[5] = Rast_open_new(result5, 1);
    outfd[6] = Rast_open_new(result6, 1);
    outfd[7] = Rast_open_new(result7, 1);
    outfd[8] = Rast_open_new(result8, 1);
    outfd[9] = Rast_open_new(result9, 1);
    outfd[10] = Rast_open_new(result10, 1);
    outfd[11] = Rast_open_new(result11, 1);
    outfd[12] = Rast_open_new(result12, 1);
    outfd[13] = Rast_open_new(result13, 1);
    outfd[14] = Rast_open_new(result14, 1);
    /* Process pixels */

    DCELL dout[MAXFILES];
    DCELL d[MAXFILES];

    for (row = 0; row < nrows; row++) {
        G_percent(row, nrows, 2);
        /* read input map */
        for (i = 0; i < MAXFILES; i++)
            Rast_get_row(infd[i], inrast[i], row, in_data_type[i]);

        /*process the data */
        for (col = 0; col < ncols; col++) {
            for (i = 0; i < MAXFILES; i++) {
                switch (in_data_type[i]) {
                case CELL_TYPE:
                    d[i] = (double)((CELL *) inrast[i])[col];
                    break;
                case FCELL_TYPE:
                    d[i] = (double)((FCELL *) inrast[i])[col];
                    break;
                case DCELL_TYPE:
                    d[i] = (double)((DCELL *) inrast[i])[col];
                    break;
                }
                /* if radiance mode or Thermal band */
                if (radiance || i >= 10) {
                    dout[i] = gain[i] * (d[i] - 1.0);
                }
                /* if reflectance default mode and Not Thermal Band */
                else {
                    dout[i] = gain[i] * (d[i] - 1.0);
                    dout[i] =
                        rad2ref_aster(dout[i], doy, sun_elevation, kexo[i]);
                }
                outrast[i][col] = dout[i];
            }
        }
        for (i = 0; i < MAXFILES; i++)
            Rast_put_row(outfd[i], outrast[i], out_data_type);
    }
    for (i = 0; i < MAXFILES; i++) {
        G_free(inrast[i]);
        Rast_close(infd[i]);
        G_free(outrast[i]);
        Rast_close(outfd[i]);
    }
    exit(EXIT_SUCCESS);
}
