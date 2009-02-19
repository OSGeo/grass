/**
   \file nviz/init.cpp
   
   \brief wxNviz extension (3D view mode) - initialization

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   (C) 2008-2009 by Martin Landa, and the GRASS development team

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
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
    G_set_percent_routine(&print_percent);

    GS_libinit();
    GVL_libinit();

    GS_set_swap_func(swap_gl);

    data = (nv_data*) G_malloc(sizeof (nv_data));
    
    G_debug(1, "Nviz::Nviz()");
}

/*!
  \brief Destroy Nviz class instance
*/
Nviz::~Nviz()
{
    G_unset_error_routine();
    G_unset_percent_routine();

    G_free((void *) data);

    data = NULL;
    
    logStream = NULL;
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

/*
  \brief Print one message, prefix inserted before each new line

  From lib/gis/error.c
*/
void print_sentence (PyObject *pyFd, const int type, const char *msg)
{
    char prefix[256];
    const char *start;
    char* sentence;

    switch (type) {
    case MSG: 
	sprintf (prefix, "GRASS_INFO_MESSAGE(%d,%d): Nviz: ", getpid(), message_id);
	break;
    case WARN:
	sprintf (prefix, "GRASS_INFO_WARNING(%d,%d): Nviz: ", getpid(), message_id);
	break;
    case ERR:
	sprintf (prefix, "GRASS_INFO_ERROR(%d,%d): Nviz: ", getpid(), message_id);
	break;
    }

    start = msg;
    
    PyFile_WriteString("\n", pyFd);

    while (*start != '\0') {
	const char *next = start;
	
	PyFile_WriteString(prefix, pyFd);
	
	while ( *next != '\0' ) {
	    next++;
	    
	    if ( *next == '\n' ) {
	        next++;
		break;
	    }
	}
	
	sentence = (char *) G_malloc((next - start + 1) * sizeof (char));
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

/*!
  \brief Print error/warning/message

  \param msg message buffer
  \param type message type

  \return 0
*/
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

/*!
  \brief Print percentage information

  \param x value

  \return 0
*/
int print_percent(int x)
{
    char msg[256];

    if (logStream) {
	sprintf(msg, "GRASS_INFO_PERCENT: %d\n", x);
	PyFile_WriteString(msg, logStream);
    }
    else {
	fprintf(stderr, "GRASS_INFO_PERCENT: %d\n", x);
    }
    
    return 0;
}
