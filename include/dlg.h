struct dlg_node
{
    double x;
    double y;
    int n_lines;
    int n_atts;
    int n_lines_alloc;
    int n_atts_alloc;
    int *lines;
    int *atts;
};

struct dlg_area
{
    double x;
    double y;
    int n_lines;
    int n_atts;
    int n_isles;
    int n_lines_alloc;
    int n_atts_alloc;
    int *lines;
    int *atts;
};

struct dlg_line
{
    int start_node;
    int end_node;
    int left_area;
    int right_area;
    int n_coors;
    int n_atts;
    int n_coors_alloc;
    int n_atts_alloc;
    int *atts;
    double *coors;
    double N;
    double S;
    double E;
    double W;
};

struct dlg_head
{
    int nlines;
    char banner[73];
    char cart_unit[41];
    char source_date[11];
    char orig_scale[9];
    char line_3[73];
    int level_code;
    int plani_code;
    int plani_zone;
    int plani_units;
    double resolution;
    int trans_param;
    int misc_records;
    int num_sides;
    int num_cats;
};

struct dlg_coors
{
    double lat[4];
    double lon[4];
    double utm_n[4];
    double utm_e[4];
};

struct dlg_proj
{
    double params[15];
    double int_params[4];
};

struct dlg_cats
{
    int read;
    char name[21];
    int form_code;
    int num_nodes;
    int act_nodes;
    int nta_link;
    int ntl_link;
    int num_areas;
    int act_areas;
    int atn_link;
    int atl_link;
    int area_list;
    int num_lines;
    int act_lines;
    int line_list;
};

struct dlg
{
    struct dlg_head head;
    struct dlg_cats cats;
    struct dlg_coors coors;
    struct dlg_proj proj;
    struct dlg_line line;
    struct dlg_area area;
    struct dlg_node node;
    long *node_off;
    long *area_off;
    long *line_off;
    int node_alloc;
    int area_alloc;
    int line_alloc;
    int max_nodes;
    int max_areas;
    int max_lines;
};

#define SW	0
#define NW	1
#define NE	2
#define SE	3

/*  this will be stored as a double on mass, sun and 3b2's */
#define ISLAND_MARKER	-99999999.

/* Need a definition for FILE   */
#include <stdio.h>

int dlg_init(FILE *, struct dlg *);
int dlg_read(FILE *, struct dlg *);
int _dlg_read_node(struct dlg_node *, FILE *);
int _dlg_read_area(struct dlg_area *, FILE *);
int _dlg_read_line(struct dlg_line *, FILE *);
int _dlg_write_area(struct dlg_area *, FILE *);
int dlg_write_header(FILE *, struct dlg *);
int _dlg_write_line(struct dlg_line *, FILE *);
int _dlg_write_node(struct dlg_node *, FILE *);
int dlg_read_whole_area(FILE *, struct dlg *, int, double **, double **,
			int *, int *);
int dlg_read_area(FILE *, struct dlg *, int);
int dlg_read_line(FILE *, struct dlg *, int);
int dlg_read_node(FILE *, struct dlg *, int);
int dlg_read_int(FILE *, int, int **);
int dlg_write_int(FILE *, int, int *);
int dlg_write_double(FILE *, int, double *);
int dlg_write_area(FILE *, struct dlg *, int);
int dlg_write_line(FILE *, struct dlg *, int);
int dlg_write_node(FILE *, struct dlg *, int);
