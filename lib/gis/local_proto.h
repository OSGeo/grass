#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

/* env.c */
void G__read_env(void);
void G__write_env(void);

/* gisinit.c */
void G__check_gisinit(void);

/* handler.c */
void G__call_error_handlers(void);

/* home.c */
const char *G__home(void);

/* mach_name.c */
const char *G__machine_name(void);

/* mapset_nme.c */
void G__get_list_of_mapsets(void);

/* timestamp.c */
int G__read_timestamp(FILE *, struct TimeStamp *);

#endif /* LOCAL_PROTO_H__ */
