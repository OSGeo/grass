#ifndef GRASS_SITEDEFS_H
#define GRASS_SITEDEFS_H

/* Allocate 'num' SITE_XYZ structs. Returns NULL on failure */
SITE_XYZ *G_alloc_site_xyz(size_t);

/* Free the array of SITE_XYZ struct */
void G_free_site_xyz(SITE_XYZ *);

/* G_readsites_xyz: Reads a sites file converting to a site struct of xyz
 * values and the cat value.  The Z value can come from one of the
 * n-dimensions, a double attribute, or a string attribute converted to a
 * double with strtod().  The 'size' must not be greater than the number
 * of elements in the SITE_XYZ array, or bad things will happen. The number 
 * of records read is returned or EOF on end of file. NOTE: EOF won't be
 * returned unless no records are read and the EOF bit is set. It's safe
 * to assume that if the number of records read is less than the size of
 * the array, that there aren't any more records.
 */
int G_readsites_xyz(FILE *,	/* The FILE stream to the sites file               */
		    int,	/* Attribute type: SITE_COL_DIM, etc...            */
		    int,	/* The field index (1 based) for the attribute     */
		    int,	/* Size of the array                               */
		    struct Cell_head *,	/* Respect region if not NULL */
		    SITE_XYZ * xyz	/* The site array of size 'size'                   */
    );

int G_readsites(FILE *, int, int, int, struct Cell_head *, Z **);

/* The same for old and new, format independent */
Site *G_site_new_struct(RASTER_MAP_TYPE cattype, int ndim, int ns, int nd);
void G_site_free_struct(Site * s);
int G_site_in_region(const Site * s, const struct Cell_head *region);
int G_site_d_cmp(const void *a, const void *b);
int G_site_c_cmp(const void *a, const void *b);
int G_site_s_cmp(const void *a, const void *b);
char *G_site_format(const Site * s, const char *fs, int id);

/* struct site_att * G_sites_get_atts (struct Map_info * ptr, int* cat); */
int G_sites_get_fields(struct Map_info *ptr, char ***cnames, int **ctypes,
		       int **ndx);
void G_sites_free_fields(int ncols, char **cnames, int *ctypes, int *ndx);

/* Old version used by v.in.sites */
FILE *G_oldsites_open_old(const char *name, const char *mapset);
int G_oldsite_describe(FILE * p, int *dims, int *cat, int *strs, int *dbls);
int G_oldsite_get(FILE * p, Site * s);
int G__oldsite_get(FILE *, Site *, int);

/* New version based on vectors used in old, not updated sites modules */
int G_site_get(struct Map_info *p, Site * s);
int G_site_put(struct Map_info *p, const Site * s);
int G_site_describe(struct Map_info *p, int *dims, int *cat, int *strs,
		    int *dbls);
int G_site_get_head(struct Map_info *p, Site_head * head);
int G_site_put_head(struct Map_info *p, Site_head * head);
struct Map_info *G_sites_open_old(const char *name, const char *mapset);
struct Map_info *G_sites_open_new(const char *name);
void G_sites_close(struct Map_info *);
const char *G_find_sites(char *name, const char *mapset);
const char *G_find_sites2(const char *name, const char *mapset);
int G__site_put(struct Map_info *, Site *, int);

struct Map_info *G_fopen_sites_old(const char *, const char *);
struct Map_info *G_fopen_sites_new(const char *);
int G_get_site(struct Map_info *, double *, double *, char **);
int G_put_site(struct Map_info *, double, double, const char *);

#endif
