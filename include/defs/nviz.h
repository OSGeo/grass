#ifndef GRASS_NVIZDEFS_H
#define GRASS_NVIZDEFS_H

/* change_view.c */
int Nviz_resize_window(int, int);
int Nviz_update_ranges(nv_data *);
int Nviz_set_viewpoint_position(double, double);
void Nviz_get_viewpoint_position(double *, double *);
int Nviz_set_viewpoint_height(double);
void Nviz_get_viewpoint_height(double *);
int Nviz_set_viewpoint_persp(int);
int Nviz_set_viewpoint_twist(int);
int Nviz_change_exag(nv_data *, double);
int Nviz_look_here(double, double);
void Nviz_get_modelview(double *);
void Nviz_set_rotation(double, double, double, double);
void Nviz_unset_rotation(void);
void Nviz_init_rotation(void);
void Nviz_flythrough(nv_data *, float *, int *, int);

/* cplanes_obj.c */
int Nviz_new_cplane(nv_data *, int);
int Nviz_on_cplane(nv_data *, int);
int Nviz_off_cplane(nv_data *, int);
int Nviz_draw_cplane(nv_data *, int, int);
int Nviz_num_cplanes(nv_data *);
int Nviz_get_current_cplane(nv_data *);
int Nviz_set_cplane_rotation(nv_data *, int, float, float, float);
int Nviz_get_cplane_rotation(nv_data *, int, float *, float *, float *);
int Nviz_set_cplane_translation(nv_data *, int, float, float, float);
int Nviz_get_cplane_translation(nv_data *, int, float *, float *, float *);
int Nviz_set_fence_color(nv_data *, int);
int Nviz_set_cplane_here(nv_data *, int, float, float);


/* draw.c */
int Nviz_draw_all_surf(nv_data *);
int Nviz_draw_all_vect();
int Nviz_draw_all_site();
int Nviz_draw_all_vol();
int Nviz_draw_all(nv_data *);
int Nviz_draw_quick(nv_data *, int);
int Nviz_load_image(GLubyte *, int, int, int);
void Nviz_draw_image(int, int, int, int, int);
void Nviz_set_2D(int, int);
void Nviz_del_texture(int);
void Nviz_get_max_texture(int *);

/* exag.c */
int Nviz_get_exag_height(double *, double *, double *);
double Nviz_get_exag();

/* lights.c */
int Nviz_set_light_position(nv_data *, int, double, double, double, double);
int Nviz_set_light_bright(nv_data *, int, double);
int Nviz_set_light_color(nv_data *, int, int, int, int);
int Nviz_set_light_ambient(nv_data *, int, double);
int Nviz_init_light(nv_data *, int);
int Nviz_new_light(nv_data *);
void Nviz_draw_model(nv_data *);

/* map_obj.c */
int Nviz_new_map_obj(int, const char *, double, nv_data *);
int Nviz_set_attr(int, int, int, int, const char *, double, nv_data *);
void Nviz_set_surface_attr_default();
int Nviz_set_vpoint_attr_default();
int Nviz_set_volume_attr_default();
int Nviz_unset_attr(int, int, int);

/* nviz.c */
void Nviz_init_data(nv_data *);
void Nviz_destroy_data(nv_data *);
void Nviz_set_bgcolor(nv_data *, int);
int Nviz_get_bgcolor(nv_data *);
int Nviz_color_from_str(const char *);
struct fringe_data *Nviz_new_fringe(nv_data *, int, unsigned long,
				    double, int, int, int, int);
struct fringe_data *Nviz_set_fringe(nv_data *, int, unsigned long,
				    double, int, int, int, int);
void Nviz_draw_fringe(nv_data *data);
int Nviz_draw_arrow(nv_data *);
int Nviz_set_arrow(nv_data *, int, int, float, unsigned int);
void Nviz_delete_arrow(nv_data *);
struct scalebar_data * Nviz_new_scalebar(nv_data *, int, float *, float, unsigned int);
struct scalebar_data * Nviz_set_scalebar(nv_data *, int , int, int, float, unsigned int);
void Nviz_draw_scalebar(nv_data *);
void Nviz_delete_scalebar(nv_data *, int);

/* position.c */
void Nviz_init_view(nv_data *);
int Nviz_set_focus_state(int);
int Nviz_set_focus_map(int, int);
int Nviz_has_focus(nv_data *);
int Nviz_set_focus(nv_data *, float, float, float);
int Nviz_get_focus(nv_data *, float *, float *, float *);
float Nviz_get_xyrange(nv_data *);
int Nviz_get_zrange(nv_data *, float *, float *);
float Nviz_get_longdim(nv_data *);

/* render.c */
struct render_window *Nviz_new_render_window();
void Nviz_init_render_window(struct render_window *);
void Nviz_destroy_render_window(struct render_window *);
int Nviz_create_render_window(struct render_window *, void *, int, int);
int Nviz_make_current_render_window(const struct render_window *);

#endif /* GRASS_NVIZDEFS_H */
