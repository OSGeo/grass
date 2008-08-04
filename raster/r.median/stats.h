struct stats
{
    int nalloc;
    int n;
    long *cat;
    double *area;
};

/* median.c */
long median(struct stats *);
