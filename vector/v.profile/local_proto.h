/* main.c */
typedef struct
{
    double distance;
    int cat;
    double z;
} Result;

/* Here are stored all profile line matching points */
extern Result *resultset;

/* processors.c */
/* Add point to result data set */
void add_point(const int, const double, const double, size_t *, const int);

/* Check if point is on profile line (inside buffer) and calculate distance to it */
void proc_point(struct line_pnts *, struct line_pnts *,
                struct line_pnts *, const int, size_t *, const int);

/* Process all two line intersection points */
void proc_line(struct line_pnts *, struct line_pnts *,
               const int, size_t *, const int);
