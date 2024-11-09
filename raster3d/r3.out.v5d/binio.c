/* Vis5D version 5.0 */

/*
   Vis5D system for visualizing five dimensional gridded data sets
   Copyright (C) 1990 - 1997 Bill Hibbard, Johan Kellum, Brian Paul,
   Dave Santek, and Andre Battaiola.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * Functions to do binary I/O of floats, ints.
 *
 * >>>> These functions are built on top of Unix I/O functions, not stdio! <<<<
 *
 * The file format is assumed to be BIG-ENDIAN.
 * If this code is compiled with -DLITTLE and executes on a little endian
 * CPU then byte-swapping will be done.
 *
 * If an ANSI compiler is used prototypes and ANSI function declarations
 * are used.  Otherwise use K&R conventions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "binio.h"

/**********************************************************************/

/******                     Byte Flipping                         *****/

/**********************************************************************/

#define FLIP4(n)                                                              \
    ((n & 0xff000000) >> 24 | (n & 0x00ff0000) >> 8 | (n & 0x0000ff00) << 8 | \
     (n & 0x000000ff) << 24)

#define FLIP2(n) (((unsigned short)(n & 0xff00)) >> 8 | (n & 0x00ff) << 8)

/*
 * Flip the order of the 4 bytes in an array of 4-byte words.
 */
void flip4(const unsigned int *src, unsigned int *dest, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        unsigned int tmp = src[i];

        dest[i] = FLIP4(tmp);
    }
}

/*
 * Flip the order of the 2 bytes in an array of 2-byte words.
 */
void flip2(const unsigned short *src, unsigned short *dest, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        unsigned short tmp = src[i];

        dest[i] = FLIP2(tmp);
    }
}

/**********************************************************************/

/*****                     Read Functions                         *****/

/**********************************************************************/

/*
 * Read a block of bytes.
 *  Input:  f - the file descriptor to read from.
 *         b - address of buffer to read into.
 *         n - number of bytes to read.
 * Return:  number of bytes read, 0 if error.
 */
int read_bytes(int f, void *b, int n)
{
    return read(f, b, n);
}

/*
 * Read an array of 2-byte integers.
 * Input:  f - file descriptor
 *         iarray - address to put integers
 *         n - number of integers to read.
 * Return:  number of integers read.
 */
int read_int2_array(int f, short *iarray, int n)
{
    int nread = read(f, iarray, n * 2);

    if (nread <= 0)
        return 0;
#ifdef LITTLE
    flip2((const unsigned short *)iarray, (unsigned short *)iarray, nread / 2);
#endif
    return nread / 2;
}

/*
 * Read an array of unsigned 2-byte integers.
 * Input:  f - file descriptor
 *         iarray - address to put integers
 *         n - number of integers to read.
 * Return:  number of integers read.
 */
int read_uint2_array(int f, unsigned short *iarray, int n)
{
    int nread = read(f, iarray, n * 2);

    if (nread <= 0)
        return 0;
#ifdef LITTLE
    flip2(iarray, iarray, nread / 2);
#endif
    return nread / 2;
}

/*
 * Read a 4-byte integer.
 * Input:  f - the file descriptor to read from
 *         i - pointer to integer to put result into.
 * Return:  1 = ok, 0 = error
 */
int read_int4(int f, int *i)
{
#ifdef LITTLE
    /* read big endian and convert to little endian */
    unsigned int n;

    if (read(f, &n, 4) == 4) {
        *i = FLIP4(n);
        return 1;
    }
    else {
        return 0;
    }
#else
    if (read(f, i, 4) == 4) {
        return 1;
    }
    else {
        return 0;
    }
#endif
}

/*
 * Read an array of 4-byte integers.
 * Input:  f - file descriptor
 *         iarray - address to put integers
 *         n - number of integers to read.
 * Return:  number of integers read.
 */
int read_int4_array(int f, int *iarray, int n)
{
    int nread = read(f, iarray, 4 * n);

    if (nread <= 0)
        return 0;
#ifdef LITTLE
    flip4((const unsigned int *)iarray, (unsigned int *)iarray, nread / 4);
#endif
    return nread / 4;
}

/*
 * Read a 4-byte IEEE float.
 * Input:  f - the file descriptor to read from.
 *         x - pointer to float to put result into.
 * Return:  1 = ok, 0 = error
 */
int read_float4(int f, float *x)
{
#ifdef LITTLE
    unsigned int n, *iptr;

    if (read(f, &n, 4) == 4) {
        iptr = (unsigned int *)x;
        *iptr = FLIP4(n);
        return 1;
    }
    else {
        return 0;
    }
#else
    if (read(f, x, 4) == 4) {
        return 1;
    }
    else {
        return 0;
    }
#endif
}

/*
 * Read an array of 4-byte IEEE floats.
 * Input:  f - file descriptor
 *         x - address to put floats
 *         n - number of floats to read.
 * Return:  number of floats read.
 */
int read_float4_array(int f, float *x, int n)
{
    int nread = read(f, x, 4 * n);

    if (nread <= 0)
        return 0;
#ifdef LITTLE
    flip4((const unsigned int *)x, (unsigned int *)x, nread / 4);
#endif
    return nread / 4;
}

/*
 * Read a block of memory.
 * Input:  f - file descriptor
 *         data - address of first byte
 *         elements - number of elements to read
 *         elsize - size of each element to read (1, 2 or 4)
 * Return: number of elements written
 */
int read_block(int f, void *data, int elements, int elsize)
{
    if (elsize == 1) {
        return read(f, data, elements);
    }
    else if (elsize == 2) {
#ifdef LITTLE
        int n;

        n = read(f, data, elements * 2) / 2;
        if (n == elements) {
            flip2((const unsigned short *)data, (unsigned short *)data,
                  elements);
        }
        return n;
#else
        return read(f, data, elements * 2) / 2;
#endif
    }
    else if (elsize == 4) {
#ifdef LITTLE
        int n;

        n = read(f, data, elements * 4) / 4;
        if (n == elements) {
            flip4((const unsigned int *)data, (unsigned int *)data, elements);
        }
        return n;
#else
        return read(f, data, elements * 4) / 4;
#endif
    }
    else {
        printf("Fatal error in read_block(): bad elsize (%d)\n", elsize);
        abort();
    }
    return 0;
}

/**********************************************************************/

/*****                         Write Functions                    *****/

/**********************************************************************/

/*
 * Write a block of bytes.
 * Input:  f - the file descriptor to write to.
 *         b - address of buffer to write.
 *         n - number of bytes to write.
 * Return:  number of bytes written, 0 if error.
 */
int write_bytes(int f, const void *b, int n)
{
    return write(f, b, n);
}

/*
 * Write an array of 2-byte integers.
 * Input:  f - file descriptor
 *         iarray - address to put integers
 *         n - number of integers to write.
 * Return:  number of integers written
 */
int write_int2_array(int f, const short *iarray, int n)
{
    int nwritten;

#ifdef LITTLE
    flip2((const unsigned short *)iarray, (unsigned short *)iarray, n);
#endif
    nwritten = write(f, iarray, 2 * n);
#ifdef LITTLE
    flip2((const unsigned short *)iarray, (unsigned short *)iarray, n);
#endif
    if (nwritten <= 0)
        return 0;
    return nwritten / 2;
}

/*
 * Write an array of 2-byte unsigned integers.
 * Input:  f - file descriptor
 *         iarray - address to put integers
 *         n - number of integers to write.
 * Return:  number of integers written
 */
int write_uint2_array(int f, const unsigned short *iarray, int n)
{
    int nwritten;

#ifdef LITTLE
    flip2(iarray, (unsigned short *)iarray, n);
#endif
    nwritten = write(f, iarray, 2 * n);
#ifdef LITTLE
    flip2(iarray, (unsigned short *)iarray, n);
#endif
    if (nwritten <= 0)
        return 0;
    else
        return nwritten / 2;
}

/*
 * Write a 4-byte integer.
 *Input:  f - the file descriptor
 *         i - the integer
 * Return:  1 = ok, 0 = error
 */
int write_int4(int f, int i)
{
#ifdef LITTLE
    i = FLIP4(i);
#endif
    return write(f, &i, 4) > 0;
}

/*
 * Write an array of 4-byte integers.
 * Input:  f - the file descriptor
 *         i - the array of ints
 *           n - the number of ints in array
 *  Return:  number of integers written.
 */
int write_int4_array(int f, const int *i, int n)
{
#ifdef LITTLE
    int nwritten;

    flip4((const unsigned int *)i, (unsigned int *)i, n);
    nwritten = write(f, i, 4 * n);
    flip4((const unsigned int *)i, (unsigned int *)i, n);
    if (nwritten <= 0)
        return 0;
    else
        return nwritten / 4;
#else
    return write(f, i, 4 * n) / 4;
#endif
}

/*
 * Write a 4-byte IEEE float.
 * Input:  f - the file descriptor
 *         x - the float
 * Return:  1 = ok, 0 = error
 */
int write_float4(int f, float x)
{
#ifdef LITTLE
    float y;
    unsigned int *iptr = (unsigned int *)&y, temp;

    y = (float)x;
    temp = FLIP4(*iptr);
    return write(f, &temp, 4) > 0;
#else
    float y;

    y = (float)x;
    return write(f, &y, 4) > 0;
#endif
}

/*
 * Write an array of 4-byte IEEE floating point numbers.
 * Input:  f - the file descriptor
 *         x - the array of floats
 *         n - number of floats in array
 * Return:  number of float written.
 */
int write_float4_array(int f, const float *x, int n)
{
#ifdef LITTLE
    int nwritten;

    flip4((const unsigned int *)x, (unsigned int *)x, n);
    nwritten = write(f, x, 4 * n);
    flip4((const unsigned int *)x, (unsigned int *)x, n);
    if (nwritten <= 0)
        return 0;
    else
        return nwritten / 4;
#else
    return write(f, x, 4 * n) / 4;
#endif
}

/*
 * Write a block of memory.
 * Input:  f - file descriptor
 *         data - address of first byte
 *         elements - number of elements to write
 *         elsize - size of each element to write (1, 2 or 4)
 * Return: number of elements written
 */
int write_block(int f, const void *data, int elements, int elsize)
{
    if (elsize == 1) {
        return write(f, data, elements);
    }
    else if (elsize == 2) {
#ifdef LITTLE
        int n;

        flip2((const unsigned short *)data, (unsigned short *)data, elements);
        n = write(f, data, elements * 2) / 2;
        flip2((const unsigned short *)data, (unsigned short *)data, elements);
        return n;
#else
        return write(f, data, elements * 2) / 2;
#endif
    }
    else if (elsize == 4) {
#ifdef LITTLE
        int n;

        flip4((const unsigned int *)data, (unsigned int *)data, elements);
        n = write(f, data, elements * 4) / 4;
        flip4((const unsigned int *)data, (unsigned int *)data, elements);
        return n;
#else
        return write(f, data, elements * 4) / 4;
#endif
    }
    else {
        printf("Fatal error in write_block(): bad elsize (%d)\n", elsize);
        abort();
    }
    return 0;
}
