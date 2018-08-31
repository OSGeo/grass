/* mask.c */
int init_d_mask_rules(d_Mask *);
int add_d_mask_rule(d_Mask *, double, double, int);
int mask_raster_array(void *, int, int, RASTER_MAP_TYPE);
int mask_d_select(DCELL *, d_Mask *);
int mask_match_d_interval(DCELL, d_Interval *);

/* null.c */
int main(int, char *[]);
int parse_d_mask_rule(char *, d_Mask *, char *);
int process(const char *, const char *, int, RASTER_MAP_TYPE);
int doit(const char *, const char *, int, RASTER_MAP_TYPE);
