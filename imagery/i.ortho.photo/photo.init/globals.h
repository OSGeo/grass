#include "orthophoto.h"

/* State flags  Most are Toggles */
extern  char Data_Loaded;	/* is there data in memory */

				/* used for abnormal exit logic */
extern  char Files_Open;		/* is there data in memory */

extern  struct Ortho_Image_Group group;

/* hold the names of files etc.  mostly used by main.c */
extern  char *N_path;
extern  char *N_name;
extern  char *N_camera;

#define DEG_TO_RADS 0.01745329
#define RAD_TO_DEGS 57.29578

/* mod_info.c */
int mod_init_info(int, struct Ortho_Camera_Exp_Init *);
