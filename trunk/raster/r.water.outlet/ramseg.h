#define RAMSEG		int
#define RAMSEGBITS 	4
#define DOUBLEBITS 	8	/* 2 * ramsegbits       */
#define SEGLENLESS 	15	/* 2 ^ ramsegbits - 1   */

#define SEG_INDEX(s,r,c) (int) \
   (((((r) >> RAMSEGBITS) * (s) + ((c) >> RAMSEGBITS)) << DOUBLEBITS) \
    + (((r) & SEGLENLESS) << RAMSEGBITS) + ((c) & SEGLENLESS))
