
/**********************************************************************
 *
 *  G3d_readHistory (name, mapset, hist)
 *      char *name                   name of map
 *      char *mapset                 mapset that map belongs to
 *      struct History *hist        structure to hold history info
 *
 *  Reads the history information associated with map layer "map"
 *  in mapset "mapset" into the structure "hist".
 *
 *   returns:    0  if successful
 *              -1  on fail
 *
 *  note:   a warning message is printed if the file is incorrect
 *
 **********************************************************************
 *
 *  G3d_writeHistory (name, hist)
 *      char *name                   name of map
 *      struct History *hist        structure holding history info
 *
 *  Writes the history information associated with map layer "map"
 *  into current from the structure "hist".
 *
 *   returns:    0  if successful
 *              -1  on fail
 **********************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include "G3d_intern.h"

/*simple error message */
void SimpleErrorMessage(FILE * fd, const char *name, const char *mapset)
{
    if (fd != NULL)
	fclose(fd);

    G_warning(_("can't get history information for [%s] in mapset [%s]"),
	      name, mapset);
    return;
}

/*!
 * \brief read raster3d History file
 *
 * This routine reads the History file for
 * the raster3d file <b>name</b> in <b>mapset</b> into the <b>History</b>
 * structure.
 * A diagnostic message is printed and -1 is returned if there is an error
 * reading the History file. Otherwise, 0 is returned.
 * A warning message is printed if the file is incorrect.
 *
 *  \param name
 *  \param mapset
 *  \param history
 *  \return int
 */

int G3d_readHistory(const char *name, const char *mapset,
		    struct History *hist)
/* This function is adapted from G_read_history */
{
    FILE *fd;
    char buff[1024], buf2[200], xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G_zero(hist, sizeof(struct History));

    /*this construct takes care of the correct history file path */
    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	sprintf(buff, "%s/%s", G3D_DIRECTORY, xname);
	sprintf(buf2, "%s@%s", G3D_HISTORY_ELEMENT, xmapset);	/* == hist@mapset */
    }
    else {
	sprintf(buff, "%s/%s", G3D_DIRECTORY, name);
	sprintf(buf2, "%s", G3D_HISTORY_ELEMENT);
    }

    if (!(fd = G_fopen_old(buff, buf2, mapset)))
	return -2;


    if (!G_getl(hist->mapid, sizeof(hist->mapid), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->mapid);

    if (!G_getl(hist->title, sizeof(hist->title), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->title);

    if (!G_getl(hist->mapset, sizeof(hist->mapset), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->mapset);

    if (!G_getl(hist->creator, sizeof(hist->creator), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->creator);

    if (!G_getl(hist->maptype, sizeof(hist->maptype), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->maptype);

    if (!G_getl(hist->datsrc_1, sizeof(hist->datsrc_1), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->datsrc_1);

    if (!G_getl(hist->datsrc_2, sizeof(hist->datsrc_2), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->datsrc_2);

    if (!G_getl(hist->keywrd, sizeof(hist->keywrd), fd)) {
	SimpleErrorMessage(fd, name, mapset);
	return -1;
    }
    G_ascii_check(hist->keywrd);

    hist->edlinecnt = 0;
    while ((hist->edlinecnt < MAXEDLINES) &&
	   (G_getl
	    (hist->edhist[hist->edlinecnt], sizeof(hist->edhist[0]), fd))) {
	G_ascii_check(hist->edhist[hist->edlinecnt]);
	hist->edlinecnt++;
    }

    fclose(fd);
    return 0;
}


/*!
 * \brief write raster3d History file
 *
 * This routine writes the History file for the raster3d file
 * <b>name</b> in the current mapset from the <b>History</b> structure.
 * A diagnostic message is printed and -1 is returned if there is an error
 * writing the History file. Otherwise, 0 is returned.
 * <b>Note.</b> The <b>history</b> structure should first be initialized
 * using <i>G_short_history.</i>
 *
 *  \param name
 *  \param history
 *  \return int
 */

int G3d_writeHistory(const char *name, struct History *hist)
/* This function is adapted from G_write_history */
{
    FILE *fd;
    int i;
    char buf[200], buf2[200], xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    if (G__name_is_fully_qualified(name, xname, xmapset)) {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, xname);
	sprintf(buf2, "%s@%s", G3D_HISTORY_ELEMENT, xmapset);	/* == hist@mapset */
    }
    else {
	sprintf(buf, "%s/%s", G3D_DIRECTORY, name);
	sprintf(buf2, "%s", G3D_HISTORY_ELEMENT);
    }

    if (!(fd = G_fopen_new(buf, buf2)))
	return -1;

    fprintf(fd, "%s\n", hist->mapid);
    fprintf(fd, "%s\n", hist->title);
    fprintf(fd, "%s\n", hist->mapset);
    fprintf(fd, "%s\n", hist->creator);
    fprintf(fd, "%s\n", hist->maptype);
    fprintf(fd, "%s\n", hist->datsrc_1);
    fprintf(fd, "%s\n", hist->datsrc_2);
    fprintf(fd, "%s\n", hist->keywrd);

    for (i = 0; i < hist->edlinecnt; i++)
	fprintf(fd, "%s\n", hist->edhist[i]);

    fclose(fd);
    return 0;
}
