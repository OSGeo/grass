
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
 * If we're running on a CRAY (8-byte ints and floats), conversions will
 * be done as needed.
 */


/*
 * Updates:
 *
 * April 13, 1995, brianp
 *   added cray_to_ieee and iee_to_cray array conversion functions.
 *   fixed potential cray bug in write_float4_array function.
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef _CRAY
#  include <string.h>
#include <grass/gis.h>
#endif
#include "binio.h"




/**********************************************************************/

/******                     Byte Flipping                         *****/

/**********************************************************************/


#define FLIP4( n )  (  (n & 0xff000000) >> 24     \
                     | (n & 0x00ff0000) >> 8      \
                     | (n & 0x0000ff00) << 8      \
                     | (n & 0x000000ff) << 24  )


#define FLIP2( n )  (((unsigned short) (n & 0xff00)) >> 8  |  (n & 0x00ff) << 8)



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


#ifdef _CRAY

/*****************************************************************************
*
* The following source code is in the public domain.
* Specifically, we give to the public domain all rights for future licensing
* of the source code, all resale rights, and all publishing rights.
*
* We ask, but do not require, that the following message be included in all
* derived works:
*
* Portions developed at the National Center for Supercomputing Applications at
* the University of Illinois at Urbana-Champaign.
*
* THE UNIVERSITY OF ILLINOIS GIVES NO WARRANTY, EXPRESSED OR IMPLIED, FOR THE
* SOFTWARE AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT LIMITATION,
* WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE
*
****************************************************************************/

/** THESE ROUTINES MUST BE COMPILED ON THE CRAY ONLY SINCE THEY **/

/** REQUIRE 8-BYTES PER C-TYPE LONG                             **/

/* Cray to IEEE single precision */
static void c_to_if(long *t, const long *f)
{
    if (*f != 0) {
	*t = (((*f & 0x8000000000000000) |	/* sign bit */
	       ((((*f & 0x7fff000000000000) >> 48) - 16258) << 55)) +	/* exp */
	      (((*f & 0x00007fffff000000) + ((*f & 0x0000000000800000) << 1)) << 8));	/* mantissa */
    }
    else
	*t = *f;
}


#define C_TO_IF( T, F )							\
	if (F != 0) {							\
		T = (((F & 0x8000000000000000) |			\
		((((F & 0x7fff000000000000) >> 48)-16258) << 55)) +	\
		(((F & 0x00007fffff000000) +				\
		((F & 0x0000000000800000) << 1)) << 8));		\
	}								\
	else {								\
		T = F;							\
	}



/* IEEE single precison to Cray */
static void if_to_c(long *t, const long *f)
{
    if (*f != 0) {
	*t = (((*f & 0x8000000000000000) |
	       ((*f & 0x7f80000000000000) >> 7) +
	       (16258 << 48)) |
	      (((*f & 0x007fffff00000000) >> 8) | (0x0000800000000000)));
	if ((*f << 1) == 0)
	    *t = 0;
    }
    else
	*t = *f;
}

/* T and F must be longs! */
#define IF_TO_C( T, F )							\
	if (F != 0) {							\
		T = (((F & 0x8000000000000000) |			\
		((F & 0x7f80000000000000) >> 7) +			\
		(16258 << 48)) |					\
		(((F & 0x007fffff00000000) >> 8) | (0x0000800000000000)));  \
		if ((F << 1) == 0) T = 0;				\
	}								\
	else {								\
		T = F;							\
	}




/*
 * Convert an array of Cray 8-byte floats to an array of IEEE 4-byte floats.
 */
void cray_to_ieee_array(long *dest, const float *source, int n)
{
    long *dst;
    const long *src;
    long tmp1, tmp2;
    int i;

    dst = dest;
    src = (const long *)source;

    for (i = 0; i < n; i += 2) {	/* add 1 in case n is odd */
	c_to_if(&tmp1, &src[i]);
	c_to_if(&tmp2, &src[i + 1]);
	*dst = (tmp1 & 0xffffffff00000000) | (tmp2 >> 32);
	dst++;
    }
}



/*
 * Convert an array of IEEE 4-byte floats to an array of 8-byte Cray floats.
 */
void ieee_to_cray_array(float *dest, const long *source, int n)
{
    long *dst;
    const long *src;
    int i;
    long ieee;

    src = source;
    dst = (long *)dest;

    for (i = 0; i < n; i++) {
	/* most significant 4-bytes of ieee contain bit pattern to convert */
	if ((i & 1) == 0) {
	    /* get upper half */
	    ieee = src[i / 2] & 0xffffffff00000000;
	}
	else {
	    /* get lower half */
	    ieee = src[i / 2] << 32;
	}
	if_to_c(dst, &ieee);
	dst++;
    }
}


#endif /*_CRAY*/



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
#ifdef _CRAY
    int i;
    signed char *buffer;
    int nread;

    buffer = (signed char *)G_malloc(n * 2);
    if (!buffer)
	return 0;
    nread = read(f, buffer, n * 2);
    if (nread <= 0)
	return 0;
    nread /= 2;
    for (i = 0; i < nread; i++) {
	/* don't forget about sign extension! */
	iarray[i] = (buffer[i * 2] * 256) | buffer[i * 2 + 1];
    }
    G_free(buffer);
    return nread;
#else
    int nread = read(f, iarray, n * 2);

    if (nread <= 0)
	return 0;
#ifdef LITTLE
    flip2((const unsigned short *)iarray, (unsigned short *)iarray,
	  nread / 2);
#endif
    return nread / 2;
#endif
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
#ifdef _CRAY
    int i;
    unsigned char *buffer;
    int nread;

    buffer = (unsigned char *)G_malloc(n * 2);
    if (!buffer)
	return 0;
    nread = read(f, buffer, n * 2);
    if (nread <= 0)
	return 0;
    nread /= 2;
    for (i = 0; i < nread; i++) {
	iarray[i] = (buffer[i * 2] << 8) | buffer[i * 2 + 1];
    }
    G_free(buffer);
    return nread;
#else
    int nread = read(f, iarray, n * 2);

    if (nread <= 0)
	return 0;
#ifdef LITTLE
    flip2(iarray, iarray, nread / 2);
#endif
    return nread / 2;
#endif
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
#  ifdef _CRAY
	*i = *i >> 32;
#  endif
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
#ifdef _CRAY
    int j, nread;
    int *buffer;

    buffer = (int *)G_malloc((n + 1) * 4);
    if (!buffer)
	return 0;
    nread = read(f, buffer, 4 * n);
    if (nread <= 0) {
	return 0;
    }
    nread /= 4;

    for (j = 0; j < nread; j++) {
	if ((j & 1) == 0) {
	    iarray[j] = buffer[j / 2] >> 32;
	}
	else {
	    iarray[j] = buffer[j / 2] & 0xffffffff;
	}
    }
    G_free(buffer);
    return nread;
#else
    int nread = read(f, iarray, 4 * n);

    if (nread <= 0)
	return 0;
#  ifdef LITTLE
    flip4((const unsigned int *)iarray, (unsigned int *)iarray, nread / 4);
#  endif
    return nread / 4;
#endif
}



/*
 * Read a 4-byte IEEE float.
 * Input:  f - the file descriptor to read from.
 *         x - pointer to float to put result into.
 * Return:  1 = ok, 0 = error
 */
int read_float4(int f, float *x)
{
#ifdef _CRAY
    long buffer = 0;

    if (read(f, &buffer, 4) == 4) {
	/* convert IEEE float (buffer) to Cray float (x) */
	if_to_c((long *)x, &buffer);
	return 1;
    }
    return 0;
#else
#  ifdef LITTLE
    unsigned int n, *iptr;

    if (read(f, &n, 4) == 4) {
	iptr = (unsigned int *)x;
	*iptr = FLIP4(n);
	return 1;
    }
    else {
	return 0;
    }
#  else
    if (read(f, x, 4) == 4) {
	return 1;
    }
    else {
	return 0;
    }
#  endif
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
#ifdef _CRAY
    /* read IEEE floats into buffer, then convert to Cray format */
    long *buffer;
    int i, nread;

    buffer = (long *)G_malloc((n + 1) * 4);
    if (!buffer)
	return 0;
    nread = read(f, buffer, n * 4);
    if (nread <= 0)
	return 0;
    nread /= 4;
    ieee_to_cray_array(x, buffer, nread);
    G_free(buffer);
    return nread;
#else
    int nread = read(f, x, 4 * n);

    if (nread <= 0)
	return 0;
#ifdef LITTLE
    flip4((const unsigned int *)x, (unsigned int *)x, nread / 4);
#endif
    return nread / 4;
#endif
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
#ifdef _CRAY
    printf("write_int2_array not implemented!\n");
    exit(1);
#else
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
#endif
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
#ifdef _CRAY
    int i, nwritten;
    unsigned char *buffer;

    buffer = (unsigned char *)G_malloc(2 * n);
    if (!buffer)
	return 0;
    for (i = 0; i < n; i++) {
	buffer[i * 2] = (iarray[i] >> 8) & 0xff;
	buffer[i * 2 + 1] = iarray[i] & 0xff;
    }
    nwritten = write(f, buffer, 2 * n);
    G_free(buffer);
    if (nwritten <= 0)
	return 0;
    else
	return nwritten / 2;
#else
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
#endif
}



/*
 * Write a 4-byte integer.
 *Input:  f - the file descriptor
 *         i - the integer
 * Return:  1 = ok, 0 = error
 */
int write_int4(int f, int i)
{
#ifdef _CRAY
    i = i << 32;
    return write(f, &i, 4) > 0;
#else
#  ifdef LITTLE
    i = FLIP4(i);
#  endif
    return write(f, &i, 4) > 0;
#endif
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
#ifdef _CRAY
    int j, nwritten;
    char *buf, *b, *ptr;

    b = buf = (char *)G_malloc(n * 4 + 8);
    if (!b)
	return 0;
    ptr = (char *)i;
    for (j = 0; j < n; j++) {
	ptr += 4;		/* skip upper 4 bytes */
	*b++ = *ptr++;
	*b++ = *ptr++;
	*b++ = *ptr++;
	*b++ = *ptr++;
    }
    nwritten = write(f, buf, 4 * n);
    G_free(buf);
    if (nwritten <= 0)
	return 0;
    else
	return nwritten / 4;
#else
#  ifdef LITTLE
    int nwritten;

    flip4((const unsigned int *)i, (unsigned int *)i, n);
    nwritten = write(f, i, 4 * n);
    flip4((const unsigned int *)i, (unsigned int *)i, n);
    if (nwritten <= 0)
	return 0;
    else
	return nwritten / 4;
#  else
    return write(f, i, 4 * n) / 4;
#  endif
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
#ifdef _CRAY
    char buffer[8];

    c_to_if((long *)buffer, (const long *)&x);
    return write(f, buffer, 4) > 0;
#else
#  ifdef LITTLE
    float y;
    unsigned int *iptr = (unsigned int *)&y, temp;

    y = (float)x;
    temp = FLIP4(*iptr);
    return write(f, &temp, 4) > 0;
#  else
    float y;

    y = (float)x;
    return write(f, &y, 4) > 0;
#  endif
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
#ifdef _CRAY
    /* convert cray floats to IEEE and put into buffer */
    int nwritten;
    long *buffer;

    buffer = (long *)G_malloc(n * 4 + 8);
    if (!buffer)
	return 0;
    cray_to_ieee_array(buffer, x, n);
    nwritten = write(f, buffer, 4 * n);
    G_free(buffer);
    if (nwritten <= 0)
	return 0;
    else
	return nwritten / 4;
#else
#  ifdef LITTLE
    int nwritten;

    flip4((const unsigned int *)x, (unsigned int *)x, n);
    nwritten = write(f, x, 4 * n);
    flip4((const unsigned int *)x, (unsigned int *)x, n);
    if (nwritten <= 0)
	return 0;
    else
	return nwritten / 4;
#  else
    return write(f, x, 4 * n) / 4;
#  endif
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
