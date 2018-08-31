#define OP_AND 0
#define OP_OR  1
#define OP_NOT 2
#define OP_XOR 3

/* Categories */
typedef struct
{
    double x, y;
    struct line_cats *cat[2];	/* category in map a and b */
    char valid;
} CENTR;

/* Attributes */
typedef struct
{
    int cat;
    int used;
    char *values;
} ATTR;

typedef struct
{
    int n;
    char *null_values;
    ATTR *attr;
    char *columns;
} ATTRIBUTES;


ATTR *find_attr(ATTRIBUTES * attributes, int cat);

int area_area(struct Map_info *In, int *field, struct Map_info *Tmp,
              struct Map_info *Out, struct field_info *Fi,
	      dbDriver * driver, int operator, int *ofield,
	      ATTRIBUTES * attr, struct ilist *BList, double snap_thresh);
int line_area(struct Map_info *In, int *field, struct Map_info *Tmp,
              struct Map_info *Out, struct field_info *Fi,
	      dbDriver * driver, int operator, int *ofield,
	      ATTRIBUTES * attr, struct ilist *BList);
