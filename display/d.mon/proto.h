#ifndef __PROTO_H__
#define __PROTO_H__
#define DEFAULT_WIDTH 720
#define DEFAULT_HEIGHT 480


/* start */
int start_mon(const char *, const char *, int, int, int,
	      const char *, int, int, int);

/* select.c */
int select_mon(const char *);

/* stop.c */
int stop_mon(const char *);

/* list.c */
void list_mon();
void print_list(FILE *);
int check_mon(const char *);
void list_cmd(const char *, FILE *);
char *get_path(const char *, int);
void list_files(const char *, FILE *);
#endif
