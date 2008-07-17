#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/nviz.h>

/* module flags and parameters */
struct GParams { 
  struct Flag *mode_all;
  /* raster */
  struct Option *elev_map, *elev_const, *color_map, *color_const,
    *mask_map, *transp_map, *transp_const, *shine_map, *shine_const,
    *emit_map, *emit_const,
  /* draw */
    *mode, *res_fine, *res_coarse, *style, *shade, *wire_color,
  /* vector */
    *vector, *line_width, *line_color, *line_mode, *line_height,
  /* misc */
    *exag, *bgcolor, 
  /* viewpoint */
    *pos, *height, *persp, *twist, 
  /* output */
    *output, *format, *size; 
};

/* args.c */
void parse_command(int, char**, struct GParams *);
int color_from_cmd(const char *);
void opt_get_num_answers(const struct Option *, int *, int *);
void check_parameters(const struct GParams *);

/* surface.c */
int load_rasters(const struct GParams *,
		 nv_data *);
void set_draw_mode(const struct GParams *);

/* vector.c */
int load_vectors(const struct GParams *,
		 nv_data *);
int set_lines_attrb(const struct GParams *);

/* write_img.c */
int write_img(const char *, int);

#endif /* __LOCAL_PROTO_H__ */
