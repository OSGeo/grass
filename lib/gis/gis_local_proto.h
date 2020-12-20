#ifndef __GIS_LOCAL_PROTO_H__
#define __GIS_LOCAL_PROTO_H__

/* subroutines used only by GIS Library */

/* env.c */
void G__read_env(void);
void G__write_env(void);

/* gisinit.c */
void G__check_gisinit(void);

/* handler.c */
void G__call_error_handlers(void);

/* home.c */
const char *G__home(void);

/* project.c */
char *G__project_path(void);

/* mach_name.c */
const char *G__machine_name(void);

/* subproject.c */
const char *G__subproject(void);
char *G__subproject_path(void);

/* subproject_nme.c */
void G__get_list_of_subprojects(void);

/* timestamp.c */
int G__read_timestamp(FILE *, struct TimeStamp *);

#endif
