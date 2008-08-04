#define MAXLABELS 10

struct labels
{
    int count;			/* number of labels files */
    char *name[MAXLABELS];
    char *mapset[MAXLABELS];
    char *font[MAXLABELS];
    char *other;
};

#ifdef MAIN
struct labels labels;
#else
extern struct labels labels;
#endif
