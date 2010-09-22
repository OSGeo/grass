#define MAXLABELS 50

struct labels
{
    int count;			/* number of labels files */
    char *name[MAXLABELS];
    char *mapset[MAXLABELS];
    char *font[MAXLABELS];
    char *other;
};

extern struct labels labels;
