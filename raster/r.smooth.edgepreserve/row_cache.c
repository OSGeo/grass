/*
 * r.smooth.edgepreserve row cache
 *
 *   Copyright 2025 by Maris Nartiss, and The GRASS Development Team
 *   Author: Maris Nartiss
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <grass/rowio.h>
#include <grass/glocale.h>

#include "local_proto.h"

/* For rowio */
int rowio_get_row(int fd, void *buf, int row, int buf_len)
{
    if (lseek(fd, ((off_t)row) * buf_len, SEEK_SET) == -1) {
        int err = errno;
        G_fatal_error(_("Seek error on temp file. %d: %s"), err, strerror(err));
    }

    ssize_t reads = read(fd, buf, (size_t)buf_len);
    if (reads == -1) {
        int err = errno;
        G_fatal_error(
            _("There was an error reading data from a temporary file. %d: %s"),
            err, strerror(err));
    }

    return (reads == buf_len);
}

/* For rowio */
int rowio_put_row(int fd, const void *buf, int row, int buf_len)
{
    // can't use G_fseek, as rowio operates on file descriptors
    if (lseek(fd, ((off_t)row) * buf_len, SEEK_SET) == -1) {
        int err = errno;
        G_fatal_error(_("Seek error on temp file. %d: %s"), err, strerror(err));
    }

    ssize_t writes = write(fd, buf, (size_t)buf_len);
    if (writes == -1) {
        int err = errno;
        G_fatal_error(
            _("There was an error writing data from a temporary file. %d: %s"),
            err, strerror(err));
    }

    return (writes == buf_len);
}

/* Function to use if cache is disk based */
DCELL *get_rowio(int row, struct Row_cache *row_cache)
{
    /* Rowio_get returned buffer is owned by the Rowio and might get
     * a new content on subsequent get call.
     */
    DCELL *buf = Rowio_get(&(row_cache->rowio_cache), row);
    if (buf == NULL)
        G_fatal_error(_("Error fetching data from a disk cache"));
    DCELL *target = G_malloc(row_cache->len);
    memcpy(target, buf, row_cache->len);
    return target;
}

void put_rowio(DCELL *buf, int row, struct Row_cache *row_cache)
{
    /* Rowio_put will memcpy buffer to a different one, thus it is safe to reuse
     */
    Rowio_put(&(row_cache->rowio_cache), buf, row);
    G_free(buf);
}

/* Function to use if cache is RAM based */
DCELL *get_ram(int row, struct Row_cache *row_cache)
{
    return row_cache->matrix[row];
}

void put_ram(DCELL *buf, int row, struct Row_cache *row_cache)
{
    row_cache->matrix[row] = buf;
}

/* For filling with initial data */
void fill_ram(DCELL *buf, int row, struct Row_cache *row_cache)
{
    memcpy(row_cache->matrix[row], buf, row_cache->len);
}

void fill_rowio(DCELL *buf, int row, struct Row_cache *row_cache)
{
    /* rowio.put calls memcpy thus buf reuse is safe */
    Rowio_put(&(row_cache->rowio_cache), buf, row);
}

/* Set up temporary storage for intermediate step data */
void setup_row_cache(int nrows, int ncols, double max_ram,
                     struct Row_cache *row_cache)
{
    /* 1 cell padding on each side */
    row_cache->nrows = nrows + 2;
    row_cache->ncols = ncols + 2;
    row_cache->len = (size_t)(row_cache->ncols) * sizeof(DCELL);
    /* Try to keep in RAM as much as possible */
    double max_rows = max_ram / ((double)(row_cache->len) / (1024.0 * 1024));
    /* 22 rows are used for computation */
    row_cache->use_rowio = max_rows - 22 > nrows ? false : true;

    if (row_cache->use_rowio) {
        if (max_rows < 24)
            G_fatal_error(_("Insufficient memory to hold processed data. "
                            "Either increase available memory with the "
                            "\"memory\" parameter "
                            "or reduce size of the computational region."));
        G_verbose_message(_("Using disk for temporary data storage"));
        /* Store data for looping over in a temporary file */
        row_cache->tmp_name = G_tempfile();
        errno = 0;
        /* Here could be upgraded to use O_TMPFILE */
        row_cache->tmp_fd = open(row_cache->tmp_name, O_RDWR | O_CREAT | O_EXCL,
                                 S_IRUSR | S_IWUSR);
        if (row_cache->tmp_fd == -1)
            G_fatal_error(_("Error creating row cache. %d: %s"), errno,
                          strerror(errno));
        int cache_nrows = (int)max_rows - 22;
        if (Rowio_setup(&(row_cache->rowio_cache), row_cache->tmp_fd,
                        cache_nrows, (int)row_cache->len, &rowio_get_row,
                        &rowio_put_row) != 1)
            G_fatal_error(_("Error creating row cache"));
        row_cache->get = &get_rowio;
        row_cache->put = &put_rowio;
        row_cache->fill = &fill_rowio;
    }
    else {
        G_verbose_message(_("Keeping temporary data in RAM"));
        /* Everything fits into RAM, no need for disk access */
        row_cache->matrix = (DCELL **)G_malloc(
            (long unsigned int)row_cache->nrows * sizeof(DCELL *));
        PRAGMA_IVDEP
        for (int row = 0; row < row_cache->nrows; row++) {
            row_cache->matrix[row] = (DCELL *)G_malloc(row_cache->len);
        }
        row_cache->get = &get_ram;
        row_cache->put = &put_ram;
        row_cache->fill = &fill_ram;
    }
}

void teardown_row_cache(struct Row_cache *row_cache)
{
    if (row_cache->use_rowio) {
        Rowio_release(&(row_cache->rowio_cache));
        errno = 0;
        if (close(row_cache->tmp_fd) == -1 || unlink(row_cache->tmp_name) == -1)
            G_warning(_("Error cleaning up row cache. %d: %s"), errno,
                      strerror(errno));
        G_free(row_cache->tmp_name);
    }
    else {
        PRAGMA_IVDEP
        for (int row = 0; row < row_cache->nrows; row++) {
            G_free(row_cache->matrix[row]);
        }
        G_free(row_cache->matrix);
    }
}
