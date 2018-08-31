#ifndef __SQLITE_PROTO_H__
#define __SQLITE_PROTO_H__

#include "globals.h"

/* error.c */
void init_error(void);

/* cursor.c */
cursor *alloc_cursor();
void free_cursor(cursor *);

/* describe.c */
int describe_table(sqlite3_stmt *, dbTable **, cursor *);

/* main.c */
int sqlite_busy_callback(void *, int);

#endif /* __SQLITE_PROTO_H__ */
