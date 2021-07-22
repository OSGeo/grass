#include <stdlib.h>
#include <string.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
 * \brief Initialize struct Signature before use
 *
 * No need to call before calling I_read_signatures.
 *
 * \param *Signature to initialize
 * \param nbands band (imagery group member) count
 */
int I_init_signatures(struct Signature *S, int nbands)
{
    S->nbands = nbands;
    S->bandrefs = (char **)G_malloc(nbands * sizeof(char **));
    for (int i = 0; i < nbands; i++)
        S->bandrefs[i] = NULL;
    S->nsigs = 0;
    S->sig = NULL;
    S->title[0] = 0;

    return 0;
}

#define SIG struct One_Sig

int I_new_signature(struct Signature *S)
{
    int n;
    int i;

    i = S->nsigs++;
    S->sig = (SIG *) G_realloc(S->sig, S->nsigs * sizeof(SIG));

    S->sig[i].mean = (double *)G_calloc(S->nbands, sizeof(double));
    S->sig[i].var = (double **)G_calloc(S->nbands, sizeof(double *));

    for (n = 0; n < S->nbands; n++)
	S->sig[i].var[n] = (double *)G_calloc(S->nbands, sizeof(double));

    S->sig[i].status = 0;
    S->sig[i].have_color = 0;
    sprintf(S->sig[i].desc, "Class %d", i + 1);
    return S->nsigs;
}

/*!
 * \brief Free memory allocated for struct Signature
 *
 * One must call I_init_signatures() to re-use struct after it has been
 * passed to this function.
 *
 * \param *Signature to free
 *
 * \return always 0
 */
int I_free_signatures(struct Signature *S)
{
    int n;
    int i;

    for (i = 0; i < S->nsigs; i++) {
	for (n = 0; n < S->nbands; n++)
	    free(S->sig[i].var[n]);
	free(S->sig[i].var);
	free(S->sig[i].mean);
    }
    free(S->sig);
    for (n = 0; n < S->nbands; n++)
        free(S->bandrefs[n]);
    free(S->bandrefs);

    S->sig = NULL;
    S->bandrefs = NULL;
    S->nbands = 0;
    S->nsigs = 0;
    S->title[0] = '\0';

    return 0;
}

int I_read_one_signature(FILE * fd, struct Signature *S)
{
    int n;
    int i;
    struct One_Sig *s;

    while ((i = fgetc(fd)) != EOF)
	if (i == '#')
	    break;
    if (i != '#')
	return 0;

    i = I_new_signature(S);
    s = &S->sig[i - 1];

    I_get_to_eol(s->desc, sizeof(s->desc), fd);
    G_strip(s->desc);

    if (fscanf(fd, "%d", &s->npoints) != 1)
	return -1;

    for (i = 0; i < S->nbands; i++) {
	if (fscanf(fd, "%lf", &s->mean[i]) != 1)
	    return -1;
    }

    for (i = 0; i < S->nbands; i++) {
	for (n = 0; n <= i; n++) {
	    if (fscanf(fd, "%lf", &s->var[i][n]) != 1)
		return -1;
	    s->var[n][i] = s->var[i][n];	/* added 28 aug 91 */
	}
    }
    if (fscanf(fd, "%f%f%f", &s->r, &s->g, &s->b) == 3 &&
	s->r >= 0.0 && s->r <= 1.0 &&
	s->g >= 0.0 && s->g <= 1.0 && s->b >= 0.0 && s->b <= 1.0)
	s->have_color = 1;

    s->status = 1;
    return 1;
}

/*!
 * \brief Read signatures from file
 *
 * File stream should be opened in advance by call to
 * I_fopen_signature_file_old()
 * It is up to caller to fclose the file stream afterwards.
 *
 * There is no need to initialize struct Signature in advance, as this
 * function internally calls I_init_signatures.
 *
 * \param pointer to FILE*
 * \param pointer to struct Signature *S
 *
 * \return 1 on success, -1 on failure
 */
int I_read_signatures(FILE * fd, struct Signature *S)
{
    int ver, n, pos;
    char c, prev;
    char bandref[GNAME_MAX];

    I_init_signatures(S, 0);
    S->title[0] = 0;
    /* File of signatures must start with its version number */
    if (fscanf(fd, "%d", &ver) != 1) {
        G_warning(_("Invalid signature file"));
        return -1;
    }
    /* Current version number is 1 */
    if (ver != 1) {
        G_warning(_("Invalid signature file version"));
        return -1;
    }

    /* Goto title line and strip initial # */
    while ((c = (char)fgetc(fd)) != EOF)
        if (c == '#')
            break;
    I_get_to_eol(S->title, sizeof(S->title), fd);
    G_strip(S->title);

    /* Read band references and count them to set nbands */
    n = 0;
    pos = 0;
    S->bandrefs = (char **)G_realloc(S->bandrefs, (n + 1) * sizeof(char **));
    while ((c = (char)fgetc(fd)) != EOF) {
        if (c == '\n') {
            if (prev != ' ') {
                bandref[pos] = '\0';
                S->bandrefs[n] = G_store(bandref);
                n++;
            }
            S->nbands = n;
            break;
        }
        if (c == ' ') {
            bandref[pos] = '\0';
            S->bandrefs[n] = G_store(bandref);
            n++;
            /* [n] is 0 based thus: (n + 1) */
            S->bandrefs = (char **)G_realloc(S->bandrefs, (n + 1) * sizeof(char **));
            pos = 0;
            prev = c;
            continue;
        }
        /* Band references are limited to GNAME_MAX - 1 + \0 in length;
         * n is 0-based */
        if (pos == (GNAME_MAX - 2)) {
            G_warning(_("Invalid signature file: band reference length limit exceeded"));
            return -1;
        }
        bandref[pos] = c;
        pos++;
        prev = c;
    }

    if (!(S->nbands > 0)) {
        G_warning(_("Signature file does not contain bands"));
        return -1;
    }

    while ((n = I_read_one_signature(fd, S)) == 1) ;

    if (n < 0)
	return -1;
    if (S->nsigs == 0)
	return -1;
    return 1;
}

/*!
 * \brief Write signatures to file
 *
 * File stream should be opened in advance by call to
 * I_fopen_signature_file_new()
 * It is up to caller to fclose the file stream afterwards.
 *
 * \param pointer to FILE*
 * \param pointer to struct Signature *S
 *
 * \return always 1
 */
int I_write_signatures(FILE * fd, struct Signature *S)
{
    int k;
    int n;
    int i;
    struct One_Sig *s;

    /* Version of signatures file structure.
     * Increment if file structure changes.
     */
    fprintf(fd, "1\n");
    /* Title of signatures */
    fprintf(fd, "#%s\n", S->title);
    /* A list of space separated band references for each
     * raster map used to generate sigs. */
    for (k = 0; k < S->nbands; k++) {
        fprintf(fd, "%s ", S->bandrefs[k]);
    }
    fprintf(fd, "\n");
    /* A signature for each target class */
    for (k = 0; k < S->nsigs; k++) {
	s = &S->sig[k];
	if (s->status != 1)
	    continue;
    /* Label for each class represented by this signature */
    fprintf(fd, "#%s\n", s->desc);
    /* Point count used to generate signature */
	fprintf(fd, "%d\n", s->npoints);
    /* Values are in the same order as band references */
	for (i = 0; i < S->nbands; i++)
	    fprintf(fd, "%g ", s->mean[i]);
	fprintf(fd, "\n");
	for (i = 0; i < S->nbands; i++) {
	    for (n = 0; n <= i; n++)
		fprintf(fd, "%g ", s->var[i][n]);
	    fprintf(fd, "\n");
	}
	if (s->have_color)
	    fprintf(fd, "%g %g %g\n", s->r, s->g, s->b);
    }
    return 1;
}

/*!
 * \brief Reorder struct Signature to match imagery group member order
 *
 * The function will check for band reference match between signature struct
 * and imagery group.
 *
 * In the case of a complete band reference match, values of passed in
 * struct Signature are reordered to match the order of imagery group items.
 *
 * If all band references are not identical (in
 * arbitrary order), function will return two dimensional array with
 * comma separated list of:
 *      - [0] band references present in the signature struct but
 * absent in the imagery group
 *      - [1] band references present in the imagery group but
 * absent in the signature struct
 *
 * If no mismatch of band references for signatures or imagery group are
 * detected (== all are present in the other list), a NULL value will be
 * returned in the particular list of mismatches (not an empty string).
 * For example:
 * \code if (ret && ret[1]) printf("List of imagery group bands without signatures: %s\n, ret[1]); \endcode
 *
 * \param *Signature existing signatures to check & sort
 * \param *Ref group reference
 *
 * \return NULL successfully sorted
 * \return err_array two comma separated lists of mismatches
 */
char **I_sort_signatures_by_bandref(struct Signature *S, const struct Ref *R) {
    unsigned int total, complete;
    unsigned int *match1, *match2, mc1, mc2, *new_order;
    double **new_means, ***new_vars;
    char **group_bandrefs, **mismatches, **new_bandrefs;

    /* Safety measure. Untranslated as this should not happen in production! */
    if (S->nbands < 1 || R->nfiles < 1)
        G_fatal_error("Programming error. Invalid length structs passed to "
                      "I_sort_signatures_by_bandref(%d, %d);", S->nbands,  R->nfiles);

    /* Obtain group band references */
    group_bandrefs = (char **)G_malloc(R->nfiles * sizeof(char **));
    for (unsigned int j = R->nfiles; j--;) {
        group_bandrefs[j] = Rast_read_bandref(R->file[j].name, R->file[j].mapset);
    }

    /* If lengths are not equal, there will be a mismatch */
    complete = S->nbands == R->nfiles;

    /* Initialize match tracker */
    new_order = (unsigned int *)G_malloc(S->nbands * sizeof(unsigned int));
    match1 = (unsigned int *)G_calloc(S->nbands, sizeof(unsigned int));
    match2 = (unsigned int *)G_calloc(R->nfiles, sizeof(unsigned int));

    /* Allocate memory for temporary storage of sorted values */
    new_bandrefs = (char **)G_malloc(S->nbands * sizeof(char **));
    new_means = (double **)G_malloc(S->nbands * sizeof(double *));
    new_vars = (double ***)G_malloc(S->nbands * sizeof(double **));
    for (unsigned int c = S->nsigs; c--;) {
        new_means[c] = (double *)G_malloc(S->nbands * sizeof(double));
        new_vars[c] = (double **)G_malloc(S->nbands * sizeof(double *));
        for (unsigned int i = S->nbands; i--;)
            new_vars[c][i] = (double *)G_malloc(S->nbands * sizeof(double));
    }

    /* Obtain order of matching items */
    for (unsigned int j = R->nfiles; j--;) {
        for (unsigned int i = S->nbands; i--;) {
            if (S->bandrefs[i] && group_bandrefs[j] &&
                !strcmp(S->bandrefs[i], group_bandrefs[j])) {
                    if (complete) {
                        /* Reorder pointers to existing strings only */
                        new_bandrefs[j] = S->bandrefs[i];
                        new_order[i] = j;
                    }
                    /* Keep a track of matching items for error reporting */
                    match1[i] = 1;
                    match2[j] = 1;
                    break;
            }
        }
    }

    /* Check for band reference mismatch */
    mc1 = mc2 = 0;
    mismatches = (char **)G_malloc(2 * sizeof(char **));
    mismatches[0] = NULL;
    mismatches[1] = NULL;
    total = 1;
    for (unsigned int i = 0; i < S->nbands; i++) {
        if (!match1[i]) {
            if (S->bandrefs[i])
                total = total + strlen(S->bandrefs[i]);
            else
                total = total + 24;
            mismatches[0] = (char *)G_realloc(mismatches[0], total * sizeof(char *));
            if (mc1)
                strcat(mismatches[0], ",");
            else
                mismatches[0][0] = '\0';
            if (S->bandrefs[i])
                strcat(mismatches[0], S->bandrefs[i]);
            else
                strcat(mismatches[0], "<band reference missing>");
            mc1++;
            total = total + 1;
        }
    }
    total = 1;
    for (unsigned int j = 0; j < R->nfiles; j++) {
        if (!match2[j]) {
            if (group_bandrefs[j])
                total = total + strlen(group_bandrefs[j]);
            else
                total = total + 24;
            mismatches[1] = (char *)G_realloc(mismatches[1], total * sizeof(char *));
            if (mc2)
                strcat(mismatches[1], ",");
            else
                mismatches[1][0] = '\0';
            if (group_bandrefs[j])
                strcat(mismatches[1], group_bandrefs[j]);
            else
                strcat(mismatches[1], "<band reference missing>");
            mc2++;
            total = total + 1;
        }
    }

    /* Swap var matrix values in each of classes */
    if (!mc1 && !mc2) {
        for (unsigned int c = S->nsigs; c--;) {
            for (unsigned int b1 = 0; b1 < S->nbands; b1++) {
                new_means[c][new_order[b1]] = S->sig[c].mean[b1];
                for (unsigned int b2 = 0; b2 <= b1; b2++) {
                    if (new_order[b1] > new_order[b2]) {
                        new_vars[c][new_order[b1]][new_order[b2]] = S->sig[c].var[b1][b2];
                    }
                    else {
                        new_vars[c][new_order[b2]][new_order[b1]] = S->sig[c].var[b1][b2];
                    }
                }
            }
        }

        /* Replace values in struct with ordered ones */
        memcpy(S->bandrefs, new_bandrefs, S->nbands * sizeof(char **));
        for (unsigned int c = S->nsigs; c--;) {
            memcpy(S->sig[c].mean, new_means[c], S->nbands * sizeof(double));
            for (unsigned int i = S->nbands; i--;)
                memcpy(S->sig[c].var[i], new_vars[c][i], S->nbands * sizeof(double));
        }
    }

    /* Clean up */
    for (unsigned int j = R->nfiles; j--;)
        free(group_bandrefs[j]);
    free(group_bandrefs);
    free(new_order);
    free(match1);
    free(match2);
    free(new_bandrefs);
    for (unsigned int c = S->nsigs; c--;) {
        free(new_means[c]);
        for (unsigned int i = S->nbands; i--;)
            free(new_vars[c][i]);
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
