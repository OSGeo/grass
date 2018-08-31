struct parms
{
    char *training_map;
    char *group;
    char *subgroup;
    char *sigfile;
    int maxsubclasses;
};

/* parse.c */
int parse(int, char *[], struct parms *);

/* write_sig.c */
int write_sigfile(struct parms *, struct SigSet *);

#ifdef _I_FILES_H
/* get_train.c */
int get_training_classes(struct parms *, struct files *, struct SigSet *);

/* labels.c */
void read_training_labels(struct parms *, struct files *);

/* openfiles.c */
int openfiles(struct parms *, struct files *);
#endif
