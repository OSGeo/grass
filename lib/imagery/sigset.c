#include <string.h>
#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/gis.h>

static int gettag(FILE *, char *);
static int get_nbands(FILE *, struct SigSet *);
static int get_title(FILE *, struct SigSet *);
static int get_class(FILE *, struct SigSet *);
static int get_classnum(FILE *, struct ClassSig *);
static int get_classtype(FILE *, struct ClassSig *);
static int get_classtitle(FILE *, struct ClassSig *);
static int get_subclass(FILE *, struct SigSet *, struct ClassSig *);
static int get_subclass_pi(FILE *, struct SubSig *);
static int get_subclass_means(FILE *, struct SubSig *, int);
static int get_subclass_covar(FILE *, struct SubSig *, int);

static double **alloc_matrix(int rows, int cols)
{
    double **m;
    int i;

    m = (double **)G_calloc(rows, sizeof(double *));
    m[0] = (double *)G_calloc(rows * cols, sizeof(double));
    for (i = 1; i < rows; i++)
	m[i] = m[i - 1] + cols;

    return m;
}

int I_SigSetNClasses(struct SigSet *S)
{
    int i, count;

    for (i = 0, count = 0; i < S->nclasses; i++)
	if (S->ClassSig[i].used)
	    count++;

    return count;
}


struct ClassData *I_AllocClassData(struct SigSet *S,
				   struct ClassSig *C, int npixels)
{
    struct ClassData *Data;

    Data = &(C->ClassData);
    Data->npixels = npixels;
    Data->count = 0;
    Data->x = alloc_matrix(npixels, S->nbands);
    Data->p = alloc_matrix(npixels, C->nsubclasses);
    return Data;
}

int I_InitSigSet(struct SigSet *S)
{
    S->nbands = 0;
    S->nclasses = 0;
    S->ClassSig = NULL;
    S->title = NULL;

    return 0;
}

int I_SigSetNBands(struct SigSet *S, int nbands)
{
    S->nbands = nbands;

    return 0;
}

struct ClassSig *I_NewClassSig(struct SigSet *S)
{
    struct ClassSig *Sp;

    if (S->nclasses == 0)
	S->ClassSig = (struct ClassSig *)G_malloc(sizeof(struct ClassSig));
    else
	S->ClassSig = (struct ClassSig *)G_realloc((char *)S->ClassSig,
						   sizeof(struct ClassSig) *
						   (S->nclasses + 1));

    Sp = &S->ClassSig[S->nclasses++];
    Sp->classnum = 0;
    Sp->nsubclasses = 0;
    Sp->used = 1;
    Sp->type = SIGNATURE_TYPE_MIXED;
    Sp->title = NULL;
    return Sp;
}

struct SubSig *I_NewSubSig(struct SigSet *S, struct ClassSig *C)
{
    struct SubSig *Sp;
    int i;

    if (C->nsubclasses == 0)
	C->SubSig = (struct SubSig *)G_malloc(sizeof(struct SubSig));
    else
	C->SubSig = (struct SubSig *)G_realloc((char *)C->SubSig,
					       sizeof(struct SubSig) *
					       (C->nsubclasses + 1));

    Sp = &C->SubSig[C->nsubclasses++];
    Sp->used = 1;
    Sp->R = (double **)G_calloc(S->nbands, sizeof(double *));
    Sp->R[0] = (double *)G_calloc(S->nbands * S->nbands, sizeof(double));
    for (i = 1; i < S->nbands; i++)
	Sp->R[i] = Sp->R[i - 1] + S->nbands;
    Sp->Rinv = (double **)G_calloc(S->nbands, sizeof(double *));
    Sp->Rinv[0] = (double *)G_calloc(S->nbands * S->nbands, sizeof(double));
    for (i = 1; i < S->nbands; i++)
	Sp->Rinv[i] = Sp->Rinv[i - 1] + S->nbands;
    Sp->means = (double *)G_calloc(S->nbands, sizeof(double));
    Sp->N = 0;
    Sp->pi = 0;
    Sp->cnst = 0;
    return Sp;
}

#define eq(a,b) strcmp(a,b)==0

int I_ReadSigSet(FILE * fd, struct SigSet *S)
{
    char tag[256];

    I_InitSigSet(S);

    while (gettag(fd, tag)) {
	if (eq(tag, "title:"))
	    if (get_title(fd, S) != 0)
            return -1;
	if (eq(tag, "nbands:"))
	    if (get_nbands(fd, S) != 0)
            return -1;
	if (eq(tag, "class:"))
	    if (get_class(fd, S) != 0)
            return -1;
    }
    return 1;			/* for now assume success */
}

static int gettag(FILE * fd, char *tag)
{
    if (fscanf(fd, "%s", tag) != 1)
	return 0;
    G_strip(tag);
    return 1;
}

static int get_nbands(FILE * fd, struct SigSet *S)
{
    if (fscanf(fd, "%d", &S->nbands) != 1)
        return -1;

    return 0;
}

static int get_title(FILE * fd, struct SigSet *S)
{
    char title[1024];

    *title = 0;
    if (fscanf(fd, "%[^\n]", title) != 1)
        return -1;
    I_SetSigTitle(S, title);

    return 0;
}

static int get_class(FILE * fd, struct SigSet *S)
{
    char tag[1024];
    struct ClassSig *C;

    C = I_NewClassSig(S);
    while (gettag(fd, tag)) {
	if (eq(tag, "endclass:"))
	    break;
	if (eq(tag, "classnum:"))
	    if (get_classnum(fd, C) != 0)
            return -1;
	if (eq(tag, "classtype:"))
	    if (get_classtype(fd, C) != 0)
            return -1;
	if (eq(tag, "classtitle:"))
	    if (get_classtitle(fd, C) != 0)
            return -1;
	if (eq(tag, "subclass:"))
	    if (get_subclass(fd, S, C) != 0)
            return -1;
    }

    return 0;
}

static int get_classnum(FILE * fd, struct ClassSig *C)
{
    if (fscanf(fd, "%ld", &C->classnum) != 1)
        return -1;

    return 0;
}

static int get_classtype(FILE * fd, struct ClassSig *C)
{
    if (fscanf(fd, "%d", &C->type) != 1)
        return -1;

    return 0;
}

static int get_classtitle(FILE * fd, struct ClassSig *C)
{
    char title[1024];

    *title = 0;
    if (fscanf(fd, "%[^\n]", title) != 1)
        return -1;
    I_SetClassTitle(C, title);

    return 0;
}

static int get_subclass(FILE * fd, struct SigSet *S, struct ClassSig *C)
{
    struct SubSig *Sp;
    char tag[1024];

    Sp = I_NewSubSig(S, C);

    while (gettag(fd, tag)) {
	if (eq(tag, "endsubclass:"))
	    break;
	if (eq(tag, "pi:"))
	    if (get_subclass_pi(fd, Sp) != 0)
            return -1;
	if (eq(tag, "means:"))
	    if (get_subclass_means(fd, Sp, S->nbands) != 0)
            return -1;
	if (eq(tag, "covar:"))
	    if (get_subclass_covar(fd, Sp, S->nbands) != 0)
            return -1;
    }

    return 0;
}

static int get_subclass_pi(FILE * fd, struct SubSig *Sp)
{
    if (fscanf(fd, "%lf", &Sp->pi) != 1)
        return -1;

    return 0;
}

static int get_subclass_means(FILE * fd, struct SubSig *Sp, int nbands)
{
    int i;

    for (i = 0; i < nbands; i++)
	if (fscanf(fd, "%lf", &Sp->means[i]) != 1)
        return -1;

    return 0;
}

static int get_subclass_covar(FILE * fd, struct SubSig *Sp, int nbands)
{
    int i, j;

    for (i = 0; i < nbands; i++)
	for (j = 0; j < nbands; j++)
	    if (fscanf(fd, "%lf", &Sp->R[i][j]) != 1)
            return -1;

    return 0;
}

int I_SetSigTitle(struct SigSet *S, const char *title)
{
    if (title == NULL)
	title = "";
    if (S->title)
	free(S->title);
    S->title = G_store(title);

    return 0;
}

const char *I_GetSigTitle(const struct SigSet *S)
{
    if (S->title)
	return S->title;
    else
	return "";
}

int I_SetClassTitle(struct ClassSig *C, const char *title)
{
    if (title == NULL)
	title = "";
    if (C->title)
	free(C->title);
    C->title = G_store(title);

    return 0;
}

const char *I_GetClassTitle(const struct ClassSig *C)
{
    if (C->title)
	return C->title;
    else
	return "";
}

int I_WriteSigSet(FILE * fd, const struct SigSet *S)
{
    const struct ClassSig *Cp;
    const struct SubSig *Sp;
    int i, j, b1, b2;

    fprintf(fd, "title: %s\n", I_GetSigTitle(S));
    fprintf(fd, "nbands: %d\n", S->nbands);
    for (i = 0; i < S->nclasses; i++) {
	Cp = &S->ClassSig[i];
	if (!Cp->used)
	    continue;
	if (Cp->nsubclasses <= 0)
	    continue;
	fprintf(fd, "class:\n");
	fprintf(fd, " classnum: %ld\n", Cp->classnum);
	fprintf(fd, " classtitle: %s\n", I_GetClassTitle(Cp));
	fprintf(fd, " classtype: %d\n", Cp->type);

	for (j = 0; j < Cp->nsubclasses; j++) {
	    Sp = &Cp->SubSig[j];
	    fprintf(fd, " subclass:\n");
	    fprintf(fd, "  pi: %g\n", Sp->pi);
	    fprintf(fd, "  means:");
	    for (b1 = 0; b1 < S->nbands; b1++)
		fprintf(fd, " %g", Sp->means[b1]);
	    fprintf(fd, "\n");
	    fprintf(fd, "  covar:\n");
	    for (b1 = 0; b1 < S->nbands; b1++) {
		fprintf(fd, "   ");
		for (b2 = 0; b2 < S->nbands; b2++)
		    fprintf(fd, " %g", Sp->R[b1][b2]);
		fprintf(fd, "\n");
	    }
	    fprintf(fd, " endsubclass:\n");
	}
	fprintf(fd, "endclass:\n");
    }

    return 0;
}
