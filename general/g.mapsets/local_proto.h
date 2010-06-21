#ifdef _MAIN_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL char **mapset_name;
GLOBAL int nmapsets;

/* get_maps.c */
int get_available_mapsets(void);

/* list.c */
void list_available_mapsets(const char *);
void list_accessible_mapsets(const char *);
