#define LRS_MULTIP 1000		/* number of offset units in mp unit, later make it optional */

/* Structures, definitions and prototypes for  Linear reference system */
int LR_get_offset(dbDriver * driver, char *table_name,
		  char *lcat_col, char *lid_col,
		  char *start_map_col, char *end_map_col,
		  char *start_mp_col, char *start_off_col,
		  char *end_mp_col, char *end_off_col,
		  int lid, double mpost, double offset,
		  double multiplier, int *line_cat, double *map_offset);

int LR_get_nearest_offset(dbDriver * driver, char *table_name,
			  char *lcat_col, char *lid_col,
			  char *start_map_col, char *end_map_col,
			  char *start_mp_col, char *start_off_col,
			  char *end_mp_col, char *end_off_col,
			  int lid, double mpost, double offset,
			  double multiplier, int direction,
			  int *line_cat, double *map_offset);

int LR_get_milepost(dbDriver * driver, char *table_name,
		    char *lcat_col, char *lid_col,
		    char *start_map_col, char *end_map_col,
		    char *start_mp_col, char *start_off_col,
		    char *end_mp_col, char *end_off_col,
		    int line_cat, double map_offset,
		    double multiplier,
		    int *lid, double *mpost, double *offset);

int LR_cmp_mileposts(double mp1, double off1, double mp2, double off2);
