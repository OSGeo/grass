/* chk_dbase.c */
int can_make_project(char *, char *);

/* clean_temp.c */
int find_process(int);

/* set_data.c */
int list_projects(const char *);
int list_subprojects(const char *, const char *);
int first_word(char *);
int hit_return(void);

/* lock.c */
int find_process(int);

/* mke_loc.c */
int make_project(const char *, const char *);

/* mke_subproject.c */
int make_subproject(const char *, const char *);

/* other.c */
int subproject_permissions(const char *);
int subproject_message(const char *);
int subproject_question(const char *);
int printfile(const char *);
