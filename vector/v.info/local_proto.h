#include <grass/vector.h>
#include <grass/parson.h>

#define SHELL_NO     0x00
#define SHELL_BASIC  0x02
#define SHELL_REGION 0x04
#define SHELL_TOPO   0x08
#define STR_LEN      1024

enum OutputFormat { PLAIN, SHELL, JSON };

/* level1.c */
int level_one_info(struct Map_info *);

/* parse.c */
void parse_args(int, char **, char **, char **, int *, int *, int *,
                enum OutputFormat *);

/* print.c */
void format_double(double, char *);
void print_region(struct Map_info *, enum OutputFormat, JSON_Object *);
void print_topo(struct Map_info *, enum OutputFormat, JSON_Object *);
void print_columns(struct Map_info *, const char *, const char *,
                   enum OutputFormat);
void print_info(struct Map_info *);
void print_shell(struct Map_info *, const char *, enum OutputFormat,
                 JSON_Object *);
void parse_history_line(const char *, char *, char *, char *, char *, char *,
                        char *, char *);
void add_record_to_json(char *, char *, char *, char *, JSON_Array *, int);
void print_history(struct Map_info *, enum OutputFormat);
