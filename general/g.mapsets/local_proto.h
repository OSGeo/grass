#ifdef _MAIN_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL char **mapset_name;
GLOBAL int nmapsets;

/* dsply_maps.c */
int display_available_mapsets(const char *);

/* dsply_path.c */
int display_mapset_path(const char *);

/* get_maps.c */
int get_available_mapsets(void);

/* get_path.c */
int get_mapset_path(void);
int delete_choices(void);

/* main_cmd.c */
int main(int, char *[]);

/* main_inter.c */
int main(int, char **);

/* scan_int.c */
int scan_int(char *, int *);

/* set_path.c */
int set_mapset_path(void);
