
/****************************************************************************
 *
 * MODULE:       imagery library
 * AUTHOR(S):    Original author(s) name(s) unknown - written by CERL
 * PURPOSE:      Image processing library
 * COPYRIGHT:    (C) 1999, 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/**********************************************************
* I_get_group (group);
* I_put_group (group);
*
* I_get_group_ref (group, &Ref);
* I_put_group_ref (group, &Ref);
* I_get_subgroup_ref_file (group, subgroup, &Ref);
* I_put_subgroup_ref_file (group, subgroup, &Ref);
* I_add_file_to_group_ref (name, mapset, &Ref)
* I_transfer_group_ref_file (&Src_ref, n, &Dst_ref)
* I_init_group_ref (&Ref);
* I_free_group_ref (&Ref);
**********************************************************/

#include <string.h>
#include <stdlib.h>
#include <grass/imagery.h>

static int get_ref(const char *, const char *, const char *, struct Ref *);
static int set_color(const char *, const char *, const char *, struct Ref *);
static int put_ref(const char *, const char *, const struct Ref *);

/* get current group name from file GROUPFILE in current mapset */
int I_get_group(char *group)
{
    FILE *fd;
    int stat;

    *group = 0;
    G_suppress_warnings(1);
    fd = G_fopen_old("", GROUPFILE, G_mapset());
    G_suppress_warnings(0);
    if (fd == NULL)
	return 0;
    stat = (fscanf(fd, "%s", group) == 1);
    fclose(fd);
    return stat;
}

/* write group name to file GROUPFILE in current mapset */
int I_put_group(const char *group)
{
    FILE *fd;

    fd = G_fopen_new("", GROUPFILE);
    if (fd == NULL)
	return 0;
    fprintf(fd, "%s\n", group);
    fclose(fd);
    return 1;
}

/* get current subgroup for group in current mapset */
int I_get_subgroup(const char *group, char *subgroup)
{
    FILE *fd;
    int stat;

    *subgroup = 0;
    if (!I_find_group(group))
	return 0;
    G_suppress_warnings(1);
    fd = I_fopen_group_file_old(group, SUBGROUPFILE);
    G_suppress_warnings(0);
    if (fd == NULL)
	return 0;
    stat = (fscanf(fd, "%s", subgroup) == 1);
    fclose(fd);
    return stat;
}

/* write current subgroup to group in current mapset */
int I_put_subgroup(const char *group, const char *subgroup)
{
    FILE *fd;

    if (!I_find_group(group))
	return 0;
    fd = I_fopen_group_file_new(group, SUBGROUPFILE);
    if (fd == NULL)
	return 0;
    fprintf(fd, "%s\n", subgroup);
    fclose(fd);
    return 1;
}


/*!
 * \brief read group REF file
 *
 * Reads the contents of the REF file for the specified <b>group</b> into
 * the <b>ref</b> structure.
 * Returns 1 if successful; 0 otherwise (but no error messages are printed).
 *
 *  \param group
 *  \param ref
 *  \return int
 */

int I_get_group_ref(const char *group, struct Ref *ref)
{
    return get_ref(group, "", NULL, ref);
}


/*!
 * \brief read group REF file
 *
 * Reads the contents of the REF file for the specified <b>group</b> into
 * the <b>ref</b> structure.
 * Returns 1 if successful; 0 otherwise (but no error messages are printed).
 *
 *  \param group
 *  \param mapset
 *  \param ref
 *  \return int
 */

int I_get_group_ref2(const char *group, const char *mapset, struct Ref *ref)
{
    return get_ref(group, "", mapset, ref);
}


/*!
 * \brief read subgroup REF file
 *
 * Reads the contents of the REF file for the
 * specified <b>subgroup</b> of the specified <b>group</b> into the
 * <b>ref</b> structure.
 * Returns 1 if successful; 0 otherwise (but no error messages are printed).
 *
 *  \param group
 *  \param subgroup
 *  \param ref
 *  \return int
 */

int I_get_subgroup_ref(const char *group,
		       const char *subgroup, struct Ref *ref)
{
    return get_ref(group, subgroup, NULL, ref);
}


/*!
 * \brief read subgroup REF file
 *
 * Reads the contents of the REF file for the
 * specified <b>subgroup</b> of the specified <b>group</b> into the
 * <b>ref</b> structure.
 * Returns 1 if successful; 0 otherwise (but no error messages are printed).
 *
 *  \param group
 *  \param subgroup
 *  \param mapset
 *  \param ref
 *  \return int
 */
int I_get_subgroup_ref2(const char *group,
		       const char *subgroup, const char *mapset,
               struct Ref *ref)
{
    return get_ref(group, subgroup, mapset, ref);
}


static int get_ref(const char *group, const char *subgroup, const char *gmapset, struct Ref *ref)
{
    int n;
    char buf[1024];
    char name[INAME_LEN], mapset[INAME_LEN];
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    char color[20];
    FILE *fd;

    I_init_group_ref(ref);

    G_unqualified_name(group, gmapset, xname, xmapset);
    group = xname;
    gmapset = xmapset;

    if (gmapset == NULL || *gmapset == 0)
        gmapset = G_mapset();

    G_suppress_warnings(1);
    if (*subgroup == 0)
	fd = I_fopen_group_ref_old2(group, gmapset);
    else
	fd = I_fopen_subgroup_ref_old2(group, subgroup, gmapset);
    G_suppress_warnings(0);
    if (!fd)
	return 0;

    while (G_getl2(buf, sizeof buf, fd)) {
	n = sscanf(buf, "%255s %255s %15s", name, mapset, color);  /* better use INAME_LEN */
	if (n == 2 || n == 3) {
	    I_add_file_to_group_ref(name, mapset, ref);
	    if (n == 3)
		set_color(name, mapset, color, ref);
	}
    }
    /* make sure we have a color assignment */
    I_init_ref_color_nums(ref);

    fclose(fd);
    return 1;
}

static int set_color(const char *name, const char *mapset, const char *color,
		     struct Ref *ref)
{
    int n;

    for (n = 0; n < ref->nfiles; n++) {
	if (strcmp(ref->file[n].name, name) == 0
	    && strcmp(ref->file[n].mapset, mapset) == 0)
	    break;
    }

    if (n < ref->nfiles)
	while (*color) {
	    switch (*color) {
	    case 'r':
	    case 'R':
		if (ref->red.n < 0)
		    ref->red.n = n;
		break;
	    case 'g':
	    case 'G':
		if (ref->grn.n < 0)
		    ref->grn.n = n;
		break;
	    case 'b':
	    case 'B':
		if (ref->blu.n < 0)
		    ref->blu.n = n;
		break;
	    }
	    color++;
	}

    return 0;
}

int I_init_ref_color_nums(struct Ref *ref)
{
    ref->red.table = NULL;
    ref->grn.table = NULL;
    ref->blu.table = NULL;

    ref->red.index = NULL;
    ref->grn.index = NULL;
    ref->blu.index = NULL;

    if (ref->nfiles <= 0 || ref->red.n >= 0 || ref->blu.n >= 0 ||
	ref->blu.n >= 0)
	return 1;
    switch (ref->nfiles) {
    case 1:
	ref->red.n = 0;
	ref->grn.n = 0;
	ref->blu.n = 0;
	break;
    case 2:
	ref->blu.n = 0;
	ref->grn.n = 1;
	break;
    case 3:
	ref->blu.n = 0;
	ref->grn.n = 1;
	ref->red.n = 2;
	break;
    case 4:
	ref->blu.n = 0;
	ref->grn.n = 1;
	ref->red.n = 3;
	break;
    default:
	ref->blu.n = 1;
	ref->grn.n = 2;
	ref->red.n = 4;
	break;
    }

    return 0;
}


/*!
 * \brief write group REF file
 *
 * Writes the contents of the <b>ref</b> structure to the REF file for
 * the specified <b>group.</b>
 * Returns 1 if successful; 0 otherwise (and prints a diagnostic error).
 * <b>Note.</b> This routine will create the <b>group</b>, if it does not
 * already exist.
 *
 *  \param group
 *  \param ref
 *  \return int
 */

int I_put_group_ref(const char *group, const struct Ref *ref)
{
    return put_ref(group, "", ref);
}


/*!
 * \brief write subgroup REF file
 *
 * Writes the contents of the <b>ref</b>
 * structure into the REF file for the specified <b>subgroup</b> of the
 * specified <b>group.</b>
 * Returns 1 if successful; 0 otherwise (and prints a diagnostic error).
 * <b>Note.</b> This routine will create the <b>subgroup</b>, if it does not
 * already exist.
 *
 *  \param group
 *  \param subgroup
 *  \param ref
 *  \return int
 */

int I_put_subgroup_ref(const char *group, const char *subgroup,
		       const struct Ref *ref)
{
    return put_ref(group, subgroup, ref);
}

static int put_ref(const char *group, const char *subgroup,
		   const struct Ref *ref)
{
    int n;
    FILE *fd;

    if (*subgroup == 0)
	fd = I_fopen_group_ref_new(group);
    else
	fd = I_fopen_subgroup_ref_new(group, subgroup);
    if (!fd)
	return 0;

    for (n = 0; n < ref->nfiles; n++) {
	fprintf(fd, "%s %s", ref->file[n].name, ref->file[n].mapset);
	if (n == ref->red.n || n == ref->grn.n || n == ref->blu.n) {
	    fprintf(fd, " ");
	    if (n == ref->red.n)
		fprintf(fd, "r");
	    if (n == ref->grn.n)
		fprintf(fd, "g");
	    if (n == ref->blu.n)
		fprintf(fd, "b");
	}
	fprintf(fd, "\n");
    }
    fclose(fd);
    return 1;
}


/*!
 * \brief add file name to Ref structure
 *
 * This routine adds the file
 * <b>name</b> and <b>mapset</b> to the list contained in the <b>ref</b>
 * structure, if it is not already in the list.  The <b>ref</b> structure must
 * have been properly initialized. This routine is used by programs, such as
 * <i>i.maxlik</i>, to add to the group new raster maps created from files
 * already in the group.
 * Returns the index into the <i>file</i> array within the <b>ref</b>
 * structure for the file after insertion; see
 * Imagery_Library_Data_Structures.
 *
 *  \param name
 *  \param mapset
 *  \param ref
 *  \return int
 */

int I_add_file_to_group_ref(const char *name, const char *mapset,
			    struct Ref *ref)
{
    int n;

    for (n = 0; n < ref->nfiles; n++) {
	if (strcmp(ref->file[n].name, name) == 0
	    && strcmp(ref->file[n].mapset, mapset) == 0)
	    return n;
    }

    if ((n = ref->nfiles++))
	ref->file =
	    (struct Ref_Files *)G_realloc(ref->file,
					  ref->nfiles *
					  sizeof(struct Ref_Files));
    else
	ref->file =
	    (struct Ref_Files *)G_malloc(ref->nfiles *
					 sizeof(struct Ref_Files));
    strcpy(ref->file[n].name, name);
    strcpy(ref->file[n].mapset, mapset);
    return n;
}


/*!
 * \brief copy Ref lists
 *
 * This routine is used to copy file names from one
 * <i>Ref</i> structure to another. The name and mapset for file <b>n</b>
 * from the <b>src</b> structure are copied into the <b>dst</b> structure
 * (which must be properly initialized).
 * For example, the following code copies one <i>Ref</i> structure to another:
 \code
 struct Ref src,dst;
 int n;
 // some code to get information into <b>src</b>
 ...
 I_init_group_ref (&dst);
 for (n = 0; n < src.nfiles; n++)
 I_transfer_group_ref_file (&src, n, &dst);
 \endcode
 * This routine is used by <i>g.gui.gcp</i> to create the REF file for a
 * subgroup.
 *
 *  \param src
 *  \param n
 *  \param dst
 *  \return int
 */

int I_transfer_group_ref_file(const struct Ref *ref2, int n, struct Ref *ref1)
{
    int k;

    /* insert old name into new ref */
    k = I_add_file_to_group_ref(ref2->file[n].name, ref2->file[n].mapset,
				ref1);

    /* preserve color assignment */
    if (n == ref2->red.n)
	ref1->red.n = k;
    if (n == ref2->grn.n)
	ref1->grn.n = k;
    if (n == ref2->blu.n)
	ref1->blu.n = k;

    return 0;
}



/*!
 * \brief initialize Ref
 *       structure
 *
 * This routine initializes the <b>ref</b> structure for other
 * library calls which require a <i>Ref</i> structure. This routine must be
 * called before any use of the structure can be made.
 * <b>Note.</b> The routines I_get_group_ref and I_get_subgroup_ref call
 * this routine automatically.
 *
 *  \param ref
 *  \return int
 */

int I_init_group_ref(struct Ref *ref)
{
    ref->nfiles = 0;
    ref->red.n = ref->grn.n = ref->blu.n = -1;
    ref->red.table = ref->grn.table = ref->blu.table = NULL;

    return 0;
}


/*!
 * \brief free Ref structure
 *
 * This routine frees memory allocated to the <b>ref</b> structure.
 *
 *  \param ref
 *  \return int
 */

int I_free_group_ref(struct Ref *ref)
{
    if (ref->nfiles > 0)
	free(ref->file);
    ref->nfiles = 0;

    return 0;
}
