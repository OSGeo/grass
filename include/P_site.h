
#ifndef GRASS_P_SITE_H
#define GRASS_P_SITE_H

/* The same for old and new, format independent */
Site *G_site_new_struct (RASTER_MAP_TYPE cattype, int ndim, int ns, int nd);
void G_site_free_struct (Site *s);
int G_site_in_region (const Site *s, const struct Cell_head *region);
int G_site_d_cmp (const void *a, const void *b);
int G_site_c_cmp (const void *a, const void *b);
int G_site_s_cmp (const void *a, const void *b);
char *G_site_format (const Site *s, const char *fs, int id);
/* SITE_ATT * G_sites_get_atts (FILE * ptr, int* cat);*/
int G_sites_get_fields (FILE * ptr, char*** cnames, int** ctypes, int** ndx);
void G_sites_free_fields (int ncols, char** cnames, int* ctypes, int* ndx);

/* Old version used by v.in.sites */
FILE * G_oldsites_open_old (const char *name, const char *mapset);
int G_oldsite_describe (FILE *p, int *dims, int *cat, int *strs, int *dbls);
int G_oldsite_get (FILE *p, Site *s);
int G__oldsite_get ( FILE *, Site *, int);

/* New version based on vectors used in old, not updated sites modules */
int G_site_get (FILE *p, Site *s);
int G_site_put (FILE *p, const Site *s);
int G_site_describe (FILE *p, int *dims, int *cat, int *strs, int *dbls);
int G_site_get_head (FILE *p, Site_head *head);
int G_site_put_head (FILE *p, Site_head *head);
FILE * G_sites_open_old (char *name, char *mapset);
FILE * G_sites_open_new (char *name);
void G_sites_close ( FILE * );
char * G_find_sites (char *name, const char *mapset);
char * G_find_sites2 (const char *name, const char *mapset);
char * G_ask_sites_new (const char *prompt, char *name);
char * G_ask_sites_old (const char *prompt, char *name);
char * G_ask_sites_any (const char *prompt, char *name);
char * G_ask_sites_in_mapset (const char *prompt, char *name);
int G__site_put ( FILE *, Site *, int);

FILE *G_fopen_sites_old(char *, char *);
FILE *G_fopen_sites_new(char *);
int G_get_site(FILE *, double *, double *, char **);
int G_put_site(FILE *, double, double, const char *);

#endif
