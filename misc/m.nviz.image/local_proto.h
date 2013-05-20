#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/nviz.h>

/* module flags and parameters */
struct GParams
{
    struct Flag *mode_all;

    struct Option 
    /* surface */
    *elev_map, *elev_const, *color_map, *color_const,
	*mask_map, *transp_map, *transp_const, *shine_map, *shine_const,
	*emit_map, *emit_const,
    /* surface draw mode */
	*mode, *res_fine, *res_coarse, *style, *shade, *wire_color, *surface_pos,
    /* vector lines */
	*vlines, *vline_width, *vline_color, *vline_mode, *vline_height, *vline_pos,
	*vline_layer, *vline_color_column, *vline_width_column,
    /* vector points */
	*vpoints, *vpoint_size, *vpoint_marker, *vpoint_color, *vpoint_width, *vpoint_pos,
	*vpoint_layer, *vpoint_size_column, *vpoint_marker_column, *vpoint_color_column, *vpoint_width_column,
    /* volumes */
	*volume, *volume_mode, *volume_shade, *volume_pos, *volume_res, *isosurf_level,
	*isosurf_color_map, *isosurf_color_const, *isosurf_transp_map, *isosurf_transp_const,
	*isosurf_shine_map, *isosurf_shine_const, *slice_pos, *slice, *slice_transp,
    /* misc */
	*exag, *bgcolor,
    /* cutting planes */
       *cplane, *cplane_pos, *cplane_rot, *cplane_tilt, *cplane_shading,
    /* viewpoint */
	*pos, *height, *persp, *twist, *focus,
    /* output */
	*output, *format, *size,
    /* lighting */
	*light_pos, *light_color, *light_bright, *light_ambient,
    /* fringe */
	*fringe, *fringe_color, *fringe_elev,
    /* north arrow */
	*north_arrow, *north_arrow_size, *north_arrow_color;
};

/* args.c */
void parse_command(int, char **, struct GParams *);
int color_from_cmd(const char *);
int opt_get_num_answers(const struct Option *);
void check_parameters(const struct GParams *);

/* surface.c */
int load_rasters(const struct GParams *, nv_data *);
void surface_set_draw_mode(const struct GParams *);

/* vector.c */
int load_vlines(const struct GParams *, nv_data *);
int load_vpoints(const struct GParams *, nv_data *);
int vlines_set_attrb(const struct GParams *);
int vpoints_set_attrb(const struct GParams *);
int check_map(const struct GParams *, int, int, int *, int *);

/* volume.c */
int load_rasters3d(const struct GParams *, nv_data *);
int add_isosurfs(const struct GParams *, nv_data *);
int add_slices(const struct GParams *, nv_data *);

/* cutting planes */
void draw_cplane(const struct GParams *, nv_data *);

/* write_img.c */
int write_img(const char *, int);

#endif /* __LOCAL_PROTO_H__ */
