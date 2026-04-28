#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/gjson.h>

enum OutputFormat { PLAIN, JSON };

int print_priv(char *, int, enum OutputFormat, G_JSON_Object *);
int print_column_definition(dbColumn *, int, enum OutputFormat, G_JSON_Array *);
int print_table_definition(dbDriver *, dbTable *, enum OutputFormat,
                           G_JSON_Object *, G_JSON_Array *);

#endif /* __LOCAL_PROTO_H__ */
