/* chk_dbase.c */
int can_make_location(char *, char *);

/* clean_temp.c */
int find_process(int);

/* set_data.c */
int list_locations(const char *);
int list_mapsets(const char *, const char *);
int first_word(char *);
int hit_return(void);

/* lock.c */
int find_process(int);

/* mke_loc.c */
int make_location(const char *, const char *);

/* mke_mapset.c */
int make_mapset(const char *, const char *);

/* other.c */
int mapset_permissions(const char *);
int mapset_message(const char *);
int mapset_question(const char *);
int printfile(const char *);
