#ifndef _GRASS_FREETYPECAP_H
#define _GRASS_FREETYPECAP_H

struct GFONT_CAP
{

    char *name;	    /**< Short name for this font face */

    char *longname; /**< Descriptive name for the font face */

    char *path;	    /**< Full path to the file containing this font face */

    int index;	    /**< Index within the file of this font face */

    int type;	    /**< Type of this font face (currently stroke or freetype) */

    char *encoding; /**< Encoding to be used with this font face. */
};

#define GFONT_STROKE 0
#define GFONT_FREETYPE 1

#endif
