#ifndef __RAMSEG_H__
#define __RAMSEG_H__

#define RAMSEG     int
#define RAMSEGBITS 4
#define DOUBLEBITS 8  /* 2 * ramsegbits       */
#define SEGLENLESS 15 /* 2 ^ ramsegbits - 1   */

<<<<<<< HEAD
#define SEG_INDEX(s, r, c)                                                 \
    (size_t)(                                                              \
        ((((size_t)(r) >> RAMSEGBITS) * (s) + ((size_t)(c) >> RAMSEGBITS)) \
         << DOUBLEBITS) +                                                  \
        (((size_t)(r)&SEGLENLESS) << RAMSEGBITS) + ((size_t)(c)&SEGLENLESS))
=======
#define SEG_INDEX(s, r, c)                                                    \
    (int)(((((r) >> RAMSEGBITS) * (s) + ((c) >> RAMSEGBITS)) << DOUBLEBITS) + \
          (((r)&SEGLENLESS) << RAMSEGBITS) + ((c)&SEGLENLESS))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

#endif /* __RAMSEG_H__ */
