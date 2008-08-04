#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

#include "orthophoto.h"

/* State flags  Most are Toggles */
GLOBAL char Data_Loaded;	/* is there data in memory */

				/* used for abnormal exit logic */
GLOBAL char Files_Open;		/* is there data in memory */

GLOBAL struct Ortho_Image_Group group;

/* hold the names of files etc.  mostly used by main.c */
GLOBAL char *N_path;
GLOBAL char *N_name;
GLOBAL char *N_camera;

#define DEG_TO_RADS 0.01745329
#define RAD_TO_DEGS 57.29578

/* mod_info.c */
int mod_init_info(int, struct Ortho_Camera_Exp_Init *);
