
/**********************************************************************
 *
 *  Rast_fpreclass_init (r)
 *
 *       struct FPReclass *r;
 *
 *  initializes new reclassification structure. calls
 *  Rast_fpreclass_clear() before it returns.
 *  
 **********************************************************************
 *
 *  void
 *  Rast_fpreclass_reset (r)
 *  
 *       struct FPReclass *r;
 *
 *  resets the number of defined rules to 0 and free's space allocated
 *  for rules. calls Rast_fpreclass_clear ().
 *
 **********************************************************************
 *
 *  void
 *  Rast_fpreclass_clear (r)
 *  
 *       struct FPReclass *r;
 *
 *  resets the number of defined rules to 0. Resets default Min and Max
 *  to be unknown. (see Rast_fpreclass_set_domain (), Rast_fpreclass_set_range ()).
 *  deactivates default mapping.
 *
 **********************************************************************
 *
 *  void
 *  Rast_fpreclass_set_domain (r, dLow, dHigh)
 *
 *       struct FPReclass *r;
 *       DCELL dLow, dHigh;
 *
 *  defines the domain for the default mapping and 
 *  activates default mapping. (see G_fpreclass_perform_d ()).
 *  
 *  note: dHigh < dLow is valid.
 *
 **********************************************************************
 *
 *  void
 *  Rast_fpreclass_set_range (r, low, high)
 *
 *       struct FPReclass *r;
 *       DCELL low, high;
 *
 *  defines the range for the default mapping. does NOT
 *  activate default mapping. (see G_fpreclass_perform_d ()).
 *  
 **********************************************************************
 *
 *  int
 *  Rast_fpreclass_get_limits (r, dMin, dMax, rMin, rmax)
 *  
 *       const struct FPReclass *r;
 *       DCELL *dMin, *dMax;
 *       DCELL *rMin, *rmax;
 *
 *  returns the minimum and maximum values of all the rules defined.
 *  
 *  returns: -1 if after Rast_fpreclass_init (), or any call to 
 *                 Rast_fpreclass_clear () or Rast_fpreclass_reset () neither 
 *                 Rast_fpreclass_add_rule () nor Rast_fpreclass_set_domain () is
 *                 used. in this case the returned minimum and maximum 
 *                 range and domain values are undefined.
 *            0 if the default rule values are returned.domain values
 *                 are identical to those set with Rast_fpreclass_set_domain ().
 *                 range values are either reclassification internal default,
 *                 or the values set with Rast_fpreclass_set_range ().
 *            1 otherwise. in this case the values returned correspond
 *                 to the extreme values of the defined rules (they need 
 *                 not be identical to the values set with
 *                 Rast_fpreclass_set_domain ()).
 *
 **********************************************************************
 *  
 *  int
 *  Rast_fpreclass_nof_rules (r)
 *  
 *       const struct FPReclass *r;
 *  
 *  returns the number of reclassification rules defined. This number does
 *  not include the 2 infinite intervals.
 *  
 **********************************************************************
 *  
 *  void
 *  Rast_fpreclass_get_ith_rule (r, i, dLow, dHigh, rLow, rHigh)
 *  
 *       const struct FPReclass *r;
 *       int i;
 *       DCELL *dLow, *dHigh;
 *       DCELL *rLow, *rHigh;
 *  
 *  returns the i'th reclassification rule, for 
 *  0 <= i < Rast_fpreclass_nof_rules().
 *  a larger value for i means that the rule has been added later.
 *  
 **********************************************************************
 *   void
 *   Rast_fpreclass_set_neg_infinite_rule (r, dLeft, c)
 *
 *       struct FPReclass *r;
 *       DCELL dLeft;
 *       DCELL c;
 *
 *   defines a rule for values "dLeft" and smaller. values in this range
 *   are mapped to "c" if none of the "finite" reclassification rules applies.
 *
 * **********************************************************************
 *
 *  int
 *  Rast_fpreclass_get_neg_infinite_rule (r, dLeft, c)
 *
 *       const struct FPReclass *r;
 *       DCELL *dLeft;
 *       DCELL *c;
 *
 *  returns in "dLeft" and "c" the rule values for the negative infinite
 *  interval (see Rast_fpreclass_set_neg_infinite_rule ()).
 *
 *  returns: 0 if this rule is not defined
 *           1 otherwise.
 *
 **********************************************************************
 *   void
 *   Rast_fpreclass_set_pos_infinite_rule (r, dRight, c)
 *
 *       struct FPReclass *r;
 *       DCELL dRight;
 *       DCELL c;
 *
 *   defines a rule for values "dRight" and larger. values in this range
 *   are mapped to "c" if neither any of the "finite" reclassification
 *   rules nor the negative infinite rule applies.
 *
 * **********************************************************************
 *
 *  int
 *  Rast_fpreclass_get_pos_infinite_rule (r, dRight, c)
 *
 *       const struct FPReclass *r;
 *       DCELL *dRight;
 *       DCELL *c;
 *
 *  returns in "dRight" and "c" the rule values for the positive infinite
 *  interval (see Rast_fpreclass_set_pos_infinite_rule ()).
 *
 *  returns: 0 if this rule is not defined
 *           1 otherwise.
 *
 **********************************************************************
 *  
 *  void
 *  Rast_fpreclass_reverse_rule_order (r)
 *
 *        struct FPReclass *r;
 *
 *  reverses the order in which the reclassification rules are stored. (see
 *  also Rast_fpreclass_get_ith_rule () and G_fpreclass_perform_XY ()).
 *  
 **********************************************************************
 *  
 *  void
 *  Rast_fpreclass_add_rule (r, dLow, dHigh, rLow, rHigh)
 *  
 *       struct FPReclass *r;
 *       DCELL dLow, dHigh;
 *       DCELL rLow, rHigh;
 *  
 *  adds a new rule to the set of reclassification rules. if dLow > dHigh
 *  the rule will be stored with the low and high values interchanged.
 *  
 *  Note: currently no cleanup of rules is performed, i.e. redundant
 *        rules are not removed.
 *  
 **********************************************************************
 *  
 *  DCELL
 *  Rast_fpreclass_get_cell_value (r, cellValue)
 *  
 *       const struct FPReclass *r;
 *       DCELL *cellValue;
 *  
 *  returns the reclassified value corresponding to "cellValue".
 *
 *  if several reclassification rules apply for cellValue, the one which has 
 *  been inserted latest (i.e. the one of them which is returned by 
 *  Rast_fpreclass_get_ith_rule() for the largest i) is used. if no such rule
 *  applies the cellValue is first tested against the negative infinite
 *  rule, and finally against the positive infinite rule. if none of
 *  these rules apply, NO_DATA is returned. the actual value of NO_DATA 
 *  is found by calling Rast_set_d_null_value()
 *  
 *  if after Rast_fpreclass_init (), or any call to Rast_fpreclass_clear () or 
 *  Rast_fpreclass_reset () neither Rast_fpreclass_add_rule (),
 *  Rast_fpreclass_set_neg_infinite_rule (),  
 *  Rast_fpreclass_set_pos_infinite_rule (), *  nor  Rast_fpreclass_set_domain () 
 *  is used NO_DATA is returned independently of the cellValue.
 *
 *  if Rast_fpreclass_set_domain () is called but no explicit reclassification
 *  rule is set, the default mapping to the cell range set with 
 *  Rast_fpreclass_set_range () or, if the cell range is not set, 
 *  to the default CELL range [0,256 - 1] is applied.
 *  
 **********************************************************************
 *  
 *  void
 *  G_fpreclass_perform_XY (r, xcell, ycell, n)
 *  
 *       const struct FPReclass *r;
 *       XCELL *xcell;
 *       YCELL *ycell;
 *       int n;
 *  
 *  "X" and "Y" in the function name can be any of "d", "f", or "i". These
 *  correspond to "DCELL", "FCELL", and "CELL", respectively, and denote
 *  the type of the domain and range values.
 *  
 *  returns in "ycell" the reclassified YCELL values corresponding to the
 *  XCELL values stored in "xcell". the number of elements reclassified
 *  is n. reclassification is performed by repeated application of 
 *  Rast_fpreclass_get_cell_value ().
 *  
 **********************************************************************/

/*--------------------------------------------------------------------------*/

/*
   the reclassification table is stored as a linear array. rules are added 
   starting from index 0. redundant rules are not eliminated. rules are tested 
   from the highest index downto 0. there are two "infinite" rules. support is 
   provided to reverse the order of the rules.
 */

/*--------------------------------------------------------------------------*/

#include <grass/gis.h>
#include <grass/raster.h>

/*--------------------------------------------------------------------------*/

#undef MIN
#undef MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define NO_DEFAULT_RULE (! r->defaultDRuleSet)
#define NO_LEFT_INFINITE_RULE (! r->infiniteLeftSet)
#define NO_RIGHT_INFINITE_RULE (! r->infiniteRightSet)
#define NO_FINITE_RULE (r->nofRules <= 0)
#define NO_EXPLICIT_RULE (NO_FINITE_RULE && \
			  NO_LEFT_INFINITE_RULE && NO_RIGHT_INFINITE_RULE)

#define DEFAULT_MIN ((DCELL) 1)
#define DEFAULT_MAX ((DCELL) 255)

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_clear(struct FPReclass *r)
{
    r->nofRules = 0;
    r->defaultDRuleSet = 0;
    r->defaultRRuleSet = 0;
    r->infiniteRightSet = r->infiniteLeftSet = 0;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_reset(struct FPReclass *r)
{
    Rast_fpreclass_clear(r);

    if (r->maxNofRules > 0)
	G_free(r->table);

    r->maxNofRules = 0;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_init(struct FPReclass *r)
{
    r->maxNofRules = 0;
    Rast_fpreclass_reset(r);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_set_domain(struct FPReclass *r, DCELL dLow, DCELL dHigh)
{
    r->defaultDMin = dLow;
    r->defaultDMax = dHigh;
    r->defaultDRuleSet = 1;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_set_range(struct FPReclass *r, DCELL low, DCELL high)
{
    r->defaultRMin = low;
    r->defaultRMax = high;
    r->defaultRRuleSet = 1;
}

/*--------------------------------------------------------------------------*/

static void fpreclass_set_limits(struct FPReclass *r,
				 DCELL dLow, DCELL dHigh,
				 DCELL rLow, DCELL rHigh)
{
    r->dMin = dLow;
    r->dMax = dHigh;
    r->rMin = rLow;
    r->rMax = rHigh;
}

/*--------------------------------------------------------------------------*/

static void fpreclass_update_limits(struct FPReclass *r,
				    DCELL dLow, DCELL dHigh,
				    DCELL rLow, DCELL rHigh)
{
    if (NO_EXPLICIT_RULE) {
	fpreclass_set_limits(r, dLow, dHigh, rLow, rHigh);
	return;
    }

    r->dMin = MIN(r->dMin, MIN(dLow, dHigh));
    r->dMax = MAX(r->dMax, MAX(dLow, dHigh));
    r->rMin = MIN(r->rMin, MIN(rLow, rHigh));
    r->rMax = MAX(r->rMax, MAX(rLow, rHigh));
}

/*--------------------------------------------------------------------------*/

int Rast_fpreclass_get_limits(const struct FPReclass *r,
			      DCELL * dMin, DCELL * dMax,
			      DCELL * rMin, DCELL * rMax)
{
    if (NO_EXPLICIT_RULE) {
	if (NO_DEFAULT_RULE)
	    return -1;

	*dMin = r->defaultDMin;
	*dMax = r->defaultDMax;

	if (r->defaultRRuleSet) {
	    *rMin = r->defaultRMin;
	    *rMax = r->defaultRMax;
	}
	else {
	    *rMin = DEFAULT_MIN;
	    *rMax = DEFAULT_MAX;
	}

	return 0;
    }

    *dMin = r->dMin;
    *dMax = r->dMax;
    *rMin = r->rMin;
    *rMax = r->rMax;

    return 1;
}

/*--------------------------------------------------------------------------*/

int Rast_fpreclass_nof_rules(const struct FPReclass *r)
{
    return r->nofRules;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_get_ith_rule(const struct FPReclass *r, int i,
				 DCELL * dLow, DCELL * dHigh,
				 DCELL * rLow, DCELL * rHigh)
{
    *dLow = r->table[i].dLow;
    *dHigh = r->table[i].dHigh;
    *rLow = r->table[i].rLow;
    *rHigh = r->table[i].rHigh;
}

/*--------------------------------------------------------------------------*/

static void fpreclass_table_increase(struct FPReclass *r)
{
    if (r->nofRules < r->maxNofRules)
	return;

    if (r->maxNofRules == 0) {
	r->maxNofRules = 50;
	r->table = (struct FPReclass_table *)
	    G_malloc(r->maxNofRules * sizeof(struct FPReclass_table));
    }
    else {
	r->maxNofRules += 50;
	r->table = (struct FPReclass_table *)
	    G_realloc((char *)r->table,
		      r->maxNofRules * sizeof(struct FPReclass_table));
    }
}

/*--------------------------------------------------------------------------*/

void
Rast_fpreclass_set_neg_infinite_rule(struct FPReclass *r, DCELL dLeft,
				     DCELL c)
{
    r->infiniteDLeft = dLeft;
    r->infiniteRLeft = c;
    fpreclass_update_limits(r, dLeft, dLeft, c, c);
    r->infiniteLeftSet = 1;
}

/*--------------------------------------------------------------------------*/

int Rast_fpreclass_get_neg_infinite_rule(const struct FPReclass *r,
					 DCELL * dLeft, DCELL * c)
{
    if (r->infiniteLeftSet == 0)
	return 0;

    *dLeft = r->infiniteDLeft;
    *c = r->infiniteRLeft;

    return 1;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_set_pos_infinite_rule(struct FPReclass *r, DCELL dRight,
					  DCELL c)
{
    r->infiniteDRight = dRight;
    r->infiniteRRight = c;
    fpreclass_update_limits(r, dRight, dRight, c, c);
    r->infiniteRightSet = 1;
}

/*--------------------------------------------------------------------------*/

int Rast_fpreclass_get_pos_infinite_rule(const struct FPReclass *r,
					 DCELL * dRight, DCELL * c)
{
    if (r->infiniteRightSet == 0)
	return 0;

    *dRight = r->infiniteDRight;
    *c = r->infiniteRRight;

    return 1;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_add_rule(struct FPReclass *r,
			     DCELL dLow, DCELL dHigh, DCELL rLow, DCELL rHigh)
{
    int i;
    struct FPReclass_table *p;

    fpreclass_table_increase(r);

    i = r->nofRules;

    p = &(r->table[i]);
    if (dHigh >= dLow) {
	p->dLow = dLow;
	p->dHigh = dHigh;
	p->rLow = rLow;
	p->rHigh = rHigh;
    }
    else {
	p->dLow = dHigh;
	p->dHigh = dLow;
	p->rLow = rHigh;
	p->rHigh = rLow;
    }

    fpreclass_update_limits(r, dLow, dHigh, rLow, rHigh);

    r->nofRules++;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_reverse_rule_order(struct FPReclass *r)
{
    struct FPReclass_table tmp;
    struct FPReclass_table *pLeft, *pRight;

    pLeft = r->table;
    pRight = &(r->table[r->nofRules - 1]);

    while (pLeft < pRight) {
	tmp.dLow = pLeft->dLow;
	tmp.dHigh = pLeft->dHigh;
	tmp.rLow = pLeft->rLow;
	tmp.rHigh = pLeft->rHigh;

	pLeft->dLow = pRight->dLow;
	pLeft->dHigh = pRight->dHigh;
	pLeft->rLow = pRight->rLow;
	pLeft->rHigh = pRight->rHigh;

	pRight->dLow = tmp.dLow;
	pRight->dHigh = tmp.dHigh;
	pRight->rLow = tmp.rLow;
	pRight->rHigh = tmp.rHigh;

	pLeft++;
	pRight--;
    }
}

/*--------------------------------------------------------------------------*/

static DCELL fpreclass_interpolate(DCELL dLow, DCELL dHigh,
				   DCELL rLow, DCELL rHigh, DCELL dValue)
{
    if (rLow == rHigh)
	return rLow;
    if (dLow == dHigh)
	return rLow;

    return ((dValue - dLow) / (dHigh - dLow) * (rHigh - rLow) + rLow);
}

/*--------------------------------------------------------------------------*/

static DCELL fpreclass_get_default_cell_value(const struct FPReclass *r,
					      DCELL cellVal)
{
    DCELL tmp;

    Rast_set_d_null_value(&tmp, 1);

    if ((cellVal < MIN(r->defaultDMin, r->defaultDMax)) ||
	(cellVal > MAX(r->defaultDMin, r->defaultDMax)))
	return tmp;

    if (r->defaultRRuleSet)
	return fpreclass_interpolate(r->defaultDMin, r->defaultDMax,
				     r->defaultRMin, r->defaultRMax, cellVal);
    else
	return fpreclass_interpolate(r->defaultDMin, r->defaultDMax,
				     DEFAULT_MIN, DEFAULT_MAX, cellVal);
}

/*--------------------------------------------------------------------------*/

DCELL Rast_fpreclass_get_cell_value(const struct FPReclass * r, DCELL cellVal)
{
    DCELL tmp;
    const struct FPReclass_table *p;

    Rast_set_d_null_value(&tmp, 1);
    if (NO_EXPLICIT_RULE) {

	if (NO_DEFAULT_RULE)
	    return tmp;
	return fpreclass_get_default_cell_value(r, cellVal);
    }

    if (!NO_FINITE_RULE)
	for (p = &(r->table[r->nofRules - 1]); p >= r->table; p--)
	    if ((cellVal >= p->dLow) && (cellVal <= p->dHigh))
		return fpreclass_interpolate(p->dLow, p->dHigh, p->rLow,
					     p->rHigh, cellVal);

    if ((!NO_LEFT_INFINITE_RULE) && (cellVal <= r->infiniteDLeft))
	return r->infiniteRLeft;

    if ((NO_RIGHT_INFINITE_RULE) || (cellVal < r->infiniteDRight))
	return tmp;

    return r->infiniteRRight;
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_di(const struct FPReclass *r,
			       const DCELL * dcell, CELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, dcell++)
	if (!Rast_is_d_null_value(dcell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, *dcell);
	else
	    Rast_set_c_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_df(const struct FPReclass *r,
			       const DCELL * dcell, FCELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, dcell++)
	if (!Rast_is_d_null_value(dcell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, *dcell);
	else
	    Rast_set_f_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_dd(const struct FPReclass *r,
			       const DCELL * dcell, DCELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, dcell++)
	if (!Rast_is_d_null_value(dcell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, *dcell);
	else
	    Rast_set_d_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_fi(const struct FPReclass *r,
			       const FCELL * fcell, CELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, fcell++)
	if (!Rast_is_f_null_value(fcell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, (DCELL) * fcell);
	else
	    Rast_set_c_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_ff(const struct FPReclass *r,
			       const FCELL * fcell, FCELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, fcell++)
	if (!Rast_is_f_null_value(fcell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, (DCELL) * fcell);
	else
	    Rast_set_f_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_fd(const struct FPReclass *r,
			       const FCELL * fcell, DCELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, fcell++)
	if (!Rast_is_f_null_value(fcell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, (DCELL) * fcell);
	else
	    Rast_set_d_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_ii(const struct FPReclass *r,
			       const CELL * icell, CELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, icell++)
	if (!Rast_is_c_null_value(icell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, (DCELL) * icell);
	else
	    Rast_set_c_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_if(const struct FPReclass *r,
			       const CELL * icell, FCELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, icell++)
	if (!Rast_is_c_null_value(icell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, (DCELL) * icell);
	else
	    Rast_set_f_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

void Rast_fpreclass_perform_id(const struct FPReclass *r,
			       const CELL * icell, DCELL * cell, int n)
{
    int i;

    for (i = 0; i < n; i++, icell++)
	if (!Rast_is_c_null_value(icell))
	    *cell++ = Rast_fpreclass_get_cell_value(r, (DCELL) * icell);
	else
	    Rast_set_d_null_value(cell++, 1);
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
