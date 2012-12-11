/*!
 * \file raster/history.c
 *
 * \brief Raster Library - History management
 *
 * (C) 2001-2009 GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdarg.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

/*!
 * \brief Append a string to a History structure
 *
 *
 * \param hist pointer to History structure which holds history info
 * \param str string to append
 *
 * \return void
 */
void Rast_append_history(struct History *hist, const char *str)
{
    hist->lines = G_realloc(hist->lines, (hist->nlines + 1) * sizeof(char *));
    hist->lines[hist->nlines++] = G_store(str);
}

/*!
 * \brief Append a formated string to a History structure
 *
 *
 * \param hist pointer to History structure which holds history info
 * \param fmt a string of format characters
 * \param ... the arguments associated with the format characters
 *
 * \return void
 */
void Rast_append_format_history(struct History *hist, const char *fmt, ...)
{
    va_list ap;
    char *str;

    hist->lines = G_realloc(hist->lines, (hist->nlines + 1) * sizeof(char *));

    va_start(ap, fmt);
    G_vasprintf(&str, fmt, ap);
    va_end(ap);

    hist->lines[hist->nlines++] = str;
}

int Rast__read_history(struct History *hist, FILE *fp)
{
    int i;

    for (i = 0; i < HIST_NUM_FIELDS; i++) {
	char buf[4096];

	if (!G_getl(buf, sizeof(buf), fp)) {
	    fclose(fp);
	    return -1;
	}

	G_ascii_check(buf);

	hist->fields[i] = G_store(buf);
    }

    hist->nlines = 0;

    for (;;) {
	char buf[4096];
	if (!G_getl(buf, sizeof(buf), fp))
	    break;
	Rast_append_history(hist, buf);
    }

    fclose(fp);

    return 0;
}

/*!
 * \brief Read raster history file
 *
 * This routine reads the history file for the raster map <i>name</i>
 * in <i>mapset</i> into the <i>hist</i> structure.
 *
 * A diagnostic message is printed and -1 is returned if there is an
 * error reading the history file. Otherwise, 0 is returned.
 *
 * \param name map name
 * \param mapset mapset name
 * \param hist pointer to History structure which holds history info
 *
 * \return -1 on error
 * \return 0 on success
 */
int Rast_read_history(const char *name, const char *mapset,
		      struct History *hist)
{
    FILE *fp;

    G_zero(hist, sizeof(struct History));

    fp = G_fopen_old("hist", name, mapset);
    if (!fp) {
	G_warning(_("Unable to get history information for <%s@%s>"),
		  name, mapset);
	return -1;
    }

    if (Rast__read_history(hist, fp) == 0)
	return 0;

    G_warning(_("Unable to get history information for <%s@%s>"),
	      name, mapset);
    return -1;
}

void Rast__write_history(struct History *hist, FILE *fp)
{
    int i;

    for (i = 0; i < HIST_NUM_FIELDS; i++)
	fprintf(fp, "%s\n", hist->fields[i] ? hist->fields[i] : "");

    for (i = 0; i < hist->nlines; i++)
	fprintf(fp, "%s\n", hist->lines[i]);

    fclose(fp);
}

/*!
 * \brief Write raster history file
 *
 * This routine writes the history file for the raster map
 * <i>name</i> in the current mapset from the <i>hist</i> structure.
 *
 * A diagnostic message is printed and -1 is returned if there is an
 * error writing the history file. Otherwise, 0 is returned.
 *
 * <b>Note:</b> The <i>hist</i> structure should first be initialized
 * using Rast_short_history().
 *
 * \param name map name
 * \param[out] hist pointer to History structure which holds history info
 *
 * \return void
 */
void Rast_write_history(const char *name, struct History *hist)
{
    FILE *fp = G_fopen_new("hist", name);
    if (!fp)
	G_fatal_error(_("Unable to write history information for <%s>"), name);

    Rast__write_history(hist, fp);
}


/*!
 * \brief Set the string of a specific history field
 *
 *
 * \param hist pointer to History structure which holds history info
 * \param field number of a specific history field, should be accessed with macros (HIST_MAPID, ...)
 *
 * \return string of the history field
 */
const char *Rast_get_history(struct History *hist, int field)
{
    return hist->fields[field];
}

/*!
 * \brief Set the string of a specific history field
 *
 *
 * \param hist pointer to History structure which holds history info
 * \param field number of a specific history field, should be accessed with macros (HIST_MAPID, ...)
 * \param str string of the history field
 *
 * \return void
 */
void Rast_set_history(struct History *hist, int field, const char *str)
{
    if (hist->fields[field])
	G_free(hist->fields[field]);
    hist->fields[field] = str ? G_store(str) : NULL;
}

void Rast_format_history(struct History *hist, int field, const char *fmt, ...)
{
    va_list ap;

    if (hist->fields[field])
	G_free(hist->fields[field]);

    va_start(ap, fmt);
    G_vasprintf(&hist->fields[field], fmt, ap);
    va_end(ap);
}

/*!
 * \brief Initialize history structure
 *
 * This routine initializes the <i>hist</i> structure, recording the
 * date, user, module name and the raster map <i>name</i>
 * structure. The <i>type</i> is an anachronism from earlier versions
 * of GRASS and should be specified as "raster".
 *
 * <b>Note:</b> This routine only initializes the data structure. It
 * does not write the history file.
 *
 * \param name map name
 * \param type map type
 * \param hist pointer to History structure which holds history info
 */
void Rast_short_history(const char *name, const char *type,
			struct History *hist)
{
    G_zero(hist, sizeof(struct History));
    Rast_set_history(hist, HIST_MAPID, G_date());
    Rast_set_history(hist, HIST_TITLE, name);
    Rast_set_history(hist, HIST_MAPSET, G_mapset());
    Rast_set_history(hist, HIST_CREATOR, G_whoami());
    Rast_set_history(hist, HIST_MAPTYPE, type);
    Rast_format_history(hist, HIST_KEYWRD, _("generated by %s"), G_program_name());
    Rast_set_history(hist, HIST_DATSRC_1, "");
    Rast_set_history(hist, HIST_DATSRC_2, "");
    hist->nlines = 0;
}

/*!
 * \brief Save command line to raster history structure
 *
 * This routine takes an existing (run Rast_short_history first() history
 * structure and adds the command line to the end of the comments
 * array, as cleaned & expanded by the parser.
 *
 *  - First version had for loops of [i][j] character assignments and ending
 *    nulls, but using the string libraries is cleaner and less bug prone.
 *  - Second version had white space detection, intelligent wrapping, and
 *    indentation of continued lines, but this proved a pain in the neck for 
 *    things like r.patch which can have long strings without any
 *    parser-acceptable breaks.
 *  - This is MK-III, simplified, but that's good: it's cut & paste-able.
 *
 * Note: use Rast_write_history() to write the structure.
 *
 * Sample Usage:
 * \code
 *   struct History history;
 *   Rast_short_history(rasterfile, "raster", &history);
 *   Rast_command_history(&history);
 *   Rast_write_history(rasterfile, &history);
 * \endcode
 *
 * \param hist pointer to History structure which holds history info
 *
 * \return 0 on success
 * \return 1 on failure (history file full, no change)
 * \return 2 on failure (history file full, added as much as we could)
 */
int Rast_command_history(struct History *hist)
{
    char *cmdlin;
    int cmdlen;

    cmdlin = G_recreate_command();
    cmdlen = strlen(cmdlin);

    if (hist->nlines > 0)	/* add a blank line if preceding history exists */
	Rast_append_history(hist, "");

    if (cmdlen < 70)		/* ie if it will fit on a single line */
	Rast_append_history(hist, cmdlin);
    else {			/* multi-line required */
	int j;			/* j is the current position in the command line string */
	for (j = 0; cmdlen - j > 70; j += 68) {
	    char buf[80];
	    memcpy(buf, &cmdlin[j], 68);
	    buf[68] = '\\';
	    buf[69] = '\0';
	    Rast_append_history(hist, buf);
	}
	if (cmdlen - j > 0)	/* ie anything left */
	    Rast_append_history(hist, &cmdlin[j]);
    }

    G_free(cmdlin);

    return 0;
}

void Rast_clear_history(struct History *hist)
{
    int i;

    for (i = 0; i < hist->nlines; i++)
	G_free(hist->lines[i]);

    if (hist->lines)
	G_free(hist->lines);

    hist->lines = NULL;
    hist->nlines = 0;
}

void Rast_free_history(struct History *hist)
{
    int i;

    for (i = 0; i < HIST_NUM_FIELDS; i++)
	if (hist->fields[i]) {
	    G_free(hist->fields[i]);
	    hist->fields[i] = NULL;
	}

    Rast_clear_history(hist);
}

int Rast_history_length(struct History *hist)
{
    return hist->nlines;
}

const char *Rast_history_line(struct History *hist, int line)
{
    if (line < 0 || line >= hist->nlines)
	return "";
    return hist->lines[line];
}

