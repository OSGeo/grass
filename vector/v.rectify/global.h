#include <grass/gis.h>
#include <grass/imagery.h>


/* georef coefficients */

extern double E12[20], N12[20], Z12[20];
extern double E21[20], N21[20], Z21[20];

/* cp.c */
int get_control_points(char *, char *, int, int, int, char *);

/* env.c */
int select_current_env(void);
int select_target_env(void);
int show_env(void);

/* target.c */
int get_target(char *);
