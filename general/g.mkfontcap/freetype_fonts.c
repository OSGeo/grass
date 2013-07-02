
/****************************************************************************
 *
 * MODULE:       g.mkfontcap
 * AUTHOR(S):    Paul Kelly
 * PURPOSE:      Generates the font configuration file by scanning various
 *               directories for GRASS stroke and Freetype-compatible fonts.
 *              
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/fontcap.h>

#include "local_proto.h"

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library ftlibrary;

static void find_fonts(const char *);

#endif /* HAVE_FT2BUILD_H */

/**
 * \brief Find Freetype fonts and store them in a global GFONT_CAP struct
 * 
 * The directories specified by the global variables searchdirs and 
 * numsearchdirs are recursively scanned to find all Freetype-compatible
 * fonts. As each font is found, information on it is transferred into 
 * the global GFONT_CAP struct, fontcap, to be used by the main program.
 **/

void find_freetype_fonts(void)
{
#ifdef HAVE_FT2BUILD_H
    int i;

    if (FT_Init_FreeType(&ftlibrary) != 0)
	G_fatal_error("Unable to initialise Freetype");

    for (i = 0; i < numsearchdirs; i++)
	find_fonts(searchdirs[i]);

    FT_Done_FreeType(ftlibrary);
#endif /* HAVE_FT2BUILD_H */
    return;

}

#ifdef HAVE_FT2BUILD_H

/**
 * \brief Recursively scans a specified directory and stores information on
 *        all font files found.
 * 
 * Any directories found are recursively scanned in the same way as the 
 * parent. Any files found are attempted to be opened using FT_New_Face()
 * to discover if they are valid font files or not. If they are, information
 * on all the font faces in each file is stored in the global GFONT_CAP 
 * struct, fontcap, to be used by the main program. Recursive structure based 
 * on lib/init/clean_temp.c by Roberto Flor.
 * 
 * \param dirpath String containing directory to be scanned
 **/

static void find_fonts(const char *dirpath)
{
    char filepath[GPATH_MAX];
    DIR *curdir;
    struct dirent *cur_entry;
    struct stat info;

    curdir = opendir(dirpath);
    if (curdir == NULL)
	return;

    /* loop over current dir */
    while ((cur_entry = readdir(curdir))) {
	if (cur_entry->d_name[0] == '.')
	    continue;		/* Skip hidden files */

	sprintf(filepath, "%s%c%s", dirpath, HOST_DIRSEP, cur_entry->d_name);

	if (stat(filepath, &info))
	    continue;		/* File is unreadable */

	if (S_ISDIR(info.st_mode))
	    find_fonts(filepath);	/* Recurse into next directory */
	else {
	    /* It's a file; we'll try opening it with Freetype to see if
	     * it's a valid font. */
	    FT_Long index, facesinfile;
	    FT_Face face;

	    index = facesinfile = 0;

	    do {
		if (totalfonts >= maxfonts) {
		    maxfonts += 20;
		    fontcap =
			G_realloc(fontcap,
				  maxfonts * sizeof(struct GFONT_CAP));
		}

		if (FT_New_Face(ftlibrary, filepath, index, &face) == 0) {
		    facesinfile = face->num_faces;
		    /* Only use scalable fonts */
		    if (face->face_flags & FT_FACE_FLAG_SCALABLE) {
			char *buf_ptr;

			fontcap[totalfonts].path = G_store(filepath);
			fontcap[totalfonts].index = index;
			fontcap[totalfonts].type = GFONT_FREETYPE;
			fontcap[totalfonts].encoding = G_store("utf-8");

			if (strchr(filepath, HOST_DIRSEP))
			    buf_ptr = strrchr(filepath, HOST_DIRSEP) + 1;
			else
			    buf_ptr = filepath;
			if (strchr(buf_ptr, '.'))
			    *(strrchr(buf_ptr, '.')) = '\0';
			if (index > 0)
			    G_asprintf(&fontcap[totalfonts].name, "%s%d",
				       buf_ptr, (int)index);
			else
			    fontcap[totalfonts].name = G_store(buf_ptr);
			/* There might not be a style name but there will always be a
			 * family name. */
			if (face->style_name == NULL)
			    fontcap[totalfonts].longname =
				G_store(face->family_name);
			else
			    G_asprintf(&fontcap[totalfonts].longname, "%s %s",
				       face->family_name, face->style_name);
			totalfonts++;
		    }

		    /* Discard this FT_Face structure and use it again */
		    FT_Done_Face(face);

		}
	    } while (++index < facesinfile);

	}
    }

    closedir(curdir);

    return;
}

#endif /* HAVE_FT2BUILD_H */
