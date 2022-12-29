
/****************************************************************************
 *
 * MODULE:       btree
 * AUTHOR(S):    CERL (present in ver 4.x)
 *               Radim Blazek <radim.blazek gmail.com>
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      balanced tree - possibly duplicating libavl functionality; see
 *               http://grass.itc.it/pipermail/grass-dev/2007-April/030396.html
 * COPYRIGHT:    (C) 2002-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <grass/btree.h>

static int cmp(const void *a, const void *b)
{
    return strcmp(a, b);
}

int main(void)
{
    char key[100], data[100];
    void *k, *d;
    BTREE B;

    btree_create(&B, strcmp, 10);
    while (1) {
	fprintf(stdout, "enter key (or RETURN if done): ");
	if (!gets(key))
	    exit(0);
	if (*key == 0)
	    break;
	fprintf(stdout, "    ");
	if (btree_find(&B, key, &d))
	    fprintf(stdout, "%s = %s\n", key, d);
	else
	    fprintf(stdout, "%s - not found\n", key);
	fprintf(stdout, "    ");
	fprintf(stdout, "enter new value (or RETURN if none): ");
	if (!gets(data))
	    exit(0);
	if (*data)
	    btree_update(&B, key, strlen(key) + 1, data, strlen(data) + 1);
    }

    fprintf(stdout, "final tree\n");
    btree_rewind(&B);
    while (btree_next(&B, &k, &d))
	fprintf(stdout, "%s:%s\n", (const char *)k, (const char *)d);

    return 0;
}
