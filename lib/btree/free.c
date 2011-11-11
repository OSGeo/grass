#include <stdlib.h>

#include <grass/btree.h>

int btree_free(BTREE * B)
{
    void *data, *key;

    btree_rewind(B);
    while (btree_next(B, &key, &data)) {
	free(key);
	free(data);
    }
    free(B->node);

    return 0;
}
