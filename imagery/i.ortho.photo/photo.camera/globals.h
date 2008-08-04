#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif
/* State flags  Most are Toggles */
GLOBAL char Data_Loaded;	/* is there data in memory */

				/* used for abnormal exit logic */
GLOBAL char Files_Open;		/* is there data in memory */

GLOBAL struct Ortho_Camera_File_Ref cam_info;

GLOBAL char *camera;

/* hold the names of files etc.  mostly used by main.c */
GLOBAL char *N_path;
GLOBAL char *N_name;
GLOBAL char *N_camera;

/* mod_cam_info.c */
int mod_cam_info(int, struct Ortho_Camera_File_Ref *);
