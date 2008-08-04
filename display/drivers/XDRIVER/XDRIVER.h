#include "driver.h"

extern void XD_Box_abs(int, int, int, int);
extern void XD_Client_Close(void);
extern void XD_Client_Open(void);
extern void XD_Do_work(int);
extern int XD_Get_location_with_box(int, int, int *, int *, int *);
extern int XD_Get_location_with_line(int, int, int *, int *, int *);
extern int XD_Get_location_with_pointer(int *, int *, int *);
extern void XD_Graph_close(void);
extern int XD_Graph_set(int, char **);
extern void XD_Line_width(int);
extern void XD_Panel_delete(const char *);
extern void XD_Panel_restore(const char *);
extern void XD_Panel_save(const char *, int, int, int, int);
extern void XD_Polydots_abs(const int *, const int *, int);
extern void XD_Polydots_rel(const int *, const int *, int);
extern void XD_Polygon_abs(const int *, const int *, int);
extern void XD_Polygon_rel(const int *, const int *, int);
extern void XD_Polyline_abs(const int *, const int *, int);
extern void XD_Polyline_rel(const int *, const int *, int);
extern void XD_Set_window(int, int, int, int);
extern void XD_begin_scaled_raster(int, int[2][2], int[2][2]);
extern int XD_scaled_raster(int, int, const unsigned char *,
			    const unsigned char *, const unsigned char *,
			    const unsigned char *);
extern void XD_Respond(void);
extern int XD_Work_stream(void);
extern void XD_color(int);
extern void XD_draw_bitmap(int, int, int, const unsigned char *);
extern void XD_draw_line(int, int, int, int);
extern void XD_draw_point(int, int);
extern int XD_lookup_color(int, int, int);
