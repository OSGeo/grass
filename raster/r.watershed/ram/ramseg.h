#ifndef __RAMSEG_H__
#define __RAMSEG_H__

#define RAMSEG     int
#define RAMSEGBITS 4
#define DOUBLEBITS 8  /* 2 * ramsegbits       */
#define SEGLENLESS 15 /* 2 ^ ramsegbits - 1   */

#define SEG_INDEX(s, r, c)                                \
    (size_t)(((((size_t)(r) >> RAMSEGBITS) * (s) +        \
               ((size_t)(c) >> RAMSEGBITS))               \
              << DOUBLEBITS) +                            \
             (((size_t)(r) & SEGLENLESS) << RAMSEGBITS) + \
             ((size_t)(c) & SEGLENLESS))

#endif /* __RAMSEG_H__ */
