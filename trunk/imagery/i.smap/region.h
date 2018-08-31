struct Region
{
    int xmin, xmax;
    int ymin, ymax;
    struct Free
    {
	int left;
	int right;
	int top;
	int bottom;
    } free;
};

/* decimate.c */
void make_pyramid(LIKELIHOOD ****, struct Region *, int, double *);
char ***get_pyramid(int, int, size_t);
void free_pyramid(char *, int, int);
char ****get_cubic_pyramid(int, int, int, size_t);
void free_cubic_pyramid(char *, int, int, int);
int levels(int, int);

/* interp.c */
void seq_MAP(unsigned char ***, struct Region *, LIKELIHOOD ****, int,
	     double *, float **);
void MLE(unsigned char **, LIKELIHOOD ***, struct Region *, int, float **);

/* reg_util.c */
int levels_reg(struct Region *);
void dec_reg(struct Region *);
void copy_reg(struct Region *, struct Region *);
void reg_to_wdht(struct Region *, int *, int *);

/* read_block.c */
int read_block(DCELL ***, struct Region *, struct files *);

/* segment.c */
#ifdef GRASS_IMAGERY_H
/* model.c */
void extract_init(struct SigSet *);
void extract(DCELL ***, struct Region *, LIKELIHOOD ***, struct SigSet *);
#endif
