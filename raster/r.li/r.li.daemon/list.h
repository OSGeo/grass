
/**
 * \file list.h
 *
 * \brief Implementation of a FIFO list of messages
 *
 * \author Porta Claudio
 *
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 * \version 1.0
 *
 * \include
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "ipc.h"

/**
 * \brief node of FIFO list
 * \member prev the previous item in list
 * \member next the next item in list
 * \member m the content of list node
 */
struct nodoLista
{
    struct nodoLista *prev;
    struct nodoLista *next;
    msg *m;
};



/**
 * \brief FIFO list
 * \member head first item in list
 * \member tail last item in list
 * \member size number of items in list
 */
struct lista
{
    struct nodoLista *head;
    struct nodoLista *tail;
    int size;
};

typedef struct nodoLista *node;

typedef struct lista *list;

/**
 * \brief insert a item in list
 * \param l list where to put items
 * \param mess the message to insert
 */
void insertNode(list l, msg m);

/**
 * \brief remove head item
 * \param l list where to remove
 */
void removeNode(list l);

 /**
  * \brief struct for runtime area generation
  * \param dist inter-area distance
  * \param add_row cell to add in rows
  * \param add_col cell to add in columns
  * \param x x coordinate of next area
  * \param y y coordinate of next area
  * \param rl area length in rows
  * \param cl areal length in columns
  * \param count identifier of next area
  * \param sf_x x coordinate of sample frame
  * \param sf_y y coordinate of sample frame
  * \param maskname name of mask for the area
  */
struct generatore
{
    int dist;
    int add_row;
    int add_col;
    int rows;
    int cols;
    int x;
    int y;
    int rl;
    int cl;
    int count;
    int sf_x;
    int sf_y;
    char *maskname;
};

typedef struct generatore *g_areas;

/**
 * \brief runtime area generation
 * \param gen area generator to use
 * \param msg next area message
 */
int next(g_areas gen, msg * toReturn);
