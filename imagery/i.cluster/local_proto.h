/* checkpt.c */
int checkpoint(struct Cluster *, int);

/* open_files.c */
int open_files(void);

/* print1.c */
int print_band_means(FILE *, struct Cluster *);

/* print2.c */
int print_class_means(FILE *, struct Cluster *);

/* print3.c */
int print_seed_means(FILE *, struct Cluster *);

/* print4.c */
int print_centroids(FILE *, struct Cluster *);

/* print5.c */
int print_separability(FILE *, struct Cluster *);

/* print6.c */
int print_distribution(FILE *, struct Cluster *);

/* timer.c */
char *print_time(time_t);
