#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* calc_linefax.c */
int viz_calc_tvals(cmndln_info * linefax, char **a_levels, char *a_min,
		   char *a_max, char *a_step, char *a_tnum, int quiet);
/* fill_fax.c */
void fill_cfax(Cube_data * Cube, int flag, int index,
	       float TEMP_VERT[13][3], float TEMP_NORM[13][3]);
/* iso_surface.c */
void viz_iso_surface(void *g3map, RASTER3D_Region * g3reg,
		     cmndln_info * linefax, int quiet);
/* make_header.c */
void viz_make_header(file_info * hf, double dmin, double dmax,
		     RASTER3D_Region * g3reg);
/* r3_data.c */
int r3read_level(void *g3map, RASTER3D_Region * g3reg, file_info * Headfax,
		 float *data, int n_lev);
/* r3_find.c */
int g3_find_dsp_file(const char *cell, const char *file, const char *mset);
const char *check_get_any_dspname(const char *dspf, const char *g3f, const char *mset);

#endif
