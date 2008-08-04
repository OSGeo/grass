
#ifndef GRASS_P_SITE_H
#define GRASS_P_SITE_H

struct Map_info;

/* The same for old and new, format independent */
Site *G_site_new_struct(RASTER_MAP_TYPE cattype, int ndim, int ns, int nd);
void G_site_free_struct(Site * s);
int G_site_in_region(const Site * s, const struct Cell_head *region);
int G_site_d_cmp(const void *a, const void *b);
int G_site_c_cmp(const void *a, const void *b);
int G_site_s_cmp(const void *a, const void *b);
char *G_site_format(const Site * s, const char *fs, int id);

/* SITE_ATT * G_sites_get_atts (struct Map_info * ptr, int* cat); */
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
char *G_find_sites(char *name, const char *mapset);
char *G_find_sites2(const char *name, const char *mapset);
char *G_ask_sites_new(const char *prompt, char *name);
char *G_ask_sites_old(const char *prompt, char *name);
char *G_ask_sites_any(const char *prompt, char *name);
char *G_ask_sites_in_mapset(const char *prompt, char *name);
int G__site_put(struct Map_info *, Site *, int);

struct Map_info *G_fopen_sites_old(const char *, const char *);
struct Map_info *G_fopen_sites_new(const char *);
int G_get_site(struct Map_info *, double *, double *, char **);
int G_put_site(struct Map_info *, double, double, const char *);

#endif
