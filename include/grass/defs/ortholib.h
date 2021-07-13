#ifndef GRASS_ORTHODEFS_H
#define GRASS_ORTHODEFS_H

/* from imagery/libes */
/* ask_camera.c */
int I_ask_camera_old(char *, char *);
int I_ask_camera_new(char *, char *);
int I_ask_camera_any(char *, char *);

/* ask_initial.c */
int I_ask_camera_old(char *, char *);
int I_ask_camera_new(char *, char *);
int I_ask_camera_any(char *, char *);

/* cam.c */
FILE *I_fopen_group_camera_new(char *);
FILE *I_fopen_group_camera_old(char *);

/* camera.c */
int I_put_group_camera(char *, char *);
int I_get_group_camera(char *, char *);

/* elev.c */
int I_put_group_elev(char *, char *, char *, char *, char *, char *, char *);
int I_get_group_elev(char *, char *, char *, char *, char *, char *, char *);

/* find_camera.c */
int I_find_camera(char *);
int I_find_camera_file(char *, char *);

/* find_init.c */
int I_find_initial(char *);

/* fopen_camera.c */
FILE *I_fopen_cam_file_new(char *);
FILE *I_fopen_cam_file_append(char *);
FILE *I_fopen_cam_file_old(char *);

/* group_elev.c */
FILE *I_fopen_group_elev_new(char *);
FILE *I_fopen_group_elev_old(char *);
int I_find_group_elev_file(char *);

/* init.c */
FILE *I_fopen_group_init_new(char *);
FILE *I_fopen_group_init_old(char *);

/* initial.c */
int I_get_camera(char *);
int I_put_camera(char *);
int I_put_group_camera(char *, char *);
int I_get_group_camera(char *, char *);

/* ls_cameras.c */
int I_list_cameras(int);

/* ls_elev.c */
int I_list_elev(int);

/* open_camera.c */
int I_open_cam_file_new(char *, char *);
int I_open_cam_file_old(char *, char *);

/* title_camera.c */
int I_get_cam_title(char *, char *, int);
int I_put_camera_title(char *, char *);

/* vask_block.c */
int I_vask_block_new(char **, char *, char *);
int I_vask_block_old(char **, char *, char *);
int I_vask_subblock_new(char **, char *, char *, int, char *);

#endif
