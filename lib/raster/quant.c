/*!
 * \file lib/raster/quant.c
 * 
 * \brief Raster Library - Quantization rules.
 *
 * The quantization table is stored as a linear array. Rules are added
 * starting from index 0. Redundant rules are not eliminated. Rules
 * are tested from the highest index downto 0. There are two
 * "infinite" rules. Support is provided to reverse the order of the
 * rules.
 *
 * (C) 1999-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author USACERL and many others
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>

static int double_comp(const void *, const void *);

#define USE_LOOKUP 1
#define MAX_LOOKUP_TABLE_SIZE 2048
#define NO_DATA (Rast_set_c_null_value (&tmp, 1), (CELL) tmp)

#undef MIN
#undef MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define NO_LEFT_INFINITE_RULE (! q->infiniteLeftSet)
#define NO_RIGHT_INFINITE_RULE (! q->infiniteRightSet)
#define NO_FINITE_RULE (q->nofRules <= 0)
#define NO_EXPLICIT_RULE (NO_FINITE_RULE && \
			  NO_LEFT_INFINITE_RULE && NO_RIGHT_INFINITE_RULE)

/*!
   \brief Resets the number of defined rules and number of infinite rules to 0

   \param q pointer to Quant structure to be reset
 */
void Rast_quant_clear(struct Quant *q)
{
    q->nofRules = 0;
    q->infiniteRightSet = q->infiniteLeftSet = 0;
}

/*!
   \brief Resets and frees allocated memory

   Resets the number of defined rules to 0 and free's space allocated
   for rules. Calls Rast_quant_clear().

   \param q pointer to Quant structure to be reset
 */
void Rast_quant_free(struct Quant *q)
{
    Rast_quant_clear(q);

    if (q->maxNofRules > 0)
	G_free(q->table);
    if (q->fp_lookup.active) {
	G_free(q->fp_lookup.vals);
	G_free(q->fp_lookup.rules);
	q->fp_lookup.nalloc = 0;
	q->fp_lookup.active = 0;
    }
    q->maxNofRules = 0;
}

/*!
 * \brief Organized fp_lookup table.
 *
 *  Organizes fp_lookup table for faster (logarithmic) lookup time
 *  G_quant_organize_fp_lookup() creates a list of min and max for
 *  each quant rule, sorts this list, and stores the pointer to quant
 *  rule that should be used inbetween any 2 numbers in this list.
 *  Also it stores extreme points for 2 infinite rules, if exist.
 *  After the call to G_quant_organize_fp_lookup()
 *  instead of linearly searching through list of rules to find
 *  a rule to apply, quant lookup will perform a binary search
 *  to find an interval containing floating point value, and then use
 *  the rule associated with this interval.
 *  when the value doesn't fall within any interval, check for the
 *  infinite rules.
 *
 * \param q pointer to Quant structure which holds quant rules info
 *
 * \return 1 on success
 */
int Rast__quant_organize_fp_lookup(struct Quant *q)
{
    int i;
    DCELL val;
    CELL tmp;
    struct Quant_table *p;

    if (q->nofRules * 2 > MAX_LOOKUP_TABLE_SIZE)
	return -1;
    if (q->nofRules == 0)
	return -1;
    q->fp_lookup.vals = (DCELL *)
	G_calloc(q->nofRules * 2, sizeof(DCELL));
    /* 2 endpoints for each rule */
    q->fp_lookup.rules = (struct Quant_table **)
	G_calloc(q->nofRules * 2, sizeof(struct Quant_table *));

    /* first we organize finite rules into a table */
    if (!NO_FINITE_RULE) {
	i = 0;
	/* get the list of DCELL values from set of all dLows and dHighs
	   of all rules */
	/* NOTE: if dLow==DHigh in a rule, the value appears twice in a list 
	   but if dLow==DHigh of the previous, rule the value appears only once */

	for (p = &(q->table[q->nofRules - 1]); p >= q->table; p--) {
	    /* check if the min is the same as previous maximum */
	    if (i == 0 || p->dLow != q->fp_lookup.vals[i - 1])
		q->fp_lookup.vals[i++] = p->dLow;
	    q->fp_lookup.vals[i++] = p->dHigh;
	}
	q->fp_lookup.nalloc = i;

	/* now sort the values */
	qsort((char *)q->fp_lookup.vals, q->fp_lookup.nalloc,
	      sizeof(DCELL), double_comp);

	/* now find the rule to apply inbetween each 2 values in a list */
	for (i = 0; i < q->fp_lookup.nalloc - 1; i++) {
	    /*debug
	       fprintf (stderr, "%lf %lf ", q->fp_lookup.vals[i], q->fp_lookup.vals[i+1]);
	     */
	    val = (q->fp_lookup.vals[i] + q->fp_lookup.vals[i + 1]) / 2.;
	    q->fp_lookup.rules[i] =
		Rast__quant_get_rule_for_d_raster_val(q, val);
	    /* debug 
	       if(q->fp_lookup.rules[i])
	       fprintf (stderr, "%lf %lf %d %d\n", q->fp_lookup.rules[i]->dLow, q->fp_lookup.rules[i]->dHigh, q->fp_lookup.rules[i]->cLow, q->fp_lookup.rules[i]->cHigh); 
	       else fprintf (stderr, "null\n");
	     */

	}
    }				/* organizing finite rules */

    if (!NO_LEFT_INFINITE_RULE) {
	q->fp_lookup.inf_dmin = q->infiniteDLeft;
	q->fp_lookup.inf_min = q->infiniteCLeft;
    }
    else {
	if (q->fp_lookup.nalloc)
	    q->fp_lookup.inf_dmin = q->fp_lookup.vals[0];
	q->fp_lookup.inf_min = NO_DATA;
    }

    if (!NO_RIGHT_INFINITE_RULE) {
	if (q->fp_lookup.nalloc)
	    q->fp_lookup.inf_dmax = q->infiniteDRight;
	q->fp_lookup.inf_max = q->infiniteCRight;
    }
    else {
	q->fp_lookup.inf_dmax = q->fp_lookup.vals[q->fp_lookup.nalloc - 1];
	q->fp_lookup.inf_max = NO_DATA;
    }
    q->fp_lookup.active = 1;
    return 1;
}

/*!
 * \brief Initialize the structure
 *
 * Initializes the <i>q</i> struct.
 *
 * \param quant pointer to Quant structure to be initialized
 */
void Rast_quant_init(struct Quant *quant)
{
    quant->fp_lookup.active = 0;
    quant->maxNofRules = 0;
    quant->truncate_only = 0;
    quant->round_only = 0;
    Rast_quant_clear(quant);
}

/*!
   \brief Returns whether or not quant rules are set to truncate map

   \param quant pointer to Quant structure which holds quant rules info

   \return 1 if truncate is enable
   \return 0 if not truncated
 */
int Rast_quant_is_truncate(const struct Quant *quant)
{
    return quant->truncate_only;
}

/*!
   \brief  Returns whether or not quant rules are set to round map
   \param quant pointer to Quant structure which holds quant rules info

   \return 1 is round
   \return 0 not round
 */
int Rast_quant_is_round(const struct Quant *quant)
{
    return quant->round_only;
}

/*!
 * \brief Sets the quant rules to perform simple truncation on floats.
 *
 * Sets the quant for <i>q</i> rules to perform simple truncation on
 * floats.
 *
 * \param quant pointer to Quant structure which holds quant rules info
 */
void Rast_quant_truncate(struct Quant *quant)
{
    quant->truncate_only = 1;
}

/*!
 * \brief Sets the quant rules to perform simple rounding on floats.
 *
 * Sets the quant for <i>q</i> rules to perform simple rounding on
 * floats.
 *
 * \param quant pointer to Quant structure which holds quant rules info
 */
void Rast_quant_round(struct Quant *quant)
{
    quant->round_only = 1;
}

static void quant_set_limits(struct Quant *q,
			     DCELL dLow, DCELL dHigh, CELL cLow, CELL cHigh)
{
    q->dMin = dLow;
    q->dMax = dHigh;
    q->cMin = cLow;
    q->cMax = cHigh;
}

static void quant_update_limits(struct Quant *q,
				DCELL dLow, DCELL dHigh,
				CELL cLow, DCELL cHigh)
{
    if (NO_EXPLICIT_RULE) {
	quant_set_limits(q, dLow, dHigh, cLow, cHigh);
	return;
    }

    q->dMin = MIN(q->dMin, MIN(dLow, dHigh));
    q->dMax = MAX(q->dMax, MAX(dLow, dHigh));
    q->cMin = MIN(q->cMin, MIN(cLow, cHigh));
    q->cMax = MAX(q->cMax, MAX(cLow, cHigh));
}

/*!
 * \brief Returns the minimum and maximum cell and dcell values of all
 *  the ranges defined.
 *
 * Extracts the minimum and maximum floating-point and integer values
 * from all the rules (except the "infinite" rules) in <i>q</i> into
 * <i>dmin</i>, <i>dmax</i>, <i>cmin</i>, and <i>cmax</i>.
 *
 * \param quant pointer to Quant structure which holds quant rules info
 * \param[out] dmin minimum fp value
 * \param[out] dmax maximum fp value
 * \param[out] cmin minimum value
 * \param[out] cmax maximum value
 *
 * \return -1 if q->truncate or q->round are true or after
 * Rast_quant_init (), or any call to Rast_quant_clear () or Rast_quant_free()
 * no explicit rules have been added. In this case the returned
 * minimum and maximum CELL and DCELL values are null.
 * \return 1 if there are any explicit rules
 * \return 0 if there are no explicit rules (this includes cases when
 * q is set to truncate or round map), and sets <i>dmin</i>,
 * <i>dmax</i>, <i>cmin</i>, and <i>cmax</i> to NULL.
 */
int Rast_quant_get_limits(const struct Quant *q,
			  DCELL * dMin, DCELL * dMax, CELL * cMin,
			  CELL * cMax)
{
    if (NO_EXPLICIT_RULE) {
	Rast_set_c_null_value(cMin, 1);
	Rast_set_c_null_value(cMax, 1);
	Rast_set_d_null_value(dMin, 1);
	Rast_set_d_null_value(dMax, 1);
	return -1;
    }

    *dMin = q->dMin;
    *dMax = q->dMax;
    *cMin = q->cMin;
    *cMax = q->cMax;

    return 1;
}

/*!
   \brief Returns the number of quantization rules defined.

   This number does not include the 2 infinite intervals.

   \param q pointer to Quant structure which holds quant rules info

   \return number of quantization rules
 */
int Rast_quant_nof_rules(const struct Quant *q)
{
    return q->nofRules;
}

/*!
   \brief Returns the i'th quantization rule.

   For 0 <= i < Rast_quant_nof_rules(). A larger value for i means that
   the rule has been added later.

   \param q pointer to Quant structure which holds quant rules info
   \param i index
   \param[out] dLow minimum fp value
   \param[out] dHigh maximum fp value
   \param[out] cLow minimum value
   \param[out] cHigh maximum value
 */
void Rast_quant_get_ith_rule(const struct Quant *q,
			     int i,
			     DCELL * dLow, DCELL * dHigh,
			     CELL * cLow, CELL * cHigh)
{
    *dLow = q->table[i].dLow;
    *dHigh = q->table[i].dHigh;
    *cLow = q->table[i].cLow;
    *cHigh = q->table[i].cHigh;
}

static void quant_table_increase(struct Quant *q)
{
    if (q->nofRules < q->maxNofRules)
	return;

    if (q->maxNofRules == 0) {
	q->maxNofRules = 50;
	q->table = (struct Quant_table *)
	    G_malloc(q->maxNofRules * sizeof(struct Quant_table));
    }
    else {
	q->maxNofRules += 50;
	q->table = (struct Quant_table *)
	    G_realloc((char *)q->table,
		      q->maxNofRules * sizeof(struct Quant_table));
    }
}

/*!
   \brief Defines a rule for values "dLeft" and smaller.

   Values in this range are mapped to "c" if none of the "finite"
   quantization rules applies.

   \param q pointer to Quant structure which holds quant rules info

   \param dLeft fp value
   \param c value
 */
void Rast_quant_set_neg_infinite_rule(struct Quant *q, DCELL dLeft, CELL c)
{
    q->infiniteDLeft = dLeft;
    q->infiniteCLeft = c;
    quant_update_limits(q, dLeft, dLeft, c, c);

    /* update lookup table */
    if (q->fp_lookup.active) {
	q->fp_lookup.inf_dmin = q->infiniteDLeft;
	q->fp_lookup.inf_min = q->infiniteCLeft;
    }
    q->infiniteLeftSet = 1;
}

/*!
   \brief Returns in "dLeft" and "c" the rule values.

   For the negative infinite interval (see Rast_quant_set_neg_infinite_rule()).

   \param q pointer to Quant structure which holds quant rules info
   \param[out] dLeft fp value
   \param[out] c value

   \return 0 if this rule is not defined
   \return 1 otherwise
 */
int Rast_quant_get_neg_infinite_rule(const struct Quant *q,
				     DCELL * dLeft, CELL * c)
{
    if (q->infiniteLeftSet == 0)
	return 0;

    *dLeft = q->infiniteDLeft;
    *c = q->infiniteCLeft;

    return 1;
}

/*!
   \brief Defines a rule for values "dRight" and larger.

   Values in this range are mapped to "c" if none of the "finite"
   quantization rules or the negative infinite rule applies.

   \param q pointer to Quant structure which holds quant rules info
   \param dRight fp value
   \param c value
 */
void Rast_quant_set_pos_infinite_rule(struct Quant *q, DCELL dRight, CELL c)
{
    q->infiniteDRight = dRight;
    q->infiniteCRight = c;
    quant_update_limits(q, dRight, dRight, c, c);

    /* update lookup table */
    if (q->fp_lookup.active) {
	q->fp_lookup.inf_dmax = q->infiniteDRight;
	q->fp_lookup.inf_max = q->infiniteCRight;
    }
    q->infiniteRightSet = 1;
}

/*!
   \brief Returns in "dRight" and "c" the rule values.

   For the positive infinite interval (see Rast_quant_set_pos_infinite_rule()).

   \param q pointer to Quant structure which holds quant rules info
   \param[out] dRight fp value
   \param[out] c value

   \return 0 if this rule is not defined
   \return 1 otherwise
 */
int Rast_quant_get_pos_infinite_rule(const struct Quant *q,
				     DCELL * dRight, CELL * c)
{
    if (q->infiniteRightSet == 0)
	return 0;

    *dRight = q->infiniteDRight;
    *c = q->infiniteCRight;

    return 1;
}

/*!
   \brief Adds a new rule to the set of quantization rules.

   If dLow < dHigh the rule will be stored with the low and high values
   interchanged.

   Note: currently no cleanup of rules is performed, i.e. redundant
   rules are not removed. This can't be changed because Categories
   structure HEAVILY depends of quant rules stored in exactly the same
   order they are entered. So if the cleanup or rearrangement is done in
   the future make a flag for add_rule whether or not to do it, then
   quant will not set this flag.

   \param q pointer to Quant structure which holds quant rules info
   \param dLow minimum fp value
   \param dHigh maximum fp value
   \param cLow minimum value
   \param cHigh maximum value
 */
void Rast_quant_add_rule(struct Quant *q,
			 DCELL dLow, DCELL dHigh, CELL cLow, CELL cHigh)
{
    int i;
    struct Quant_table *p;

    quant_table_increase(q);

    i = q->nofRules;

    p = &(q->table[i]);
    if (dHigh >= dLow) {
	p->dLow = dLow;
	p->dHigh = dHigh;
	p->cLow = cLow;
	p->cHigh = cHigh;
    }
    else {
	p->dLow = dHigh;
	p->dHigh = dLow;
	p->cLow = cHigh;
	p->cHigh = cLow;
    }

    /* destroy lookup table, it has to be rebuilt */
    if (q->fp_lookup.active) {
	G_free(q->fp_lookup.vals);
	G_free(q->fp_lookup.rules);
	q->fp_lookup.active = 0;
	q->fp_lookup.nalloc = 0;
    }

    quant_update_limits(q, dLow, dHigh, cLow, cHigh);

    q->nofRules++;
}

/*!
   \brief Rreverses the order in which the qunatization rules are stored.

   See also Rast_quant_get_ith_rule() and Rast_quant_perform_d()).

   \param q pointer to Quant rules which holds quant rules info
 */
void Rast_quant_reverse_rule_order(struct Quant *q)
{
    struct Quant_table tmp;
    struct Quant_table *pLeft, *pRight;

    pLeft = q->table;
    pRight = &(q->table[q->nofRules - 1]);

    while (pLeft < pRight) {
	tmp.dLow = pLeft->dLow;
	tmp.dHigh = pLeft->dHigh;
	tmp.cLow = pLeft->cLow;
	tmp.cHigh = pLeft->cHigh;

	pLeft->dLow = pRight->dLow;
	pLeft->dHigh = pRight->dHigh;
	pLeft->cLow = pRight->cLow;
	pLeft->cHigh = pRight->cHigh;

	pRight->dLow = tmp.dLow;
	pRight->dHigh = tmp.dHigh;
	pRight->cLow = tmp.cLow;
	pRight->cHigh = tmp.cHigh;

	pLeft++;
	pRight--;
    }
}

static CELL quant_interpolate(DCELL dLow, DCELL dHigh,
			      CELL cLow, CELL cHigh, DCELL dValue)
{
    if (cLow == cHigh)
	return cLow;
    if (dLow == dHigh)
	return cLow;

    return (CELL) ((dValue - dLow) / (dHigh - dLow) * (DCELL) (cHigh - cLow) +
		   (DCELL) cLow);
}

static int less_or_equal(double x, double y)
{
    if (x <= y)
	return 1;
    else
	return 0;
}

static int less(double x, double y)
{
    if (x < y)
	return 1;
    else
	return 0;
}

/*!
 * \brief 
 *
 * 
 * Returns a CELL category for the floating-point <i>value</i> based
 * on the quantization rules in <i>q</i>. The first rule found that
 * applies is used. The rules are searched in the reverse order they
 * are added to <i>q</i>. If no rule is found, the <i>value</i>
 * is first tested against the negative infinite rule, and finally
 * against the positive infinite rule. If none of these rules apply,
 * the NULL-value is returned.
 *
 * <b>Note:</b> See G_quant_organize_fp_lookup() for details on how
 * the values are looked up from fp_lookup table when it is
 * active. Right now fp_lookup is automatically organized during the
 * first call to Rast_quant_get_cell_value().
 *
 * \param q pointer to Quant structure which holds quant rules info
 * \param dcellValue fp cell value
 *
 * \return cell value (integer)
 */
CELL Rast_quant_get_cell_value(struct Quant * q, DCELL dcellVal)
{
    CELL tmp;
    DCELL dtmp;
    int try, min_ind, max_ind;
    struct Quant_table *p;
    int (*lower) ();

    dtmp = dcellVal;
    /* I know the functions which call me already check for null values,
       but I am a public function, and can be called from outside */
    if (Rast_is_d_null_value(&dtmp))
	return NO_DATA;

    if (q->truncate_only)
	return (CELL) dtmp;

    if (q->round_only) {
	if (dcellVal > 0)
	    return (CELL) (dcellVal + .5);
	return (CELL) (dcellVal - .5);
    }

    if (NO_EXPLICIT_RULE)
	return NO_DATA;
    if (NO_EXPLICIT_RULE)
	return NO_DATA;

    if (USE_LOOKUP &&
	(q->fp_lookup.active || Rast__quant_organize_fp_lookup(q) > 0)) {
	/* first check if values fall within range */
	/* if value is below the range */
	if (dcellVal < q->fp_lookup.vals[0]) {
	    if (dcellVal <= q->fp_lookup.inf_dmin)
		return q->fp_lookup.inf_min;
	    else
		return NO_DATA;
	}
	/* if value is below above range */
	if (dcellVal > q->fp_lookup.vals[q->fp_lookup.nalloc - 1]) {
	    if (dcellVal >= q->fp_lookup.inf_dmax)
		return q->fp_lookup.inf_max;
	    else
		return NO_DATA;
	}
	/* make binary search to find which interval our value belongs to
	   and apply the rule for this interval */
	try = (q->fp_lookup.nalloc - 1) / 2;
	min_ind = 0;
	max_ind = q->fp_lookup.nalloc - 2;
	while (1) {
	    /* DEBUG 
	       fprintf (stderr, "%d %d %d\n", min_ind, max_ind, try); 
	     */
	    /* when the ruke for the interval is NULL, we exclude the end points.
	       when it exists, we include the end-points */
	    if (q->fp_lookup.rules[try])
		lower = less;
	    else
		lower = less_or_equal;

	    if (lower(q->fp_lookup.vals[try + 1], dcellVal)) {	/* recurse to the second half */
		min_ind = try + 1;
		/* must be still < nalloc-1, since number is within the range */
		try = (max_ind + min_ind) / 2;
		continue;
	    }
	    if (lower(dcellVal, q->fp_lookup.vals[try])) {	/* recurse to the second half */
		max_ind = try - 1;
		/* must be still >= 0, since number is within the range */
		try = (max_ind + min_ind) / 2;
		continue;
	    }
	    /* the value fits into the interval! */
	    p = q->fp_lookup.rules[try];
	    if (p)
		return quant_interpolate(p->dLow, p->dHigh, p->cLow, p->cHigh,
					 dcellVal);
	    /* otherwise when finite rule for this interval doesn't exist */
	    else {		/* first check if maybe infinite rule applies */
		if (dcellVal <= q->fp_lookup.inf_dmin)
		    return q->fp_lookup.inf_min;
		if (dcellVal >= q->fp_lookup.inf_dmax)
		    return q->fp_lookup.inf_max;
		else
		    return NO_DATA;
	    }
	}			/* while */
    }				/* looking up in fp_lookup */

    if (!NO_FINITE_RULE) {
	p = Rast__quant_get_rule_for_d_raster_val(q, dcellVal);
	if (!p)
	    return NO_DATA;
	return quant_interpolate(p->dLow, p->dHigh, p->cLow, p->cHigh,
				 dcellVal);
    }

    if ((!NO_LEFT_INFINITE_RULE) && (dcellVal <= q->infiniteDLeft))
	return q->infiniteCLeft;

    if ((NO_RIGHT_INFINITE_RULE) || (dcellVal < q->infiniteDRight))
	return NO_DATA;

    return q->infiniteCRight;
}

/*!
   \brief Returns in "cell" the quantized CELL values.

   Returns in "cell" the quantized CELL values corresponding to the
   DCELL values stored in "dcell". the number of elements quantized
   is n. quantization is performed by repeated application of 
   Rast_quant_get_cell_value().

   \param q pointer to Quant structure which holds quant rules info
   \param dcell pointer to fp cell values array
   \param[out] cell pointer cell values array
   \param n number of cells
 */
void Rast_quant_perform_d(struct Quant *q,
			  const DCELL * dcell, CELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, dcell++)
	if (!Rast_is_d_null_value(dcell))
	    *cell++ = Rast_quant_get_cell_value(q, *dcell);
	else
	    Rast_set_c_null_value(cell++, 1);
}

/*!
   \brief Same as Rast_quant_perform_d(), except the type.

   \param q pointer to Quant structure which holds quant rules info
   \param fcell pointer to fp cell values array
   \param[out] cell pointer cell values array
   \param n number of cells
 */
void Rast_quant_perform_f(struct Quant *q,
			  const FCELL * fcell, CELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, fcell++)
	if (!Rast_is_f_null_value(fcell))
	    *cell++ = Rast_quant_get_cell_value(q, (DCELL) * fcell);
	else
	    Rast_set_c_null_value(cell++, 1);
}

static int double_comp(const void *xx, const void *yy)
{
    const DCELL *x = xx;
    const DCELL *y = yy;

    if (Rast_is_d_null_value(x))
	return 0;
    if (*x < *y)
	return -1;
    else if (*x == *y)
	return 0;
    else
	return 1;
}

/*!
   \brief Returns quant rule which will be applied.

   Returns quant rule which will be applied when looking up the integer
   quant value for val (used when organizing fp_lookup).

   \param q pointer to Quant structure which holds quant rules info
   \param val fp cell value

   \return pointer to the Quant_table (color rule)
   \return NULL otherwise
 */
struct Quant_table *Rast__quant_get_rule_for_d_raster_val(const struct Quant
							  *q, DCELL val)
{
    const struct Quant_table *p;

    for (p = &(q->table[q->nofRules - 1]); p >= q->table; p--)
	if ((val >= p->dLow) && (val <= p->dHigh))
	    break;
    if (p >= q->table)
	return (struct Quant_table *)p;
    else
	return (struct Quant_table *)NULL;
}
