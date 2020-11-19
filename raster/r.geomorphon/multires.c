#include "local_proto.h"

int pattern_matching(int *pattern)
{
    int n, i;
    unsigned char binary = 0, result = 255, test = 0;
    unsigned char source = 32;
    int sign = -1;

    for (i = 0, n = 1; i < NUM_DIRS; i++, n *= 2)
        binary += (pattern[i] == sign) ? n : 0;
    /* rotate */
    for (i = 0; i < NUM_DIRS; ++i) {
        if ((i &= 7) == 0)
            test = binary;
        else
            test = (binary << i) | (binary >> (NUM_DIRS - i));
        result = MIN(result, test);
    }
    return ((result & source) == source) ? 1 : 0;

}
