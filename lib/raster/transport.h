
#ifndef _TRANSPORT_H
#define _TRANSPORT_H

extern void LOC_get_location_with_box(int cx, int cy, int *wx, int *wy,
				      int *button);
extern void LOC_get_location_with_line(int cx, int cy, int *wx, int *wy,
				       int *button);
extern void LOC_get_location_with_pointer(int *wx, int *wy, int *button);
extern int LOC_open_driver(void);
extern int LOC__open_quiet(void);
extern void LOC_stabilize(void);
extern void LOC_kill_driver(void);
extern void LOC_close_driver(void);
extern void LOC_release_driver(void);
extern int LOC_pad_create(const char *pad);
extern int LOC_pad_current(char *name);
extern int LOC_pad_delete(void);
extern int LOC_pad_invent(char *pad);
extern int LOC_pad_list(char ***list, int *count);
extern int LOC_pad_select(const char *pad);
extern int LOC_pad_append_item(const char *item, const char *value,
			       int replace);
extern int LOC_pad_delete_item(const char *name);
extern int LOC_pad_get_item(const char *name, char ***list, int *count);
extern int LOC_pad_list_items(char ***list, int *count);
extern int LOC_pad_set_item(const char *name, const char *value);
extern int LOC_screen_left(void);
extern int LOC_screen_rite(void);
extern int LOC_screen_bot(void);
extern int LOC_screen_top(void);
extern void LOC_get_num_colors(int *n);
extern void LOC_standard_color(int index);
extern void LOC_RGB_color(unsigned char red, unsigned char grn,
			  unsigned char blu);
extern void LOC_line_width(int width);
extern void LOC_erase(void);
extern void LOC_move_abs(int x, int y);
extern void LOC_move_rel(int x, int y);
extern void LOC_cont_abs(int x, int y);
extern void LOC_cont_rel(int x, int y);
extern void LOC_polydots_abs(const int *xarray, const int *yarray,
			     int number);
extern void LOC_polydots_rel(const int *xarray, const int *yarray,
			     int number);
extern void LOC_polyline_abs(const int *xarray, const int *yarray,
			     int number);
extern void LOC_polyline_rel(const int *xarray, const int *yarray,
			     int number);
extern void LOC_polygon_abs(const int *xarray, const int *yarray, int number);
extern void LOC_polygon_rel(const int *xarray, const int *yarray, int number);
extern void LOC_box_abs(int x1, int y1, int x2, int y2);
extern void LOC_box_rel(int x, int y);
extern void LOC_text_size(int width, int height);
extern void LOC_text_rotation(float rotation);
extern void LOC_set_window(int t, int b, int l, int r);
extern void LOC_text(const char *text);
extern void LOC_get_text_box(const char *text, int *t, int *b, int *l,
			     int *r);
extern void LOC_font(const char *name);
extern void LOC_charset(const char *name);
extern void LOC_font_list(char ***list, int *count);
extern void LOC_font_info(char ***list, int *count);
extern void LOC_panel_save(const char *name, int t, int b, int l, int r);
extern void LOC_panel_restore(const char *name);
extern void LOC_panel_delete(const char *name);
extern void LOC_begin_scaled_raster(int mask, int src[2][2], int dst[2][2]);
extern int LOC_scaled_raster(int n, int row, const unsigned char *red,
			     const unsigned char *grn,
			     const unsigned char *blu,
			     const unsigned char *nul);
extern void LOC_end_scaled_raster(void);
extern void LOC_bitmap(int ncols, int nrows, int threshold,
		       const unsigned char *buf);

extern void REM_get_location_with_box(int cx, int cy, int *wx, int *wy,
				      int *button);
extern void REM_get_location_with_line(int cx, int cy, int *wx, int *wy,
				       int *button);
extern void REM_get_location_with_pointer(int *wx, int *wy, int *button);
extern int REM_open_driver(void);
extern int REM__open_quiet(void);
extern void REM_stabilize(void);
extern void REM_kill_driver(void);
extern void REM_close_driver(void);
extern void REM_release_driver(void);
extern int REM_pad_create(const char *pad);
extern int REM_pad_current(char *name);
extern int REM_pad_delete(void);
extern int REM_pad_invent(char *pad);
extern int REM_pad_list(char ***list, int *count);
extern int REM_pad_select(const char *pad);
extern int REM_pad_append_item(const char *item, const char *value,
			       int replace);
extern int REM_pad_delete_item(const char *name);
extern int REM_pad_get_item(const char *name, char ***list, int *count);
extern int REM_pad_list_items(char ***list, int *count);
extern int REM_pad_set_item(const char *name, const char *value);
extern int REM_screen_left(void);
extern int REM_screen_rite(void);
extern int REM_screen_bot(void);
extern int REM_screen_top(void);
extern void REM_get_num_colors(int *n);
extern void REM_standard_color(int index);
extern void REM_RGB_color(unsigned char red, unsigned char grn,
			  unsigned char blu);
extern void REM_line_width(int width);
extern void REM_erase(void);
extern void REM_move_abs(int x, int y);
extern void REM_move_rel(int x, int y);
extern void REM_cont_abs(int x, int y);
extern void REM_cont_rel(int x, int y);
extern void REM_polydots_abs(const int *xarray, const int *yarray,
			     int number);
extern void REM_polydots_rel(const int *xarray, const int *yarray,
			     int number);
extern void REM_polyline_abs(const int *xarray, const int *yarray,
			     int number);
extern void REM_polyline_rel(const int *xarray, const int *yarray,
			     int number);
extern void REM_polygon_abs(const int *xarray, const int *yarray, int number);
extern void REM_polygon_rel(const int *xarray, const int *yarray, int number);
extern void REM_box_abs(int x1, int y1, int x2, int y2);
extern void REM_box_rel(int x, int y);
extern void REM_text_size(int width, int height);
extern void REM_text_rotation(float rotation);
extern void REM_set_window(int t, int b, int l, int r);
extern void REM_text(const char *text);
extern void REM_get_text_box(const char *text, int *t, int *b, int *l,
			     int *r);
extern void REM_font(const char *name);
extern void REM_charset(const char *name);
extern void REM_font_list(char ***list, int *count);
extern void REM_font_info(char ***list, int *count);
extern void REM_panel_save(const char *name, int t, int b, int l, int r);
extern void REM_panel_restore(const char *name);
extern void REM_panel_delete(const char *name);
extern void REM_begin_scaled_raster(int mask, int src[2][2], int dst[2][2]);
extern int REM_scaled_raster(int n, int row, const unsigned char *red,
			     const unsigned char *grn,
			     const unsigned char *blu,
			     const unsigned char *nul);
extern void REM_end_scaled_raster(void);
extern void REM_bitmap(int ncols, int nrows, int threshold,
		       const unsigned char *buf);

struct transport
{
    int (*open_driver) (void);
    int (*open_quiet) (void);
    void (*stabilize) (void);
    void (*kill_driver) (void);
    void (*close_driver) (void);
    void (*release_driver) (void);

    int (*screen_left) (void);
    int (*screen_rite) (void);
    int (*screen_bot) (void);
    int (*screen_top) (void);
    void (*get_num_colors) (int *);
    void (*standard_color) (int);
    void (*RGB_color) (unsigned char, unsigned char, unsigned char);
    void (*line_width) (int);
    void (*erase) (void);
    void (*move_abs) (int, int);
    void (*move_rel) (int, int);
    void (*cont_abs) (int, int);
    void (*cont_rel) (int, int);
    void (*polydots_abs) (const int *, const int *, int);
    void (*polydots_rel) (const int *, const int *, int);
    void (*polyline_abs) (const int *, const int *, int);
    void (*polyline_rel) (const int *, const int *, int);
    void (*polygon_abs) (const int *, const int *, int);
    void (*polygon_rel) (const int *, const int *, int);
    void (*box_abs) (int, int, int, int);
    void (*box_rel) (int, int);
    void (*text_size) (int, int);
    void (*text_rotation) (float);
    void (*set_window) (int, int, int, int);
    void (*text) (const char *);
    void (*get_text_box) (const char *, int *, int *, int *, int *);
    void (*font) (const char *);
    void (*charset) (const char *);
    void (*font_list) (char ***, int *);
    void (*font_info) (char ***, int *);
    void (*panel_save) (const char *, int, int, int, int);
    void (*panel_restore) (const char *);
    void (*panel_delete) (const char *);
    void (*begin_scaled_raster) (int, int[2][2], int[2][2]);
    int (*scaled_raster) (int, int, const unsigned char *,
			  const unsigned char *, const unsigned char *,
			  const unsigned char *);
    void (*end_scaled_raster) (void);
    void (*bitmap) (int, int, int, const unsigned char *);

    void (*get_location_with_box) (int, int, int *, int *, int *);
    void (*get_location_with_line) (int, int, int *, int *, int *);
    void (*get_location_with_pointer) (int *, int *, int *);

    int (*pad_create) (const char *);
    int (*pad_current) (char *);
    int (*pad_delete) (void);
    int (*pad_invent) (char *);
    int (*pad_list) (char ***, int *);
    int (*pad_select) (const char *);
    int (*pad_append_item) (const char *, const char *, int);
    int (*pad_delete_item) (const char *);
    int (*pad_get_item) (const char *, char ***, int *);
    int (*pad_list_items) (char ***, int *);
    int (*pad_set_item) (const char *, const char *);
};

extern const struct transport *trans;

#endif /* _TRANSPORT_H */
