#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/parson.h>

enum OutputFormat { PLAIN, JSON };

int print_priv(char *, int, enum OutputFormat, JSON_Object *);
int print_column_definition(dbColumn *, int, enum OutputFormat, JSON_Array *);
int print_table_definition(dbDriver *, dbTable *, enum OutputFormat,
                           JSON_Object *, JSON_Array *);

#endif /* __LOCAL_PROTO_H__ */
