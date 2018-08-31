/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <grass/vector.h>

/*
 **  Written by Dave Gerdes  9/1988
 **  US Army Construction Engineering Research Lab
 */


/* 
 ** 
 **  This code is a quick hack to allow the writing of portable
 **  binary data files.
 **  The approach is to take known values and compare them against
 **  the current machine's internal representation.   A cross reference
 **  table is then built, and then all file reads and writes must go through
 **  through these routines to correct the numbers if need be.
 **
 **  As long as the byte switching is symmetrical, the conversion routines
 **  will work both directions.

 **  The integer test patterns are quite simple, and their choice was
 **  arbitrary, but the float and double valued were more critical.

 **  I did not have a specification for IEEE to go by, so it is possible
 **  that I have missed something.  My criteria were:
 **
 **  First, true IEEE numbers had to be chosen to avoid getting an FPE.
 **  Second, every byte in the test pattern had to be unique.   And
 **  finally, the number had to not be sensitive to rounding by the 
 **  specific hardware implementation.
 **
 **  By experimentation it was found that the number  1.3333  met
 **  all these criteria for both floats and doubles

 **  See the discourse at the end of this file for more information
 **  
 **
 */

#define TEST_PATTERN 1.3333
#ifdef HAVE_LONG_LONG_INT
#define OFF_T_TEST 0x0102030405060708LL
#else
#define OFF_T_TEST 0x01020304
#endif
#define LONG_TEST 0x01020304
#define INT_TEST 0x01020304
#define SHORT_TEST 0x0102

union type_conv
{
    double d;
    float f;
    off_t o;
    long l;
    int i;
    short s;
    unsigned char c[PORT_DOUBLE];
};
static union type_conv u;

/* dbl_cmpr holds the bytes of an IEEE representation of  TEST_PATTERN */
static unsigned char dbl_cmpr[] =
    { 0x3f, 0xf5, 0x55, 0x32, 0x61, 0x7c, 0x1b, 0xda };
/* flt_cmpr holds the bytes of an IEEE representation of  TEST_PATTERN */
static unsigned char flt_cmpr[] = { 0x3f, 0xaa, 0xa9, 0x93 };
static unsigned char off_t_cmpr[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
static unsigned char lng_cmpr[] = { 0x01, 0x02, 0x03, 0x04 };
static unsigned char int_cmpr[] = { 0x01, 0x02, 0x03, 0x04 };
static unsigned char shrt_cmpr[] = { 0x01, 0x02 };

static char dbl_cnvrt[sizeof(double)];
static char flt_cnvrt[sizeof(float)];
static char off_t_cnvrt[sizeof(off_t)];
static char lng_cnvrt[sizeof(long)];
static char int_cnvrt[sizeof(int)];
static char shrt_cnvrt[sizeof(short)];

static int nat_dbl, nat_flt, nat_lng, nat_off_t, nat_int, nat_shrt, nat_char;


/* function prototypes */
static int find_offset(unsigned char *, unsigned char, int);
static int dumpflags(void);


int main(int argc, char **argv)
{
    register int i;
    int tmp, tmp2;
    int err = 0;
    int dbl_order, flt_order, lng_order, int_order, shrt_order;

    /* Find native sizes */
    printf("\n/* Native machine sizes */\n");
    printf("#define NATIVE_DOUBLE %d\n", (nat_dbl = sizeof(double)));
    printf("#define NATIVE_FLOAT  %d\n", (nat_flt = sizeof(float)));
    printf("#define NATIVE_OFF_T  %d\n", (nat_off_t = sizeof(off_t)));
    printf("#define NATIVE_LONG   %d\n", (nat_lng = sizeof(long)));
    printf("#define NATIVE_INT    %d\n", (nat_int = sizeof(int)));
    printf("#define NATIVE_SHORT  %d\n", (nat_shrt = sizeof(short)));
    printf("#define NATIVE_CHAR   %d\n", (nat_char = sizeof(char)));

    /* Following code checks only if all assumptions are fulfilled */
    /* Check sizes */
    if (nat_dbl != PORT_DOUBLE) {
	fprintf(stderr, "ERROR, sizeof (double) != %d\n", PORT_DOUBLE);
	err = 1;
    }
    if (nat_flt != PORT_FLOAT) {
	fprintf(stderr, "ERROR, sizeof (float) != %d\n", PORT_FLOAT);
	err = 1;
    }
    /* port_off_t is variable */
    if (nat_lng < PORT_LONG) {
	fprintf(stderr, "ERROR, sizeof (long) < %d\n", PORT_LONG);
	err = 1;
    }
    if (nat_int < PORT_INT) {
	fprintf(stderr, "ERROR, sizeof (int) < %d\n", PORT_INT);
	err = 1;
    }
    if (nat_shrt < PORT_SHORT) {
	fprintf(stderr, "ERROR, sizeof (short) < %d\n", PORT_SHORT);
	err = 1;
    }
    if (nat_char != PORT_CHAR) {
	fprintf(stderr, "ERROR, sizeof (char) != %d\n", PORT_CHAR);
	err = 1;
    }

    /* Find for each byte in big endian test pattern (*_cmpr) 
     * offset of corresponding byte in machine native order.
     * Look if native byte order is little or big or some other (pdp)
     * endian.
     */
    /* Find double order */
    u.d = TEST_PATTERN;
    for (i = 0; i < PORT_DOUBLE; i++) {
	tmp = find_offset(u.c, dbl_cmpr[i], PORT_DOUBLE);
	if (-1 == tmp) {
	    fprintf(stderr, "ERROR, could not find '%x' in double\n",
		    dbl_cmpr[i]);
	    err = 1;
	}
	dbl_cnvrt[i] = tmp;
    }
    tmp = tmp2 = 1;
    for (i = 0; i < PORT_DOUBLE; i++) {
	if (dbl_cnvrt[i] != i)
	    tmp = 0;		/* isn't big endian */
	if (dbl_cnvrt[i] != (PORT_DOUBLE - i - 1))
	    tmp2 = 0;		/* isn't little endian */
    }
    if (tmp)
	dbl_order = ENDIAN_BIG;
    else if (tmp2)
	dbl_order = ENDIAN_LITTLE;
    else
	dbl_order = ENDIAN_OTHER;

    /* Find float order */
    u.f = TEST_PATTERN;
    for (i = 0; i < PORT_FLOAT; i++) {
	tmp = find_offset(u.c, flt_cmpr[i], PORT_FLOAT);
	if (-1 == tmp) {
	    fprintf(stderr, "ERROR, could not find '%x' in float\n",
		    flt_cmpr[i]);
	    err = 1;
	}
	flt_cnvrt[i] = tmp;
    }
    tmp = tmp2 = 1;
    for (i = 0; i < PORT_FLOAT; i++) {
	if (flt_cnvrt[i] != i)
	    tmp = 0;
	if (flt_cnvrt[i] != (PORT_FLOAT - i - 1))
	    tmp2 = 0;
    }
    if (tmp)
	flt_order = ENDIAN_BIG;
    else if (tmp2)
	flt_order = ENDIAN_LITTLE;
    else
	flt_order = ENDIAN_OTHER;

    /* Find off_t order */
    if (nat_off_t == 8)
	u.o = OFF_T_TEST;
    else
	u.o = LONG_TEST;
    for (i = 0; i < nat_off_t; i++) {
	tmp = find_offset(u.c, off_t_cmpr[i], nat_off_t);
	if (-1 == tmp) {
	    fprintf(stderr, "ERROR, could not find '%x' in off_t\n",
		    off_t_cmpr[i]);
	    err = 1;
	}
	off_t_cnvrt[i] = tmp;
    }
    tmp = tmp2 = 1;
    for (i = 0; i < nat_off_t; i++) {
	if (off_t_cnvrt[i] != (i + (nat_off_t - nat_off_t)))
	    tmp = 0;
	if (off_t_cnvrt[i] != (nat_off_t - i - 1))
	    tmp2 = 0;
    }
    if (tmp)
	off_t_order = ENDIAN_BIG;
    else if (tmp2)
	off_t_order = ENDIAN_LITTLE;
    else
	off_t_order = ENDIAN_OTHER;

    /* Find long order */
    u.l = LONG_TEST;
    for (i = 0; i < PORT_LONG; i++) {
	tmp = find_offset(u.c, lng_cmpr[i], nat_lng);
	if (-1 == tmp) {
	    fprintf(stderr, "ERROR, could not find '%x' in long\n",
		    lng_cmpr[i]);
	    err = 1;
	}
	lng_cnvrt[i] = tmp;
    }
    tmp = tmp2 = 1;
    for (i = 0; i < PORT_LONG; i++) {
	if (lng_cnvrt[i] != (i + (nat_lng - PORT_LONG)))
	    tmp = 0;
	if (lng_cnvrt[i] != (PORT_LONG - i - 1))
	    tmp2 = 0;
    }
    if (tmp)
	lng_order = ENDIAN_BIG;
    else if (tmp2)
	lng_order = ENDIAN_LITTLE;
    else
	lng_order = ENDIAN_OTHER;

    /* Find int order */
    u.i = INT_TEST;
    for (i = 0; i < PORT_INT; i++) {
	tmp = find_offset(u.c, int_cmpr[i], nat_int);
	if (-1 == tmp) {
	    fprintf(stderr, "ERROR, could not find '%x' in int\n",
		    int_cmpr[i]);
	    err = 1;
	}
	int_cnvrt[i] = tmp;
    }
    tmp = tmp2 = 1;
    for (i = 0; i < PORT_INT; i++) {
	if (int_cnvrt[i] != (i + (nat_lng - PORT_LONG)))
	    tmp = 0;
	if (int_cnvrt[i] != (PORT_INT - i - 1))
	    tmp2 = 0;
    }
    if (tmp)
	int_order = ENDIAN_BIG;
    else if (tmp2)
	int_order = ENDIAN_LITTLE;
    else
	int_order = ENDIAN_OTHER;

    /* Find short order */
    u.s = SHORT_TEST;
    for (i = 0; i < PORT_SHORT; i++) {
	tmp = find_offset(u.c, shrt_cmpr[i], nat_shrt);
	if (-1 == tmp) {
	    fprintf(stderr, "ERROR, could not find '%x' in shrt\n",
		    shrt_cmpr[i]);
	    err = 1;
	}
	shrt_cnvrt[i] = tmp;
    }
    tmp = tmp2 = 1;
    for (i = 0; i < PORT_SHORT; i++) {
	if (shrt_cnvrt[i] != (i + (nat_shrt - PORT_SHORT)))
	    tmp = 0;
	if (shrt_cnvrt[i] != (PORT_SHORT - i - 1))
	    tmp2 = 0;
    }
    if (tmp)
	shrt_order = ENDIAN_BIG;
    else if (tmp2)
	shrt_order = ENDIAN_LITTLE;
    else
	shrt_order = ENDIAN_OTHER;

    printf("\n/* Native machine byte orders */\n");
    printf("#define DOUBLE_ORDER %d\n", dbl_order);
    printf("#define FLOAT_ORDER  %d\n", flt_order);
    printf("#define OFF_T_ORDER  %d\n", off_t_order);
    printf("#define LONG_ORDER   %d\n", lng_order);
    printf("#define INT_ORDER    %d\n", int_order);
    printf("#define SHORT_ORDER  %d\n", shrt_order);

    printf("\n\n/* Translation matrices from big endian to native */\n");
    dumpflags();

    return (err);
}


/*
 ** match search_value against each char in basis. 
 ** return offset or -1 if not found
 */
static int
find_offset(unsigned char *basis, unsigned char search_value, int size)
{
    register int i;

    for (i = 0; i < size; i++)
	if (basis[i] == search_value)
	    return (i);

    return (-1);
}


static int dumpflags(void)
{
    int i;

    fprintf(stdout, "\n/* Double format: */\nstatic int dbl_cnvrt[] = {");
    i = 0;
    while (i < nat_dbl) {
	fprintf(stdout, "%d", dbl_cnvrt[i]);
	if (++i < nat_dbl)
	    fprintf(stdout, ", ");
    }
    fprintf(stdout, "};\n\n");

    fprintf(stdout, "/* Float format : */\nstatic int flt_cnvrt[] = {");
    i = 0;
    while (i < nat_flt) {
	fprintf(stdout, "%d", flt_cnvrt[i]);
	if (++i < nat_flt)
	    fprintf(stdout, ", ");
    }
    fprintf(stdout, "};\n\n");

    fprintf(stdout, "/* off_t format  : */\nstatic int off_t_cnvrt[] = {");
    i = 0;
    while (i < nat_off_t) {
	fprintf(stdout, "%d", off_t_cnvrt[i]);
	if (++i < nat_off_t)
	    fprintf(stdout, ", ");
    }
    fprintf(stdout, "};\n\n");

    fprintf(stdout, "/* Long format  : */\nstatic int lng_cnvrt[] = {");
    i = 0;
    while (i < nat_lng) {
	fprintf(stdout, "%d", lng_cnvrt[i]);
	if (++i < nat_lng)
	    fprintf(stdout, ", ");
    }
    fprintf(stdout, "};\n\n");

    fprintf(stdout, "/* Int format  : */\nstatic int int_cnvrt[] = {");
    i = 0;
    while (i < nat_int) {
	fprintf(stdout, "%d", int_cnvrt[i]);
	if (++i < nat_int)
	    fprintf(stdout, ", ");
    }
    fprintf(stdout, "};\n\n");

    fprintf(stdout, "/* Short format : */\nstatic int shrt_cnvrt[] = {");
    i = 0;
    while (i < nat_shrt) {
	fprintf(stdout, "%d", shrt_cnvrt[i]);
	if (++i < nat_shrt)
	    fprintf(stdout, ", ");
    }
    fprintf(stdout, "};\n\n");

    return 0;
}

/*

   The 3.0 dig, and dig_plus files are inherently non-portable.  This 
   can be seen in moving files between a SUN 386i and other SUN machines.
   The recommended way to transport files was always to convert to ASCII
   (b.a.vect) and copy the ASCII files:  dig_ascii and dig_att to the 
   destination machine.

   The problem lies in the way that different architectures internally
   represent data.   If a number is internally store as  0x01020304 on
   a 680x0 family machine, the same number will be stored as
   0x04030201 on an 80386 class machine.

   The CERL port of GRASS to the Compaq 386 already has code to deal
   with this incompatibility.  This code converts all files that are written
   out to conform to the 680x0 standard.  These binary files can then be 
   shared between machines without conversion.
   This code is designed to work with the majority of computers in use
   today that fit the following requirements:
   byte     ==  8 bits
   int      ==  4 bytes
   long     ==  4 bytes
   double   ==  IEEE standard 64 bit
   float    ==  IEEE standard 32 bit
   bytes can be swapped around in any reasonable way, but bits within each
   byte must be maintained in normal high to low ordering:  76543210

   If this ability is desired on a SUN 386i, for example, you simply
   define the compiler flag  CERL_PORTABLE in the src/CMD/makehead  file
   and recompile all of the mapdev programs.


   Binary DLG files are NOT supported by this code, and will continue to
   be non-portable between different architectures.


   -dave gerdes
 */
