
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
void insertNode(struct list *l, msg mess)
{
    struct node *new;

    new = G_malloc(sizeof(struct node));
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
void removeNode(struct list *l)
{
    if (l->head == NULL) {
	return;
    }
    if (l->head->next == NULL) {
	struct node *tmp = l->head;

	l->head = NULL;
	G_free(tmp->m);
	G_free(tmp);
	l->size--;
    }
    else {
	struct node *tmp = l->head;

	l->head = l->head->next;
	l->head->prev = NULL;
	G_free(tmp->m);
	G_free(tmp);
	l->size--;
    }
}

/**
 * \brief runtime area generation
 * \param gen area generator to use
 * \param msg next area message
 */
int next(struct g_area *gen, msg *toReturn)
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
