#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>

static int gettag(FILE *, char *);
static int get_semantic_labels(FILE *, struct SigSet *);
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

struct ClassData *I_AllocClassData(struct SigSet *S, struct ClassSig *C,
                                   int npixels)
{
    struct ClassData *Data;

    Data = &(C->ClassData);
    Data->npixels = npixels;
    Data->count = 0;
    Data->x = alloc_matrix(npixels, S->nbands);
    Data->p = alloc_matrix(npixels, C->nsubclasses);
    return Data;
}

/*!
 * \brief Initialize struct SigSet before use
 *
 * No need to call before calling I_ReadSigSet.
 *
 * \param S *Signature to initialize
 * \param nbands band (imagery group member) count
 */
int I_InitSigSet(struct SigSet *S, int nbands)
{
    S->nbands = nbands;
    S->semantic_labels = (char **)G_malloc(nbands * sizeof(char **));
    for (int i = 0; i < nbands; i++)
        S->semantic_labels[i] = NULL;
    S->nclasses = 0;
    S->ClassSig = NULL;
    S->title = NULL;

    return 0;
}

struct ClassSig *I_NewClassSig(struct SigSet *S)
{
    struct ClassSig *Sp;

    if (S->nclasses == 0)
        S->ClassSig = (struct ClassSig *)G_malloc(sizeof(struct ClassSig));
    else
        S->ClassSig = (struct ClassSig *)G_realloc(
            (char *)S->ClassSig, sizeof(struct ClassSig) * (S->nclasses + 1));

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
        C->SubSig = (struct SubSig *)G_realloc(
            (char *)C->SubSig, sizeof(struct SubSig) * (C->nsubclasses + 1));

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

#define eq(a, b) strcmp(a, b) == 0

/*!
 * \brief Read sigset signatures from file
 *
 * File stream should be opened in advance by call to
 * I_fopen_sigset_file_old()
 * It is up to caller to fclose the file stream afterwards.
 *
 * There is no need to initialise struct SigSet in advance, as this
 * function internally calls I_InitSigSet.
 *
 * \param fd pointer to FILE*
 * \param S pointer to struct SigSet *S
 *
 * \return 1 on success, -1 on failure
 */
int I_ReadSigSet(FILE *fd, struct SigSet *S)
{
    char tag[256];
    unsigned int version;

    if (fscanf(fd, "%u", &version) != 1) {
        G_warning(_("Invalid signature file"));
        return -1;
    }
    if (version != 1) {
        G_warning(_("Invalid signature file version"));
        return -1;
    }

    I_InitSigSet(S, 0);
    while (gettag(fd, tag)) {
        if (eq(tag, "title:"))
            if (get_title(fd, S) != 0)
                return -1;
        if (eq(tag, "semantic_labels:"))
            if (get_semantic_labels(fd, S) != 0)
                return -1;
        if (eq(tag, "class:"))
            if (get_class(fd, S) != 0)
                return -1;
    }
    return 1; /* for now assume success */
}

static int gettag(FILE *fd, char *tag)
{
    if (fscanf(fd, "%255s", tag) != 1)
        return 0;
    G_strip(tag);
    return 1;
}

static int get_semantic_labels(FILE *fd, struct SigSet *S)
{
    int n, pos;
    char c, prev;
    char semantic_label[GNAME_MAX];

    /* Read semantic labels and count them to set nbands */
    n = 0;
    pos = 0;
    S->semantic_labels =
        (char **)G_realloc(S->semantic_labels, (n + 1) * sizeof(char **));
    while ((c = (char)fgetc(fd)) != EOF) {
        if (c == '\n') {
            if (prev != ' ') {
                semantic_label[pos] = '\0';
                if (strlen(semantic_label) > 0) {
                    S->semantic_labels[n] = G_store(semantic_label);
                    n++;
                }
            }
            S->nbands = n;
            break;
        }
        if (c == ' ') {
            semantic_label[pos] = '\0';
            if (strlen(semantic_label) > 0) {
                S->semantic_labels[n] = G_store(semantic_label);
                n++;
                /* [n] is 0 based thus: (n + 1) */
                S->semantic_labels = (char **)G_realloc(
                    S->semantic_labels, (n + 1) * sizeof(char **));
            }
            pos = 0;
            prev = c;
            continue;
        }
        /* Semantic labels are limited to GNAME_MAX - 1 + \0 in length;
         * n is 0-based */
        if (pos == (GNAME_MAX - 2)) {
            G_warning(_("Invalid signature file: semantic label length limit "
                        "exceeded"));
            return -1;
        }
        semantic_label[pos] = c;
        pos++;
        prev = c;
    }
    if (!(S->nbands > 0)) {
        G_warning(_("Signature file does not contain bands"));
        return -1;
    }

    return 0;
}

static int get_title(FILE *fd, struct SigSet *S)
{
    char title[1024];

    *title = 0;
    if (fscanf(fd, "%1023[^\n]", title) != 1)
        return -1;
    G_strip(title);
    I_SetSigTitle(S, title);

    return 0;
}

static int get_class(FILE *fd, struct SigSet *S)
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

static int get_classnum(FILE *fd, struct ClassSig *C)
{
    if (fscanf(fd, "%ld", &C->classnum) != 1)
        return -1;

    return 0;
}

static int get_classtype(FILE *fd, struct ClassSig *C)
{
    if (fscanf(fd, "%d", &C->type) != 1)
        return -1;

    return 0;
}

static int get_classtitle(FILE *fd, struct ClassSig *C)
{
    char title[1024];

    *title = 0;
    if (fscanf(fd, "%1023[^\n]", title) != 1)
        return -1;
    G_strip(title);
    I_SetClassTitle(C, title);

    return 0;
}

static int get_subclass(FILE *fd, struct SigSet *S, struct ClassSig *C)
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

static int get_subclass_pi(FILE *fd, struct SubSig *Sp)
{
    if (fscanf(fd, "%lf", &Sp->pi) != 1)
        return -1;

    return 0;
}

static int get_subclass_means(FILE *fd, struct SubSig *Sp, int nbands)
{
    int i;

    for (i = 0; i < nbands; i++)
        if (fscanf(fd, "%lf", &Sp->means[i]) != 1)
            return -1;

    return 0;
}

static int get_subclass_covar(FILE *fd, struct SubSig *Sp, int nbands)
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

int I_WriteSigSet(FILE *fd, const struct SigSet *S)
{
    const struct ClassSig *Cp;
    const struct SubSig *Sp;
    int i, j, b1, b2;

    /* This is version 1 sigset file format */
    fprintf(fd, "1\n");
    fprintf(fd, "title: %s\n", I_GetSigTitle(S));
    fprintf(fd, "semantic_labels: ");
    for (i = 0; i < S->nbands; i++) {
        fprintf(fd, "%s ", S->semantic_labels[i]);
    }
    fprintf(fd, "\n");
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

/*!
 * \brief Reorder struct SigSet to match imagery group member order
 *
 * The function will check for semantic label match between sigset struct
 * and imagery group.
 *
 * In the case of a complete semantic label match, values of passed in
 * struct SigSet are reordered to match the order of imagery group items.
 * This reordering is done only for items present in the sigset file.
 * Thus reordering should be done only after calling I_ReadSigSet.
 *
 * If all semantic labels are not identical (in
 * arbitrary order), function will return two dimensional array with
 * comma separated list of:
 *      - [0] semantic labels present in the signature struct but
 * absent in the imagery group
 *      - [1] semantic labels present in the imagery group but
 * absent in the signature struct
 *
 * If no mismatch of semantic labels for signatures or imagery group are
 * detected (== all are present in the other list), a NULL value will be
 * returned in the particular list of mismatches (not an empty string).
 * For example:
 * \code
 * if (ret && ret[1])
 *     printf("List of imagery group bands without signatures: %s\n, ret[1]);
 * \endcode
 *
 * \param S existing signatures to check & sort
 * \param R group reference
 *
 * \return NULL successfully sorted
 * \return err_array two comma separated lists of mismatches
 */
char **I_SortSigSetBySemanticLabel(struct SigSet *S, const struct Ref *R)
{
    unsigned int total, complete;
    unsigned int *match1, *match2, mc1, mc2, *new_order;
    double ***new_means, ****new_vars;
    char **group_semantic_labels, **mismatches, **new_semantic_labels;

    /* Safety measure. Untranslated as this should not happen in production! */
    if (S->nbands < 1 || R->nfiles < 1)
        G_fatal_error("Programming error. Invalid length structs passed to "
                      "I_sort_signatures_by_semantic_label(%d, %d);",
                      S->nbands, R->nfiles);

    /* Obtain group semantic labels */
    group_semantic_labels = (char **)G_malloc(R->nfiles * sizeof(char *));
    for (unsigned int j = R->nfiles; j--;) {
        group_semantic_labels[j] =
            Rast_get_semantic_label_or_name(R->file[j].name, R->file[j].mapset);
    }

    /* If lengths are not equal, there will be a mismatch */
    complete = S->nbands == R->nfiles;

    /* Initialize match tracker */
    new_order = (unsigned int *)G_malloc(S->nbands * sizeof(unsigned int));
    match1 = (unsigned int *)G_calloc(S->nbands, sizeof(unsigned int));
    match2 = (unsigned int *)G_calloc(R->nfiles, sizeof(unsigned int));

    /* Allocate memory for temporary storage of sorted values */
    new_semantic_labels = (char **)G_malloc(S->nbands * sizeof(char *));
    new_means = (double ***)G_malloc(S->nclasses * sizeof(double **));
    // new_vars[S.ClassSig[x]][.SubSig[y]][R[band1]][R[band1]]
    new_vars = (double ****)G_malloc(S->nclasses * sizeof(double ***));
    for (unsigned int c = S->nclasses; c--;) {
        new_means[c] =
            (double **)G_malloc(S->ClassSig[c].nsubclasses * sizeof(double *));
        new_vars[c] = (double ***)G_malloc(S->ClassSig[c].nsubclasses *
                                           sizeof(double **));
        for (unsigned int s = S->ClassSig[c].nsubclasses; s--;) {
            new_means[c][s] = (double *)G_malloc(S->nbands * sizeof(double));
            new_vars[c][s] = (double **)G_malloc(S->nbands * sizeof(double *));
            for (unsigned int i = S->nbands; i--;)
                new_vars[c][s][i] =
                    (double *)G_malloc(S->nbands * sizeof(double));
        }
    }

    /* Obtain order of matching items */
    for (unsigned int j = R->nfiles; j--;) {
        for (unsigned int i = S->nbands; i--;) {
            if (S->semantic_labels[i] && group_semantic_labels[j] &&
                !strcmp(S->semantic_labels[i], group_semantic_labels[j])) {
                if (complete) {
                    /* Reorder pointers to existing strings only */
                    new_semantic_labels[j] = S->semantic_labels[i];
                    new_order[i] = j;
                }
                /* Keep a track of matching items for error reporting */
                match1[i] = 1;
                match2[j] = 1;
                break;
            }
        }
    }

    /* Check for semantic label mismatch */
    mc1 = mc2 = 0;
    mismatches = (char **)G_malloc(2 * sizeof(char **));
    mismatches[0] = NULL;
    mismatches[1] = NULL;
    total = 1;
    for (unsigned int i = 0; i < (unsigned int)S->nbands; i++) {
        if (!match1[i]) {
            if (S->semantic_labels[i])
                total = total + strlen(S->semantic_labels[i]);
            else
                total = total + 24;
            mismatches[0] =
                (char *)G_realloc(mismatches[0], total * sizeof(char *));
            if (mc1)
                strcat(mismatches[0], ",");
            else
                mismatches[0][0] = '\0';
            if (S->semantic_labels[i])
                strcat(mismatches[0], S->semantic_labels[i]);
            else
                strcat(mismatches[0], "<semantic label missing>");
            mc1++;
            total = total + 1;
        }
    }
    total = 1;
    for (unsigned int j = 0; j < (unsigned int)R->nfiles; j++) {
        if (!match2[j]) {
            if (group_semantic_labels[j])
                total = total + strlen(group_semantic_labels[j]);
            else
                total = total + 24;
            mismatches[1] =
                (char *)G_realloc(mismatches[1], total * sizeof(char *));
            if (mc2)
                strcat(mismatches[1], ",");
            else
                mismatches[1][0] = '\0';
            if (group_semantic_labels[j])
                strcat(mismatches[1], group_semantic_labels[j]);
            else
                strcat(mismatches[1], "<semantic label missing>");
            mc2++;
            total = total + 1;
        }
    }

    /* Swap mean and var matrix values in each of classes */
    if (!mc1 && !mc2) {
        for (unsigned int c = S->nclasses; c--;) {
            for (unsigned int s = S->ClassSig[c].nsubclasses; s--;) {
                for (unsigned int b1 = 0; b1 < (unsigned int)S->nbands; b1++) {
                    new_means[c][s][new_order[b1]] =
                        S->ClassSig[c].SubSig[s].means[b1];
                    for (unsigned int b2 = 0; b2 < (unsigned int)S->nbands;
                         b2++) {
                        new_vars[c][s][new_order[b1]][new_order[b2]] =
                            S->ClassSig[c].SubSig[s].R[b1][b2];
                    }
                }
            }
        }

        /* Replace values in struct with ordered ones */
        memcpy(S->semantic_labels, new_semantic_labels,
               S->nbands * sizeof(char **));
        for (unsigned int c = S->nclasses; c--;) {
            for (unsigned int s = S->ClassSig[c].nsubclasses; s--;) {
                memcpy(S->ClassSig[c].SubSig[s].means, new_means[c][s],
                       S->nbands * sizeof(double));
                for (unsigned int i = S->nbands; i--;)
                    memcpy(S->ClassSig[c].SubSig[s].R[i], new_vars[c][s][i],
                           S->nbands * sizeof(double));
            }
        }
    }

    /* Clean up */
    for (unsigned int j = R->nfiles; j--;)
        free(group_semantic_labels[j]);
    free(group_semantic_labels);
    free(new_order);
    free(match1);
    free(match2);
    free(new_semantic_labels);
    for (unsigned int c = S->nclasses; c--;) {
        for (unsigned int s = S->ClassSig[c].nsubclasses; s--;) {
            free(new_means[c][s]);
            for (unsigned int i = S->nbands; i--;)
                free(new_vars[c][s][i]);
            free(new_vars[c][s]);
        }
        free(new_means[c]);
        free(new_vars[c]);
    }

    free(new_means);
    free(new_vars);

    if (mc1 || mc2) {
        return mismatches;
    }
    free(mismatches);
    return NULL;
}
