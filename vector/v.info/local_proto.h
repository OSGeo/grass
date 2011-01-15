#include <grass/vector.h>

#define NO_INFO     0x00
#define BASIC_INFO  0x02
#define REGION_INFO 0x04
#define TOPO_INFO   0x08

/* level1.c */
int level_one_info(struct Map_info *);

/* parse.c */
void parse_args(int, char**,
		char **, char**,
		int *, int *, int *);

/* print.c */
void format_double(double, char *);
void print_region(const struct Map_info *);
void print_topo(const struct Map_info *);
void print_columns(const struct Map_info *, const char *, const char *);
void print_info(const struct Map_info *);
void print_shell(const struct Map_info *);
