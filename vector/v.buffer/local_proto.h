#if (GEOS_VERSION_MAJOR >= 3 && GEOS_VERSION_MINOR >= 3)
#define GEOS_3_3
#endif

struct buf_contours
{
    int inner_count;
    int outer;
    int *inner;
};

struct buf_contours_pts
{
    int inner_count;
    struct line_pnts *oPoints;
    struct line_pnts **iPoints;
};

#ifdef HAVE_GEOS
int geos_buffer(struct Map_info *, struct Map_info *,
                struct Map_info *, int, int, double,
		struct spatial_index *,
		struct line_cats *,
		struct buf_contours **,
		int *, int *, int, int);
#endif

