/**
   \file init.cpp
   
   \brief Experimental C++ wxWidgets Nviz prototype -- initialization

   Used by wxGUI Nviz extension.

   Copyright: (C) by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
*/

#include "nviz.h"

#define MSG  0
#define WARN 1
#define ERR  2

static void swap_gl();
static int print_error(const char *, const int);
static int print_percent(int);
static void print_sentence (PyObject*, const int, const char *);
static PyObject *logStream;
static int message_id = 1;

/*!
  \brief Initialize Nviz class instance
*/
Nviz::Nviz(PyObject *log)
{
    G_gisinit(""); /* GRASS functions */

    logStream = log;

    G_set_error_routine(&print_error);
    // TODO
    // G_set_percent_routine(&print_percent);

    GS_libinit();
    /* GVL_libinit(); TODO */

    GS_set_swap_func(swap_gl);

    data = (nv_data*) G_malloc(sizeof (nv_data));

    /* GLCanvas */
    glCanvas = NULL;

    G_debug(1, "Nviz::Nviz()");
}

/*!
  \brief Destroy Nviz class instance
*/
Nviz::~Nviz()
{
    G_unset_error_routine();
    // TODO
    // G_unset_percent_routine();

    G_free((void *) data);

    data = NULL;
    glCanvas = NULL;

    logStream = NULL;
}

/*!
  \brief Associate display with render window

  \return 1 on success
  \return 0 on failure
*/
int Nviz::SetDisplay(void *display)
{
    glCanvas = (wxGLCanvas *) display;
    // glCanvas->SetCurrent();

    G_debug(1, "Nviz::SetDisplay()");

    return 1;
}

void Nviz::InitView()
{
    /* initialize nviz data */
    Nviz_init_data(data);

    /* define default attributes for map objects */
    Nviz_set_surface_attr_default();
    /* set background color */
    Nviz_set_bgcolor(data, Nviz_color_from_str("white")); /* TODO */

    /* initialize view */
    Nviz_init_view();

    /* set default lighting model */
    SetLightsDefault();

    /* clear window */
    GS_clear(data->bgcolor);

    G_debug(1, "Nviz::InitView()");

    return;
}

/*!
  \brief Reset session

  Unload all data layers

  @todo vector, volume
*/
void Nviz::Reset()
{
    int i;
    int *surf_list, nsurfs;

    surf_list = GS_get_surf_list(&nsurfs);
    for (i = 0; i < nsurfs; i++) {
	GS_delete_surface(surf_list[i]);
    }
}

void swap_gl()
{
    return;
}

/*!
  \brief Set background color

  \param color_str color string
*/
void Nviz::SetBgColor(const char *color_str)
{
    data->bgcolor = Nviz_color_from_str(color_str);

    return;
}

int print_error(const char *msg, const int type)
{
    if (logStream) {
	print_sentence(logStream, type, msg);
    }
    else {
	fprintf(stderr, "Nviz: %s\n", msg);
    }

    return 0;
}

/*
  \brief Print one message, prefix inserted before each new line

  From lib/gis/error.c
*/
void print_sentence (PyObject *pyFd, const int type, const char *msg)
{
    char prefix[256];
    const char *start;
    char* sentence;

    switch ( type ) {
	case MSG: 
    	    sprintf (prefix, "GRASS_INFO_MESSAGE(%d,%d): ", getpid(), message_id);
	    break;
	case WARN:
    	    sprintf (prefix, "GRASS_INFO_WARNING(%d,%d): ", getpid(), message_id);
	    break;
	case ERR:
    	    sprintf (prefix, "GRASS_INFO_ERROR(%d,%d): ", getpid(), message_id);
	    break;
    }

    start = msg;

    PyFile_WriteString("\n", pyFd);

    while ( *start != '\0' ) {
	const char *next = start;

	PyFile_WriteString(prefix, pyFd);

	while ( *next != '\0' ) {
	    next++;
		
	    if ( *next == '\n' ) {
	        next++;
		break;
	    }
	}

	sentence = (char *) G_malloc ((next - start + 1) * sizeof (char));
	strncpy(sentence, start, next - start + 1);
	sentence[next-start] = '\0';

	PyFile_WriteString(sentence, pyFd);
	G_free((void *)sentence);

	PyFile_WriteString("\n", pyFd);
	start = next;
    }

    PyFile_WriteString("\n", pyFd);
    sprintf(prefix, "GRASS_INFO_END(%d,%d)\n", getpid(), message_id);
    PyFile_WriteString(prefix, pyFd);

    message_id++;
}

int print_percent(int x)
{
    char msg[256];

    sprintf(msg, "GRASS_INFO_PERCENT: %d\n", x);
    PyFile_WriteString(msg, logStream);

    return 0;
}
