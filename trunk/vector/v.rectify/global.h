
/* georef coefficients */

extern double E12[20], N12[20], Z12[20];
extern double E21[20], N21[20], Z21[20];
extern double HG12[20], HG21[20], HQ12[20], HQ21[20];
extern double OR12[20], OR21[20];

/* cp.c */
int get_control_points(char *, char *, int, int, int, int, char *, FILE *);

/* env.c */
int select_current_env(void);
int select_target_env(void);
int show_env(void);

/* target.c */
int get_target(char *);
