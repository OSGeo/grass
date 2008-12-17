
#include <stdio.h>
#include "path.h"

#define DEF_MINBBOX  2
#define DEF_MINDIST  2
#define DEF_MAXPTS  99

#define FILE_NAME  "htmlmap"

#define INITIAL_TEXT 1000

#define APACHE 0		/* write output in apache/ncsa server image map format */
#define NCSA   0		/* write output in apache/ncsa server image map format */
#define CLIENT 1		/* write output in netscape client side image map format */
#define RAW    2		/* write output in raw format */

struct MapPoly
{
    char *url;
    int num_pts;
    int *x_pts;
    int *y_pts;
    struct MapPoly *next_poly;
};

struct html_state
{
    char *last_text;
    int last_text_len;
    int type;
    FILE *output;
    struct MapPoly *head;
    struct MapPoly **tail;
    int MAX_POINTS;
    int BBOX_MINIMUM;
    int MINIMUM_DIST;
};

extern struct html_state html;

/* Draw.c */
extern void HTML_Begin(void);
extern void HTML_Move(double, double);
extern void HTML_Cont(double, double);
extern void HTML_Close(void);
extern void HTML_Fill(void);
extern void HTML_Stroke(void);

/* Driver.c */
extern const struct driver *HTML_Driver(void);

/* Graph_Clse.c */
extern void HTML_Graph_close(void);

/* Graph_Set.c */
extern int HTML_Graph_set(void);

/* Box.c */
extern void HTML_Box(double, double, double, double);

/* Polygon.c */
extern void html_polygon(const struct path *);

/* Text.c */
extern void HTML_Text(const char *);
