#define FORMAT_POINT 0
#define FORMAT_ALL   1

/* b2a.c */
int bin_to_asc(FILE *, FILE *, struct Map_info *, int ver, int format, int dp,
	       char *, int, int, char *, char **);
/* head.c */
int write_head(FILE * dascii, struct Map_info *Map);
