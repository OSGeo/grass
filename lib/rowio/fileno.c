#include <grass/rowio.h>

/*!
 * \brief get file descriptor
 *
 * Rowio_fileno()
 * returns the file descriptor associated with the ROWIO structure.
 *
 *  \param r
 *  \return int
 */

int rowio_fileno(const ROWIO * R)
{
    return R->fd;
}
