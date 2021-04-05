#include <stdlib.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

/*!
 * \brief Initialize struct Signature before use
 *
 * Pass 0 as nbands before call to I_read_signatures as nbands might
 * not be known yet.
 *
 * \param *Signature to initialize
 * \param nbands band (imagery group member) count
 */
int I_init_signatures(struct Signature *S, int nbands)
{
    S->nbands = nbands;
    S->bandrefs = (char **)G_malloc(nbands * sizeof(char **));
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
 * It is up to calee to fclose the file stream afterwards.
 *
 * \param pointer to FILE*
 * \param pointer to struct Signature *S
 *
 * \return 1 on success, -1 on failure
 */
int I_read_signatures(FILE * fd, struct Signature *S)
{
    int ver;
    int n;
    int step, pos, alloced, bandref;
    char c, prev;
    step = 8;
    pos = 0;
    alloced = 0;
    bandref = 0;

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
    if (S->nbands == 0)
        S->bandrefs = (char **)G_realloc(S->bandrefs, sizeof(char **));
    S->bandrefs[bandref] = (char *)G_malloc((step + 1) * sizeof(char *));
    alloced = step; /* + 1 for '\0' */
    while ((c = (char)fgetc(fd)) != EOF) {
        if (c == '\n') {
            if (prev != ' ') {
                S->bandrefs[bandref][pos] = '\0';
                bandref++;
            }
            S->nbands = bandref;
            G_free(S->bandrefs[bandref]);
            break;
        }
        if (pos == alloced) {
            alloced = alloced + step;
            S->bandrefs[bandref] = (char *)G_realloc(S->bandrefs[bandref], alloced * sizeof(char *));
        }
        if (c == ' ') {
            S->bandrefs[bandref][pos] = '\0';
            bandref++;
            /* bandref are 0 based thus: (bandref + 1) */
            S->bandrefs = (char **)G_realloc(S->bandrefs, (bandref + 1) * sizeof(char **));
            S->bandrefs[bandref] = (char *)G_malloc((step + 1) * sizeof(char *));
            alloced = step; /* + 1 for '\0' */
            pos = 0;
            prev = c;
            continue;
        }
        S->bandrefs[bandref][pos] = c;
        pos++;
        prev = c;
    }

    if (! S->nbands > 0) {
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
 * It is up to calee to fclose the file stream afterwards.
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
    /* Label for each class repersented by this signature */
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
