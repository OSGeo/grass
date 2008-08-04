#include <grass/gis.h>
#include <grass/glocale.h>

struct list
{
    char **element;		/* list of related elements */
    char *alias;		/* element alias */
    char **desc;		/* description of elements */
    char *text;			/* menu text */
    int nelem;			/* number of elements */
    char status;
    char *mainelem;		/* main element */
    char *maindesc;		/* main element description */
};

#ifdef MAIN
int nlist;
struct list *list;
#endif

extern int nlist;
extern struct list *list;

#define REMOVE 1
#define RENAME 2
#define COPY   3
#define LIST   4

char *ask_in_mapset();

/* add_elem.c */
int add_element(char *, char *);

/* ask.c */
char *ask_old(int, char *, char *);
char *ask_new(int, char *, char *);
char *ask_in_mapset(int, char *, char *);

/* copyfile.c */
int copyfile(char *, char *, char *, char *);

/* do_copy.c */
int do_copy(int, char *, char *, char *);

/* do_list.c */
int do_list(int, char *);

/* do_remove.c */
int do_remove(int, char *);

/* do_rename.c */
int do_rename(int, char *, char *);

/* empty.c */
int empty(char *);

/* find.c */
char *find(int, char *, char *);

/* get_len.c */
int get_description_len(int);

/* menu.c */
int menu(int);
int build_menu(void);
int free_menu(void);
int display_menu(void);

/* read_list.c */
int read_list(int);

/* show_elem.c */
int show_elements(void);

/* sighold.c */
int hold_signals(int);
