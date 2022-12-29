
/**
   \file lib/gis/ls.c

   \brief Functions to list the files in a directory.

   \author Paul Kelly
   
   (C) 2007, 2008 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/config.h>
#include <grass/glocale.h>

#ifdef HAVE_TERMIOS_H
#  include <termios.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif

typedef int ls_filter_func(const char * /*filename */ , void * /*closure */ );

static struct state {
    ls_filter_func *ls_filter;
    void *ls_closure;
    ls_filter_func *ls_ex_filter;
    void *ls_ex_closure;
} state;

static struct state *st = &state;

static int cmp_names(const void *aa, const void *bb)
{
    char *const *a = (char *const *)aa;
    char *const *b = (char *const *)bb;

    return strcmp(*a, *b);
}

/**
 * \brief Sets a function and its complementary data for G_ls2 filtering.
 *
 * Defines a filter function and its rule data that allow G_ls2 to filter out
 * unwanted file names.  Call this function before G_ls2.
 *
 * \param func      Filter callback function to compare a file name and closure
 * 		    pattern (if NULL, no filter will be used).
 * 		    func(filename, closure) should return 1 on success, 0 on
 * 		    failure.
 * \param closure   Data used to determine if a file name matches the rule.
 **/

void G_set_ls_filter(ls_filter_func *func, void *closure)
{
    st->ls_filter = func;
    st->ls_closure = closure;
}

void G_set_ls_exclude_filter(ls_filter_func *func, void *closure)
{
    st->ls_ex_filter = func;
    st->ls_ex_closure = closure;
}

/**
 * \brief Stores a sorted directory listing in an array
 * 
 * The filenames in the specified directory are stored in an array of
 * strings, then sorted alphabetically. Each filename has space allocated
 * using G_store(), which can be freed using G_free() if necessary. The
 * same goes for the array itself.
 * 
 * 
 * \param dir       Directory to list
 * \param num_files Pointer to an integer in which the total number of
 *                  files listed will be stored
 * 
 * \return          Pointer to array of strings containing the listing
 **/

char **G_ls2(const char *dir, int *num_files)
{
    struct dirent *dp;
    DIR *dfd;
    char **dir_listing = NULL;
    int n = 0;

    if ((dfd = opendir(dir)) == NULL)
	G_fatal_error(_("Unable to open directory %s"), dir);

    while ((dp = readdir(dfd)) != NULL) {
	if (dp->d_name[0] == '.')	/* Don't list hidden files */
	    continue;
	if (st->ls_filter && !(*st->ls_filter)(dp->d_name, st->ls_closure))
	    continue;
	if (st->ls_ex_filter && (*st->ls_ex_filter)(dp->d_name, st->ls_ex_closure))
	    continue;
	dir_listing = (char **)G_realloc(dir_listing, (1 + n) * sizeof(char *));
	dir_listing[n] = G_store(dp->d_name);
	n++;
    }
    closedir(dfd);

    /* Sort list of filenames alphabetically */
    qsort(dir_listing, n, sizeof(char *), cmp_names);

    *num_files = n;
    return dir_listing;
}

/**
 * \brief Prints a directory listing to a stream, in prettified column format
 * 
 * A replacement for system("ls -C"). Lists the contents of the directory
 * specified to the given stream, e.g. stderr. Tries to determine an 
 * appropriate column width to keep the number of lines used to a minimum
 * and look pretty on the screen.
 * 
 * \param dir    Directory to list
 * \param stream Stream to print listing to
 **/

void G_ls(const char *dir, FILE * stream)
{
    int i, n;
    char **dir_listing = G_ls2(dir, &n);

    G_ls_format(dir_listing, n, 0, stream);

    for (i = 0; i < n; i++)
	G_free(dir_listing[i]);

    G_free(dir_listing);
}

/**
 * \brief Prints a listing of items to a stream, in prettified column format
 * 
 * Lists the contents of the array passed to the given stream, e.g. stderr.
 * Prints the number of items specified by "perline" to each line, unless
 * perline is given as 0 in which case the function tries to determine an 
 * appropriate column width to keep the number of lines used to a minimum
 * and look pretty on the screen.
 * 
 * \param list      Array of strings containing items to be printed
 * \param num_items Number of items in the array
 * \param perline   Number of items to print per line, 0 for autodetect
 * \param stream    Stream to print listing to
 **/

void G_ls_format(char **list, int num_items, int perline, FILE * stream)
{
    int i;

    int field_width, column_height;
    int screen_width = 80;	/* Default width of 80 columns */

    if (num_items < 1)
	return;			/* Nothing to print */

#ifdef TIOCGWINSZ
    /* Determine screen_width if possible */
    {
	struct winsize size;

	if (ioctl(fileno(stream), TIOCGWINSZ, (char *)&size) == 0)
	    screen_width = size.ws_col;
    }
#endif

    if (perline == 0) {
	int max_len = 0;

	for (i = 0; i < num_items; i++) {
	    /* Find maximum filename length */
	    if (strlen(list[i]) > max_len)
		max_len = strlen(list[i]);
	}
	/* Auto-fit the number of items that will
	 * fit per line (+1 because of space after item) */
	perline = screen_width / (max_len + 1);
	if (perline < 1)
	    perline = 1;
    }

    /* Field width to accommodate longest filename */
    field_width = screen_width / perline;
    /* Longest column height (i.e. num_items <= perline * column_height) */
    column_height = (num_items / perline) + ((num_items % perline) > 0);

    {
	const int max
	    = num_items + column_height - (num_items % column_height);
	char **next;

	for (i = 1, next = list; i <= num_items; i++) {
	    char **cur = next;

	    next += column_height;
	    if (next >= list + num_items) {
		/* the next item has to be on the other line */
		next -= (max - 1 - (next < list + max ? column_height : 0));
		fprintf(stream, "%s\n", *cur);
	    }
	    else {
		fprintf(stream, "%-*s", field_width, *cur);
	    }
	}
    }
}
