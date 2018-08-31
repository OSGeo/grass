/* ****************************************************************************
 * FILE:         annealing.c
 * MODULE:       v.labels.sa
 * AUTHOR(S):    Wolf Bergenheim
 * PURPOSE:      This file contains functions which have to do with the
 annealing part of the algorithm.
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include "labels.h"
#ifndef M_E

/**
 * This is simply Euler's number
 */
# define M_E 2.7182818284590452353602874713526625L
#endif

/**
 * How many times to decrease the Temperature T.
 */
#define TEMP_DECS 50

static double calc_label_overlap(label_t * label, int cc, int nc);
static void do_label_overlap(label_t * label, int cc, int nc);

/**
 * This is just a variable used to show statistics in the end.
 */
static unsigned int overlaps_created = 0;

/**
 * This is just a variable used to show statistics in the end.
 */
static unsigned int overlaps_removed = 0;

/**
 * This funxtion does the actual sumulated annealing process. Each round 30 x
 * n (the number of labels) a label is picked at random, and placed in a random
 * new position. Then the dE is calculated, and if dE is > 0 the new position is
 * reversed with the probablility 1 - e^(-dE / T).
 @param labels The array of all labels.
 @param n_labels The size of the labels array.
 @params The commandline parameters.
 */
void simulate_annealing(label_t * labels, int n_labels, struct params *p)
{
    /* The temperature of the system */
    double T;

    /* The change in energy */
    double dE;

    T = -1.0 / log(1.0 / 3.0);
    unsigned int t, tot_better = 0, tot_worse = 0, tot_ign = 0;

    fprintf(stderr, "Optimizing label positions: ...");
    for (t = 0; t < TEMP_DECS; t++) {
	int i;
	unsigned int successes = 0, consec_successes = 0;

	for (i = 0; i < (n_labels * 30); i++) {
	    int l, c, cc, r;
	    label_t *lp;

	    /* pick a random label */
	    r = rand();
	    l = (int)((double)(n_labels) * (r / (RAND_MAX + 1.0)));
	    lp = &labels[l];
	    /* skip labels without sufficient number of candidates */
	    if (lp->n_candidates < 2)
		continue;

	    cc = lp->current_candidate;
	    /*and a random new candidate place */
	    c = (int)((double)(lp->n_candidates) *
		      (rand() / (RAND_MAX + 1.0)));
	    if (c == cc) {
		if (c == 0)
		    c++;
		else
		    c--;
	    }
	    /* calc dE */
	    dE = lp->candidates[c].score - lp->candidates[cc].score;
	    dE += calc_label_overlap(lp, cc, c);

	    /* if dE < 0 accept */
	    if (dE < 0.0) {
		lp->current_score = lp->candidates[c].score;
		do_label_overlap(lp, cc, c);
		lp->current_candidate = c;
		successes++;
		consec_successes++;
		tot_better++;
	    }
	    /* else apply with probability p=e^(-dE/T) */
	    else {
		double dp, dr;

		dp = pow(M_E, -dE / T);
		dr = (double)rand() / RAND_MAX;
		if (dr <= dp) {
		    do_label_overlap(lp, cc, c);
		    lp->current_score += lp->candidates[c].score;
		    lp->current_candidate = c;
		    successes++;
		    consec_successes++;
		    tot_worse++;
		}
		else {
		    tot_ign++;
		    consec_successes = 0;
		}
	    }
	    /* decrease immediately */
	    if (consec_successes > (5 * n_labels)) {
		consec_successes = 0;
		break;
	    }
	}
	G_percent(t, TEMP_DECS, 1);
	/* we have found an optimal solution */
	if (successes == 0) {
	    break;
	}
	T -= 0.1 * T;
    }
    G_percent(TEMP_DECS, TEMP_DECS, 1);
}

/**
 * This function calculates the change in E (dE) if the given label would
 * be moved to the new place. Contrary to the original algorithm this function
 * requires twice as much energy to overlap two labels then is released by
 * resolving an overlap. I don't have and scientific fact but it seems to
 * improve the result.
 * @param label The label in question
 * @param cc The current candidate
 * @param nc The new potential candidate location
 * @return The dE value.
 */
static double calc_label_overlap(label_t * label, int cc, int nc)
{
    int i;
    double dE = 0.0;

    /* calculate the overlaps removed */
    for (i = 0; i < label->candidates[cc].n_intersections; i++) {
	label_t *ol;
	int oc;

	ol = label->candidates[cc].intersections[i].label;
	oc = label->candidates[cc].intersections[i].candidate;
	if (ol->current_candidate == oc) {
	    dE -= LABEL_OVERLAP_WEIGHT;
	}
    }

    /* calculate the overlaps created */
    for (i = 0; i < label->candidates[nc].n_intersections; i++) {
	label_t *ol;
	int oc;

	ol = label->candidates[nc].intersections[i].label;
	oc = label->candidates[nc].intersections[i].candidate;
	if (ol->current_candidate == oc) {
	    dE += LABEL_OVERLAP_WEIGHT;
	}
    }

    return dE;
}

/**
 * This function commits the label change to the new location.
 * @param label The label to move
 * @param cc The current candidate
 * @param nc The new potential candidate location
 */
static void do_label_overlap(label_t * label, int cc, int nc)
{
    int i;

    /* remove the current label overlaps */
    for (i = 0; i < label->candidates[cc].n_intersections; i++) {
	label_t *ol;
	int oc;

	ol = label->candidates[cc].intersections[i].label;
	oc = label->candidates[cc].intersections[i].candidate;
	if (ol->current_candidate == oc) {
	    ol->current_score -= LABEL_OVERLAP_WEIGHT;
	    label->current_score -= LABEL_OVERLAP_WEIGHT;
	    /* ol->candidates[oc].score -= LABEL_OVERLAP_WEIGHT; */
	    overlaps_removed++;
	}
    }

    /* create new overlaps */
    for (i = 0; i < label->candidates[nc].n_intersections; i++) {
	label_t *ol;
	int oc;

	ol = label->candidates[nc].intersections[i].label;
	oc = label->candidates[nc].intersections[i].candidate;
	if (ol->current_candidate == oc) {
	    ol->current_score += LABEL_OVERLAP_WEIGHT;
	    label->current_score += LABEL_OVERLAP_WEIGHT;
	    /* ol->candidates[oc]->score += 40; */
	    overlaps_created++;
	}
    }
}
