/* display.c */
int display(const char *, int, char *, RASTER_MAP_TYPE, int);
int mask_raster_array(void *, int, int, RASTER_MAP_TYPE);

/* main.c */
int parse_mask_rule(char *, Mask *, char *);
int parse_d_mask_rule(char *, d_Mask *, char *);

/* mask.c */
int init_mask_rules(Mask *);
int init_d_mask_rules(d_Mask *);
int add_mask_rule(Mask *, long, long, int);
int add_d_mask_rule(d_Mask *, double, double, int);
int mask_cell_array(CELL *, int, Mask *, int);
int mask_d_cell_array(DCELL *, int, d_Mask *, int);
int mask_select(long *, Mask *, int);
int mask_d_select(DCELL *, d_Mask *, int);
int mask_match_interval(long, Interval *);
int mask_match_d_interval(DCELL, d_Interval *);
