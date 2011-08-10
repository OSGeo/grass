#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include "G3d_intern.h"

/*---------------------------------------------------------------------------*/

static int
G3d_readWriteWindow(struct Key_Value *windowKeys, int doRead, int *proj,
		    int *zone, double *north, double *south, double *east,
		    double *west, double *top, double *bottom, int *rows,
		    int *cols, int *depths, double *ew_res, double *ns_res,
		    double *tb_res)
{
    int returnVal;
    int (*windowInt) (), (*windowDouble) ();

    if (doRead) {
	windowDouble = G3d_keyGetDouble;
	windowInt = G3d_keyGetInt;
    }
    else {
	windowDouble = G3d_keySetDouble;
	windowInt = G3d_keySetInt;
    }

    returnVal = 1;
    returnVal &= windowInt(windowKeys, G3D_REGION_PROJ, proj);
    returnVal &= windowInt(windowKeys, G3D_REGION_ZONE, zone);

    returnVal &= windowDouble(windowKeys, G3D_REGION_NORTH, north);
    returnVal &= windowDouble(windowKeys, G3D_REGION_SOUTH, south);
    returnVal &= windowDouble(windowKeys, G3D_REGION_EAST, east);
    returnVal &= windowDouble(windowKeys, G3D_REGION_WEST, west);
    returnVal &= windowDouble(windowKeys, G3D_REGION_TOP, top);
    returnVal &= windowDouble(windowKeys, G3D_REGION_BOTTOM, bottom);

    returnVal &= windowInt(windowKeys, G3D_REGION_ROWS, rows);
    returnVal &= windowInt(windowKeys, G3D_REGION_COLS, cols);
    returnVal &= windowInt(windowKeys, G3D_REGION_DEPTHS, depths);

    returnVal &= windowDouble(windowKeys, G3D_REGION_EWRES, ew_res);
    returnVal &= windowDouble(windowKeys, G3D_REGION_NSRES, ns_res);
    returnVal &= windowDouble(windowKeys, G3D_REGION_TBRES, tb_res);

    if (returnVal)
	return 1;

    G3d_error("G3d_readWriteWindow: error writing window");
    return 0;
}

/*
 * If windowName == NULL -> G3D_WINDOW_ELEMENT ("$MAPSET/WIND3")
 * otherwise G3D_WINDOW_DATABASE ("$MAPSET/windows3d/$NAME")
 */
static void G3d_getFullWindowPath(char *path, const char *windowName)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    if (windowName == NULL) {
	G_file_name(path, "", G3D_WINDOW_ELEMENT, G_mapset());
	return;
    }

    while (*windowName == ' ')
	windowName++;

    if ((*windowName == '/') || (*windowName == '.')) {
	sprintf(path, windowName);
	return;
    }

    if (G_name_is_fully_qualified(windowName, xname, xmapset)) {
	G_file_name(path, G3D_WINDOW_DATABASE, xname, xmapset);
	return;
    }

    G_file_name(path, G3D_WINDOW_DATABASE, windowName, G_mapset());
}

/*---------------------------------------------------------------------------*/
/*
   static void
   G3d_getWindowLocation (path, windowName)

   char path[1024];
   char *windowName;

   {
   char xname[512], xmapset[512];
   char *p, *slash;

   if (windowName == NULL) {
   G_file_name (path, "", "", G_mapset ());
   return;
   }

   while (*windowName == ' ') windowName++;

   if ((*windowName != '/') && (*windowName != '.')) {
   if (G_name_is_fully_qualified (windowName, xname, xmapset)) 
   G_file_name (path, G3D_WINDOW_DATABASE, xname, xmapset);
   else
   G_file_name (path, G3D_WINDOW_DATABASE, windowName, G_mapset ());
   } else
   sprintf (path, windowName);
   p = path;
   slash = NULL;
   while (*p != 0) {
   if (*p == '/') slash = p;
   p++;
   }
   if (slash != NULL) *slash = 0;
   }
 */

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 *  Reads
 * <em>window</em> from the file specified by <em>windowName</em>. The name is
 * converted by the rules defined in window defaults. A NULL pointer indicates
 * the <em>WIND3</em> file in the current mapset.
 *
 *  \param window
 *  \param windowName
 *  \return 1 ... if successful
 *          0 ... otherwise.
 */

int G3d_readWindow(G3D_Region * window, const char *windowName)
{
    struct Cell_head win;
    struct Key_Value *windowKeys;
    char path[GPATH_MAX];


    if (windowName == NULL) {
	G_get_window(&win);

	window->proj = win.proj;
	window->zone = win.zone;
	window->north = win.north;
	window->south = win.south;
	window->east = win.east;
	window->west = win.west;
	window->top = win.top;
	window->bottom = win.bottom;
	window->rows = win.rows3;
	window->cols = win.cols3;
	window->depths = win.depths;
	window->ns_res = win.ns_res3;
	window->ew_res = win.ew_res3;
	window->tb_res = win.tb_res;
    }
    else {
	G3d_getFullWindowPath(path, windowName);

	if (access(path, R_OK) != 0) {
	    G_warning("G3d_readWindow: unable to find [%s].", path);
	    return 0;
	}

	windowKeys = G_read_key_value_file(path);

	if (!G3d_readWriteWindow(windowKeys, 1,
				 &(window->proj), &(window->zone),
				 &(window->north), &(window->south),
				 &(window->east), &(window->west),
				 &(window->top), &(window->bottom),
				 &(window->rows), &(window->cols),
				 &(window->depths), &(window->ew_res),
				 &(window->ns_res), &(window->tb_res))) {
	    G3d_error
		("G3d_readWindow: error extracting window key(s) of file %s",
		 path);
	    return 0;
	}

	G_free_key_value(windowKeys);
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
/* modified version of G__make_mapset_element */
/*
   static int
   G3d_createPath (thePath)

   char *thePath;

   {
   char command[1024];
   char *path, *p, *pOld;

   if (*thePath == 0) return 0;

   strcpy (path = command, "mkdir ");
   while (*path) path++;
   p = path;
 */
  /* now append element, one directory at a time, to path */
/*
   while (1) {
   if (*thePath == '/') *p++ = *thePath++;
   pOld = p;
   while ((*thePath) && (*thePath != '/')) *p++ = *thePath++;
   *p = 0;

   if (p == pOld) return 1;

   if (access (path, 0) != 0) mkdir (path,0777);
   if (access (path, 0) != 0) system (command);
   if (access (path, 0) != 0) {
   char err[1024];
   sprintf (err, "can't make mapset element %s (%s)", thePath, path);
   G_fatal_error (err);
   exit(1);
   }
   }
   }
 */

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * 
 * Writes <em>window</em> to the file specified by <em>windowName</em>. The name
 * is converted by the rules defined in window defaults. A NULL pointer
 * indicates the <em>WIND3</em> file in the current mapset.
 *
 *  \param window
 *  \param windowName
 *  \return 1 ... if successful
 *          0 ... otherwise.
 */

/*
   int
   G3d_writeWindow (window, windowName)

   G3D_Region *window;
   char *windowName;

   {
   return 0;
   }
 */

/*---------------------------------------------------------------------------*/


/*!
 * \brief 
 *
 * Allows the window to be set at run-time via the <em>region3</em>
 * command line argument. This function has to be called before
 * <em>G_parser ()</em>. See also window defaults.
 *
 *  \return void
 */

void G3d_useWindowParams(void)
{
    G3d_setWindowParams();
}
