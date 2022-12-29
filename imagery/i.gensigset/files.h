#define _I_FILES_H

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

/* closefiles.c */
int closefiles(struct files *);

/* read_data.c */
int read_data(struct files *, struct SigSet *);

/* read_train.c */
int read_training_map(CELL *, int, int, struct files *);
