
/**
 * @file labels.h
 * This file defines the main datastructures used in this module.
 */
#ifndef _LABELS_H
#define _LABELS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/fontcap.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define LABEL_OVERLAP_WEIGHT 80.0

typedef struct _label label_t;
typedef struct _label_candidate label_candidate_t;
typedef struct _label_intersection label_intersection_t;
typedef struct _label_point label_point_t;
typedef struct _label_score label_score_t;

/**
 * A structure representing a point location */
struct _label_point
{

    double x;	  /**< The X coordinate */

    double y;	  /**< The Y coordinate */
};

/**
 * This structure represents a label for a vector feature */
struct _label
{

    struct line_pnts *skyline;	/**< The skyline of the text, as an offest
				  *  from the label point */
    struct bound_box bb;
    double size;

    double current_score;	  /**< The current score of the label. */

    label_candidate_t *candidates;  /**< A list of candidate positions */

    int n_candidates;		 /**< The size of the candidates array */

    int current_candidate;	 /**< An index into the candidates array
				   *  describing the currently selected candidate */

    char *text;			 /**< The label text */

    int cat;			     /**< the cat of the feature */

    int type;			     /**< The feture type (point, line, area) */

    struct line_pnts *shape;	     /**< The points for the feature that this
				       *  label belongs to */
};

/**
 * This structure represents a label candidate position.
 */
struct _label_candidate
{

    label_point_t point;     /**< The point of the label position 
			       *  (lower left corner)*/

    double score; /**< The base score of this position (sans overlap metric) */
    double lineover;

    double rotation;	 /**< The mount the label is rotated in this position */

    label_intersection_t *intersections;  /**< A list of all label candidate 
                                            *  positions which intersect with
                                            *  this position. */

    int n_intersections; /**< Number of items in the intersections array */
    struct line_pnts *baseline;
    struct line_pnts *swathline;
    int above;
};

/**
 * This structire represents an intersection of two labels
 */
struct _label_intersection
{

    label_t *label;	/**< A pointer to the #label_t structure */

    int candidate;     /**< The number of the candidate */
};

/**
 * These are the input options
 */
struct params
{
    struct Option *map;
    struct Option *type;
    struct Option *layer;
    struct Option *column;
    struct Option *labels;
    struct Option *font;
    struct Option *size;
    struct Option *charset;
    struct Option *isize;
    struct Option *color;
    struct Option *hlcolor;
    struct Option *hlwidth;
    struct Option *bgcolor;
    struct Option *opaque;
    struct Option *bocolor;
    struct Option *bowidth;

    /*    struct Option */
    /*      struct Option *where; *//* later */
};

/**
 * This function reads the labels from the input maps and returns an array of
 * label structures.
 */
label_t *labels_init(struct params *p, int *n_labels);

/**
 * This function searches for a maximum of 32 label candidate positions for
 * each feature.
 * @param labels The array of label structures.
 * @param n_labels The size of the array
 */
void label_candidates(label_t * labels, int n_labels);

/**
 * This function checks for all possible label overlap situations
 * @param labels The array of labels to check
 * @param n_labels The size of the labels array
 */
void label_candidate_overlap(label_t * labels, int n_labels);

/**
 * This function performs the actual annealing function. See documentation for
 * more information on simulated Annealing.
 * @param labels The labels to perform annealing on
 * @param n_labels The size of the labels array
 * @param p The program parametrs.
 */
void simulate_annealing(label_t * labels, int n_labels, struct params *p);

/**
 * This function writes the label information to the label file.
 * @param labelf An opened label file to append the label to.
 * @param label The label
 * @param p The parameters
 */
void print_label(FILE * labelf, label_t * label, struct params *p);

/**
 * This function rotates the label skyline and then translates it to the
 * given point.
 * @param skyline The skyline to translate
 * @param p The point to translate the skyline to
 * @param angle The angle (in radians) to rotate the label counter-clockwise
 * @return A lint_pnts structure containing the rotated and translated
 * skyline.
 */
struct line_pnts *skyline_trans_rot(struct line_pnts *skyline,
				    label_point_t * p, double angle);

struct GFONT_CAP *find_font_from_freetypecap(const char *font);
void free_freetypecap(struct GFONT_CAP *ftcap);

#endif /* _LABELS_H */
