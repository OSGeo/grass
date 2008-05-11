#ifndef _RGBPACK_H
#define _RGBPACK_H

#define RED_MASK 0x000000FF
#define GRN_MASK 0x0000FF00
#define BLU_MASK 0x00FF0000

#define INT_TO_RED(i, r)    (r = (i & RED_MASK))
#define INT_TO_GRN(i, g)    (g = (i & GRN_MASK) >> 8)
#define INT_TO_BLU(i, b)    (b = (i & BLU_MASK) >> 16)

#define RGB_TO_INT(r,g,b,i) (i = (((r) & RED_MASK) +                \
			    ((int)((g) << 8) & GRN_MASK) +     \
			    ((int)((b) << 16) & BLU_MASK)))

#define CONST_COLS 45
#define COLUMNS     9

#endif /* _RGBPACK_H */
