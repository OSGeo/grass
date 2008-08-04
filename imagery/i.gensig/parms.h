#define _l_parms_h

struct parms
{
    char *training_map;
    char *group;
    char *subgroup;
    char *sigfile;
};
int parse(int, char *[], struct parms *);
int write_sigfile(struct parms *, struct Signature *);
