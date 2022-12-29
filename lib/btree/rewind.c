#include <grass/btree.h>

int btree_rewind(BTREE * B)
{
    B->cur = 0;

    return 0;
}
