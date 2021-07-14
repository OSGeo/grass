#define _l_parms_h

struct parms
{
    char *training_map;
    char *group;
    char *subgroup;
    char sigfile[GNAME_MAX + GMAPSET_MAX + 1];
};
int parse(int, char *[], struct parms *);
int write_sigfile(struct parms *, struct Signature *);
