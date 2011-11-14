#ifndef GRASS_SITEDEFS_H
#define GRASS_SITEDEFS_H

/* Old version used by v.in.sites */
FILE *G_oldsites_open_old(const char *, const char *);
int G_oldsite_describe(FILE *, int *, int *, int *, int *);
int G_oldsite_get(FILE *, Site *);

/*!
  \todo Update modules which are still using sites library. 
  After that all the function below could be removed
*/
/* The same for old and new, format independent */
Site *G_site_new_struct(RASTER_MAP_TYPE, int, int, int);
void G_site_free_struct(Site *);
int G_site_in_region(const Site *, const struct Cell_head *);
/* New version based on vectors used in old, not updated sites modules */
int G_site_get(struct Map_info *, Site *);
int G_site_put(struct Map_info *, const Site *);
int G_site_describe(struct Map_info *, int *, int *, int *,
		    int *);
int G_site_put_head(struct Map_info *, Site_head *);
struct Map_info *G_sites_open_old(const char *, const char *);
struct Map_info *G_sites_open_new(const char *);

struct Map_info *G_fopen_sites_old(const char *, const char *);
struct Map_info *G_fopen_sites_new(const char *);

#endif
