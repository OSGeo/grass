
#include <stdio.h>

#define DEF_MINBBOX  2
#define DEF_MINDIST  2
#define DEF_MAXPTS  99

#define FILE_NAME  "htmlmap"

#define INITIAL_TEXT 1000

#define APACHE 0		/* write output in apache/ncsa server image map format */
#define NCSA   0		/* write output in apache/ncsa server image map format */
#define CLIENT 1		/* write output in netscape client side image map format */
#define RAW    2		/* write output in raw format */

extern char *last_text;
extern int last_text_len;
extern char *file_name;
extern int html_type;
extern FILE *output;

struct MapPoly
{
    char *url;
    int num_pts;
    int *x_pts;
    int *y_pts;
    struct MapPoly *next_poly;
};

extern struct MapPoly *head;
extern struct MapPoly **tail;

/* Graph_Clse.c */
extern void HTML_Graph_close(void);

/* Graph_Set.c */
extern int HTML_Graph_set(int, char **);

/* Polygn_abs.c */
extern void HTML_Polygon_abs(const int *, const int *, int);

/* Text.c */
extern void HTML_Text(const char *);
