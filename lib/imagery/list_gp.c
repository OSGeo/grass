#include <string.h>
#include <grass/imagery.h>

/*!
 * \brief Prints maps in a group (fancy version)
 *
 * \param group group name
 * \param ref group reference (set with I_get_group_ref())
 * \param fd where to print (typically stdout)
 * \return 0
 */
int I_list_group (
    const char *group,
    const struct Ref *ref,
    FILE *fd)
{
    char buf[80];
    int i;
    int len, tot_len;
    int max;

    if (ref->nfiles <= 0)
    {
	fprintf (fd, "group [%s] is empty\n", group);
	return 0;
    }
    max = 0;
    for (i=0; i < ref->nfiles; i++)
    {
	sprintf (buf, "%s in %s", ref->file[i].name, ref->file[i].mapset);
	len = strlen(buf)+4;
	if (len > max) max = len;
    }
    fprintf (fd, "group [%s] references the following cellfiles\n", group);
    fprintf (fd, "-------------\n");
    tot_len = 0;
    for (i=0; i < ref->nfiles; i++)
    {
	sprintf (buf, "%s in %s", ref->file[i].name, ref->file[i].mapset);
	tot_len += max;
	if (tot_len > 78)
	{
	    fprintf (fd, "\n");
	    tot_len = max;
	}
	fprintf (fd, "%-*s", max, buf);
    }
    if (tot_len)
	fprintf (fd, "\n");
    fprintf (fd, "-------------\n");

    return 0;
}

/*!
 * \brief Prints maps in a group (simple version)
 *
 * Same as I_list_group(), but without all the fancy stuff.
 * Prints one map per line in map@mapset form.
 *
 * \param ref group reference (set with I_get_group_ref())
 * \param fd where to print (typically stdout)
 * \return 0
 */
int I_list_group_simple (const struct Ref *ref, FILE *fd)
{
    int i;

    if (ref->nfiles <= 0)
	return 0;

    for (i=0; i < ref->nfiles; i++)
	fprintf(fd , "%s@%s\n", ref->file[i].name, ref->file[i].mapset);

    return 0;
}
