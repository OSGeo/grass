#include <grass/config.h>

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "local_proto.h"


/*
 * next_row_addr() - Sets offset of next row
 *
 * RETURN: EXIT_SUCCESS / EXIT_FAILURE or number of bytes read
 */
int next_row_addr(int fd, off_t * offset, int nbytes)
{
    unsigned char buf[256];
    int i;

    /* nbytes <=0 means pre 3.0 compression */
    if (nbytes <= 0)
	return (read(fd, offset, sizeof(*offset)) == sizeof(*offset));

    /* 3.0 compression */
    if (read(fd, buf, (size_t) nbytes) != nbytes)
	return EXIT_FAILURE;

    *offset = 0;
    for (i = 0; i < nbytes; i++)
	*offset = *offset * 256 + buf[i];

    return EXIT_SUCCESS;
}
