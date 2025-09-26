#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/parson.h>

#ifndef _LOCAL_PROTO_
#define _LOCAL_PROTO_

/* define codes for characteristics of relation between two nearest features */
enum Code {
    CAT = 1,        // category of nearest feature
    FROM_X = 2,     // x coordinate of nearest point on 'from' feature
    FROM_Y = 3,     // y coordinate of nearest point on 'from' feature
    TO_X = 4,       // x coordinate of nearest point on 'to' feature
    TO_Y = 5,       // y coordinate of nearest point on 'to' feature
    FROM_ALONG = 6, // distance to nearest point on 'from' along linear feature
    TO_ALONG = 7,   // distance to nearest point on 'to' along linear feature
    DIST = 8,       // minimum distance to nearest feature
    TO_ANGLE = 9,   // angle of linear feature in nearest point
    TO_ATTR = 10,   // attribute of nearest feature
    END = 11        // end of list
};

enum OutputFormat { PLAIN, JSON };

/* Structure to store info about nearest feature for each category */
typedef struct {
    int from_cat;                  // category (from)
    int count;                     // number of features already found
    int to_cat;                    // category (to)
    double from_x, from_y, from_z; // coordinates of nearest 'from' point
    double to_x, to_y, to_z;       // coordinates of nearest 'to' point
    double from_along,
        to_along;      // distance along a linear feature to the nearest point
    double from_angle; // angle of linear feature in nearest point
    double to_angle;   // angle of linear feature in nearest point
    double dist;       // distance to nearest feature
} NEAR;

/* Upload and column store */
typedef struct {
    enum Code upload; // code
    char *column;     // column name
} UPLOAD;

typedef int dist_func(const struct line_pnts *, double, double, double, int,
                      double *, double *, double *, double *, double *,
                      double *);
extern dist_func *line_distance;

/* cmp.c */
int cmp_near(const void *, const void *);
int cmp_near_to(const void *, const void *);
int cmp_exist(const void *, const void *);

/* distance.c */
int get_line_box(const struct line_pnts *Points, struct bound_box *box);
int line2line(struct line_pnts *FPoints, int ftype, struct line_pnts *TPoints,
              int ttype, double *fx, double *fy, double *fz, double *falong,
              double *fangle, double *tx, double *ty, double *tz,
              double *talong, double *tangle, double *dist, int with_z);
int line2area(struct Map_info *To, struct line_pnts *Points, int type, int area,
              const struct bound_box *abox, double *fx, double *fy, double *fz,
              double *falong, double *fangle, double *tx, double *ty,
              double *tz, double *talong, double *tangle, double *dist,
              int with_z);

/* print.c */
int print_upload(NEAR *, UPLOAD *, int, dbCatValArray *, dbCatVal *, char *,
                 enum OutputFormat, JSON_Object *);

#endif
