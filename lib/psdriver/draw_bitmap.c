#include "psdriver.h"

void PS_Bitmap(int ncols, int nrows, int threshold, const unsigned char *buf)
{
    int i, j;

    output("%d %d %d %d BITMAP\n", cur_x, cur_y, ncols, nrows);

    for (j = 0; j < nrows; j++) {
        unsigned int bit = 0x80;
        unsigned int acc = 0;

        for (i = 0; i < ncols; i++) {
            unsigned int k = buf[j * ncols + i];

<<<<<<< HEAD
<<<<<<< HEAD
            if (k > (unsigned int)threshold)
=======
            if (k > threshold)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
            if (k > threshold)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
                acc |= bit;

            bit >>= 1;

            if (!bit) {
                output("%02X", acc);
                bit = 0x80;
                acc = 0;
            }
        }

        if (bit != 0x80)
            output("%02X", acc);

        output("\n");
    }
}
