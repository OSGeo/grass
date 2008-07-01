#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/nviz.h>

/* module flags and parameters */
struct GParams { 
  /* raster */
  struct Option *elev_map, *elev_const, *color_map, *color_const,
    *mask_map, *transp_map, *transp_const, *shine_map, *shine_const,
    *emit_map, *emit_const,
  /* vector */
    *vector, 
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

/* write_img.c */
int write_img(const char *, int);

#endif /* __LOCAL_PROTO_H__ */
