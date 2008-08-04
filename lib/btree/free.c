#include <grass/btree.h>
#include <stdlib.h>

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
