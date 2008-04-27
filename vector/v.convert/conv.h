/* Head */
#define DIG4_ORGAN_LEN       30
#define DIG4_DATE_LEN        20
#define DIG4_YOUR_NAME_LEN   20
#define DIG4_MAP_NAME_LEN    41
#define DIG4_SOURCE_DATE_LEN 11
#define DIG4_LINE_3_LEN      53	/* see below */

#define VERS_4_DATA_SIZE 20

#define OLD_LINE_3_SIZE 73
#define NEW_LINE_3_SIZE 53

/* old file format element type codes */
#define FILE_LINE               0
#define FILE_AREA               1
#define FILE_DOT                2
 
#define FILE_DEAD_LINE          4
#define FILE_DEAD_AREA          5
#define FILE_DEAD_DOT           6

struct Line
  {
      int type;
      int n_points;
      int cat;   /* attached category */
      double *x, *y;
  };

struct Categ
  {
      double x;  /* x location point */
      double y;  /* y location point */
      int cat;   /* Category Number */
      int type;  /* type (new vector5) */
  };

