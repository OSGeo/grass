#define MAXLABELS 50

struct labels
{
    int count;			/* number of labels files */
    char *name[MAXLABELS];
    char *subproject[MAXLABELS];
    char *font[MAXLABELS];
    char *other;
};

extern struct labels labels;
