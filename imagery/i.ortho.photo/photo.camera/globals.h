/* State flags  Most are Toggles */
extern char Data_Loaded;	/* is there data in memory */

				/* used for abnormal exit logic */
extern char Files_Open;		/* is there data in memory */

extern struct Ortho_Camera_File_Ref cam_info;

extern char *camera;

/* hold the names of files etc.  mostly used by main.c */
extern char *N_path;
extern char *N_name;
extern char *N_camera;

/* mod_cam_info.c */
int mod_cam_info(int, struct Ortho_Camera_File_Ref *);
