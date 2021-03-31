#ifndef GRASS_MANAGEDEFS_H
#define GRASS_MANAGEDEFS_H

/* add_elem.c */
void M__add_element(const char *, const char *);

/* do_copy.c */
int M_do_copy(int, const char *, const char *, const char *);

/* do_list.c */
void M_do_list(int, const char *);

/* do_remove.c */
int M_do_remove(int, const char *);

/* do_rename.c */
int M_do_rename(int, const char *, const char *);

/* empty.c */
int M__empty(char *);

/* find.c */
const char *M_find(int, char *, const char *);

/* get_len.c */
int M__get_description_len(int);

/* list.c */
int M_get_element(const char *);
const struct list *M_get_list(int);

/* read_list.c */
int M_read_list(int, int *);

/* option.c */
struct Option* M_define_option(int, const char *, int);
const char *M_get_options(int);
const char *M_get_option_desc(int);

/* show_elem.c */
void M_show_elements(void);

/* sighold.c */
int M__hold_signals(int);

#endif
