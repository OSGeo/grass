struct files
{
    int train_fd;
    CELL *train_cell;
    int ncats;
    CELL *training_cats;
    struct Categories training_labels;

    int *band_fd;
    DCELL **band_cell;
    int nbands;
};
int closefiles(struct files *);
int compute_covariances(struct files *, struct Signature *);
int get_training_classes(struct files *, struct Signature *);
int compute_means(struct files *, struct Signature *);
int read_training_map(CELL *, int, int, struct files *);

#ifdef _l_parms_h
void read_training_labels(struct parms *, struct files *);
int openfiles(struct parms *, struct files *);
#endif
