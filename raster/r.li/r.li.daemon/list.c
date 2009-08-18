
/**
 * \file list.c
 *
 * \brief Implementation of a FIFO list of messages
 *
 * \author Porta Claudio
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 * \version 1.0
 *
 * 
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "daemon.h"


/**
 * \brief insert a node in list
 * \param l list where to insert
 * \param mess the message to insert
 */
void insertNode(list l, msg mess)
{
    node new;

    new = G_malloc(sizeof(node));
    new->m = G_malloc(sizeof(msg));

    if (new != NULL) {
	memcpy(new->m, &mess, sizeof(msg));
	new->next = new->prev = NULL;

	if (l->head == NULL) {
	    l->head = l->tail = new;
	}
	else {
	    l->tail->next = new;
	    new->prev = l->tail;
	    l->tail = new;
	}
    }
    else
	G_message(_("Out of memory"));

    l->size++;
}


/**
 *\brief remove a node from list
 * \param list where to remove
 */
void removeNode(list l)
{
    if (l->head == NULL) {
	return;
    }
    if (l->head->next == NULL) {
	node tmp = l->head;

	l->head = NULL;
	free(tmp);
	l->size--;
    }
    else {
	node tmp = l->head;

	l->head = l->head->next;
	l->head->prev = NULL;
	free(tmp);
	l->size--;
    }
}

/**
 * \brief runtime area generation
 * \param gen area generator to use
 * \param msg next area message
 */
int next(g_areas gen, msg * toReturn)
{

    if (gen->cl > gen->cols)
	return 0;
    if (gen->rl > gen->rows)
	return 0;

    if (gen->maskname == NULL) {
	/* area */
	(*toReturn).type = AREA;
	if (gen->cols - gen->x + gen->sf_x < gen->add_col) {
	    gen->x = gen->sf_x + gen->dist;
	    gen->y = gen->y + gen->add_row;
	}
	if (gen->rows - gen->y + gen->sf_y >= gen->add_row) {
	    (*toReturn).f.f_a.aid = gen->count;
	    (gen->count)++;
	    (*toReturn).f.f_a.x = gen->x;
	    gen->x = gen->x + gen->add_col;
	    (*toReturn).f.f_a.y = gen->y;
	    (*toReturn).f.f_a.rl = gen->rl;
	    (*toReturn).f.f_a.cl = gen->cl;
	    return 1;
	}
	else
	    return 0;
    }
    else {
	/* maskedarea */
	(*toReturn).type = MASKEDAREA;
	if (gen->cols - gen->x + gen->sf_x < gen->add_col) {
	    gen->x = gen->sf_x + gen->dist;
	    gen->y = gen->y + gen->add_row;
	}
	if (gen->rows - gen->y + gen->sf_y > gen->add_row) {
	    (*toReturn).f.f_ma.aid = gen->count;
	    (gen->count)++;
	    (*toReturn).f.f_ma.x = gen->x;
	    gen->x = gen->x + gen->add_col;
	    (*toReturn).f.f_ma.y = gen->y;
	    (*toReturn).f.f_ma.rl = gen->rl;
	    (*toReturn).f.f_ma.cl = gen->cl;
	    strcpy((*toReturn).f.f_ma.mask, gen->maskname);
	    return 1;
	}
	else
	    return 0;
    }
}
