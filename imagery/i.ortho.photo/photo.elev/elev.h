extern char *elev_layer;
extern char *mapset_elev;
extern char *tl;
extern char *math_exp;
extern char *units;
extern char *nd;

/* ask_elev.c */
int ask_elev(char *, char *, char *);

/* main.c */
int select_current_env(void);
int select_target_env(void);

/* mod_elev.c */
int mod_elev_data(void);
