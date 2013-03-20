#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "raster3d_intern.h"

/*--------------------------------------------------------------------------*/

#define XDR_DOUBLE_LENGTH 8
#define XDR_DOUBLE_NOF_EXP_BYTES 2
#define XDR_FLOAT_LENGTH 4
#define XDR_FLOAT_NOF_EXP_BYTES 1

/*--------------------------------------------------------------------------*/

void Rast3d_fpcompress_print_binary(char *c, int numBits)
{
    unsigned char bit;
    int i;

    bit = 1 << (numBits - 1);

    for (i = 0; i < numBits; i++) {
	printf("%d", (*((unsigned char *)c) & bit) != 0);
	bit >>= 1;
    }
}

/*--------------------------------------------------------------------------*/

void Rast3d_fpcompress_dissect_xdr_double(unsigned char *numPointer)
{
    char sign, exponent;

    sign = *numPointer >> 7;
    exponent = (*numPointer << 1) | (*(numPointer + 1) >> 7);

    printf("%f: sign = ", *((float *)numPointer));
    Rast3d_fpcompress_print_binary(&sign, 1);
    printf("   exp = ");
    Rast3d_fpcompress_print_binary(&exponent, 8);
    printf("   mantissa = ");
    Rast3d_fpcompress_print_binary((char *)(numPointer + 1), 7);
    Rast3d_fpcompress_print_binary((char *)(numPointer + 2), 8);
    Rast3d_fpcompress_print_binary((char *)(numPointer + 3), 8);
    printf("\n");
}

/*--------------------------------------------------------------------------*/

static unsigned char clearMask[9] =
    { 255, 128, 192, 224, 240, 248, 252, 254, 255 };

/*--------------------------------------------------------------------------*/

#define ALL_NULL_CODE 2
#define ZERO_NULL_CODE 1
#define SOME_NULL_CODE 0

/*--------------------------------------------------------------------------*/

static void
G_fpcompress_rearrangeEncodeFloats(unsigned char *src, int size,
				   int precision, unsigned char *dst,
				   int *length, int *offsetMantissa)
{
    unsigned int nNullBits, nBits;
    register unsigned char *srcStop;
    register unsigned char *cp0, *cp1, *cp2, *cp3, *nullBits;
    unsigned char mask, isNull;
    int gt8, gt16, srcIncrement, nofNull;
    float *f;

    srcStop = src + size * XDR_FLOAT_LENGTH;

    if ((precision >= 23) || (precision == -1)) {
	cp3 = dst;
	cp2 = cp3 + size;
	cp1 = cp2 + size;
	cp0 = cp1 + size;
	while (srcStop != src) {
	    *cp3++ = *src++;	/* sign + 7 exponent bits */
	    *cp2++ = *src++;	/* 1 exponent bit + 7 ms mantissa bits */
	    *cp1++ = *src++;	/* 8 mantissa bits */
	    *cp0++ = *src++;	/* 8 ls mantissa bits */
	}

	*length = size * XDR_FLOAT_LENGTH;
	*offsetMantissa = size;

	return;
    }

    f = (float *)src;
    nofNull = 0;
    while (srcStop != (unsigned char *)f)
	nofNull += Rast3d_is_xdr_null_float(f++);

    if (nofNull == size) {
	*dst = (unsigned char)ALL_NULL_CODE;

	*length = 1;
	*offsetMantissa = 1;

	return;
    }

    precision += 1;		/* treat the ls exponent bit like an addl mantissa bit */

    *dst = (unsigned char)(nofNull == 0 ? ZERO_NULL_CODE : SOME_NULL_CODE);

    gt16 = precision > 16;
    gt8 = precision > 8;
    srcIncrement = 1 + (!gt8) + (!gt16);

    precision %= 8;

    nullBits = dst + 1;
    if (nofNull)
	cp0 = nullBits + size / 8 + ((size % 8) != 0);
    else
	cp0 = nullBits;
    cp3 = cp0 + size - nofNull;
    cp2 = cp3 + size - nofNull;
    cp1 = cp3 + (gt8 + gt16) * (size - nofNull);

    mask = clearMask[precision];
    nBits = nNullBits = 0;

    while (srcStop != src) {
	if (nofNull) {
	    isNull = Rast3d_is_xdr_null_float((float *)src);

	    if (nNullBits) {
		*nullBits |= ((unsigned char)isNull << nNullBits++);
		if (nNullBits == 8) {
		    nullBits++;
		    nNullBits = 0;
		}
	    }
	    else {
		*nullBits = (unsigned char)isNull;
		nNullBits++;
	    }

	    if (isNull) {
		src += XDR_FLOAT_LENGTH;
		continue;
	    }
	}

	/* printf ("write src cp0 %d %d (%d %d) %d\n", *src, *cp0, src, cp0, nullBits); */

	*cp0++ = *src++;
	if (gt8)
	    *cp3++ = *src++;
	if (gt16)
	    *cp2++ = *src++;

	if (nBits && precision) {
	    *cp1 |= (unsigned char)((unsigned char)(*src & mask) >> nBits);

	    /*printf ("%d\n", ((*src & mask) >> nBits) << nBits); */

	    if (8 - nBits < precision) {
		cp1++;

		/*printf ("%d %d\n", *cp1, (*src & mask) << (8 - nBits)); */

		*cp1 =
		    (unsigned char)((unsigned char)((*src & mask)) <<
				    (8 - nBits));
		nBits += precision - 8;
	    }
	    else {
		nBits = (nBits + precision) % 8;
		if (!nBits)
		    cp1++;
	    }
	}
	else {
	    *cp1 = (unsigned char)(*src & mask);
	    /* printf ("%d %d %d\n", *cp1, *src, nBits); */
	    nBits = (nBits + precision) % 8;
	    if (!nBits)
		cp1++;
	}

	src += srcIncrement;
    }

    *length = 1;		/* null-bit-vector indicator-byte */

    if (nofNull)		/* length of null-bit-vector */
	*length += size / 8 + ((size % 8) != 0);

    /* length of data */
    *length += (gt8 + gt16 + (precision == 0) + 1) * (size - nofNull) +
	((precision * (size - nofNull)) / 8) +
	(((precision * (size - nofNull)) % 8) != 0);

    *offsetMantissa = size - nofNull;
}

/*--------------------------------------------------------------------------*/

static void
G_fpcompress_rearrangeEncodeDoubles(unsigned char *src, int size,
				    int precision, unsigned char *dst,
				    int *length, int *offsetMantissa)
{
    unsigned int nNullBits, nBits;
    unsigned char isNull;
    register unsigned char *srcStop;
    register unsigned char *cp0, *cp1, *cp2, *cp3;
    register unsigned char *cp4, *cp5, *cp6, *cp7, *nullBits;
    unsigned char mask;
    int gt8, gt16, gt24, gt32, gt40, gt48, srcIncrement, nofNull;
    double *d;

    srcStop = src + size * XDR_DOUBLE_LENGTH;

    if ((precision >= 52) || (precision == -1)) {
	cp7 = dst;
	cp6 = cp7 + size;
	cp5 = cp6 + size;
	cp4 = cp5 + size;
	cp3 = cp4 + size;
	cp2 = cp3 + size;
	cp1 = cp2 + size;
	cp0 = cp1 + size;

	while (srcStop != src) {
	    *cp7++ = *src++;	/* sign + 7 ms exponent bits */
	    *cp6++ = *src++;	/* 4 exponent bits + 4 ms mantissa bits */
	    *cp5++ = *src++;	/* 8 mantissa bits */
	    *cp4++ = *src++;
	    *cp3++ = *src++;
	    *cp2++ = *src++;
	    *cp1++ = *src++;
	    *cp0++ = *src++;	/* 8 ls mantissa bits */
	}

	*length = size * XDR_DOUBLE_LENGTH;
	*offsetMantissa = size;

	return;
    }

    precision += 4;		/* treat the 4 ls exponent bits like addl mantissa bits */

    d = (double *)src;
    nofNull = 0;
    while (srcStop != (unsigned char *)d)
	nofNull += Rast3d_is_xdr_null_double(d++);

    if (nofNull == size) {
	*dst = (unsigned char)ALL_NULL_CODE;

	*length = 1;
	*offsetMantissa = 1;

	return;
    }

    *dst = (unsigned char)(nofNull == 0 ? ZERO_NULL_CODE : SOME_NULL_CODE);

    gt48 = precision > 48;
    gt40 = precision > 40;
    gt32 = precision > 32;
    gt24 = precision > 24;
    gt16 = precision > 16;
    gt8 = precision > 8;
    srcIncrement =
	1 + (!gt8) + (!gt16) + (!gt24) + (!gt32) + (!gt40) + (!gt48);

    precision %= 8;

    nullBits = dst + 1;
    if (nofNull)
	cp0 = nullBits + size / 8 + ((size % 8) != 0);
    else
	cp0 = nullBits;
    cp7 = cp0 + size - nofNull;
    cp6 = cp7 + size - nofNull;
    cp5 = cp6 + size - nofNull;
    cp4 = cp5 + size - nofNull;
    cp3 = cp4 + size - nofNull;
    cp2 = cp3 + size - nofNull;
    cp1 = cp7 + (gt8 + gt16 + gt24 + gt32 + gt40 + gt48) * (size - nofNull);

    mask = clearMask[precision];
    nBits = nNullBits = 0;

    while (srcStop != src) {
	if (nofNull) {
	    isNull = Rast3d_is_xdr_null_double((double *)src);

	    if (nNullBits) {
		*nullBits |= ((unsigned char)isNull << nNullBits++);
		if (nNullBits == 8) {
		    nullBits++;
		    nNullBits = 0;
		}
	    }
	    else {
		*nullBits = (unsigned char)isNull;
		nNullBits++;
	    }

	    if (isNull) {
		src += XDR_DOUBLE_LENGTH;
		continue;
	    }
	}

	*cp0++ = *src++;
	if (gt32) {
	    *cp7++ = *src++;
	    *cp6++ = *src++;
	    *cp5++ = *src++;

	    if (gt32)
		*cp4++ = *src++;
	    if (gt40)
		*cp3++ = *src++;
	    if (gt48)
		*cp2++ = *src++;
	}
	else {
	    if (gt8)
		*cp7++ = *src++;
	    if (gt16)
		*cp6++ = *src++;
	    if (gt24)
		*cp5++ = *src++;
	}

	if (nBits && precision) {
	    *cp1 |= (unsigned char)((unsigned char)(*src & mask) >> nBits);
	    if (8 - nBits < precision) {
		cp1++;
		*cp1 =
		    (unsigned char)(((unsigned char)(*src & mask)) <<
				    (8 - nBits));
		nBits += precision - 8;
	    }
	    else {
		nBits = (nBits + precision) % 8;
		if (!nBits)
		    cp1++;
	    }
	}
	else {
	    *cp1 = (unsigned char)(*src & mask);
	    nBits = (nBits + precision) % 8;
	    if (!nBits)
		cp1++;
	}

	src += srcIncrement;
    }

    *length = 1;

    if (nofNull)
	*length += size / 8 + ((size % 8) != 0);

    *length +=
	(1 + gt8 + gt16 + gt24 + gt32 + gt40 + gt48 +
	 (precision ==
	  0)) * (size - nofNull) + ((precision * (size - nofNull)) / 8) +
	(((precision * (size - nofNull)) % 8) != 0);

    if (gt8)
	*offsetMantissa = 2 * (size - nofNull);
    else
	*offsetMantissa = *length;
}

/*--------------------------------------------------------------------------*/

static void
G_fpcompress_rearrangeDecodeFloats(unsigned char *src, int size,
				   int precision, unsigned char *dst)
{
    unsigned int nNullBits, nBits;
    register unsigned char *dstStop;
    register unsigned char *cp0, *cp1, *cp2, *cp3, *nullBits;
    unsigned char mask, isNull;
    int gt8, gt16, dstIncrement, nofNull;
    float *f, *fStop;

    if ((precision != -1) && (precision <= 15)) {	/* 23 - 8 */
	cp3 = dst + 3;
	dstStop = dst + XDR_FLOAT_LENGTH * size + 3;
	while (dstStop != cp3) {
	    *cp3 = 0;
	    cp3 += XDR_FLOAT_LENGTH;
	}

	if (precision <= 7) {
	    cp3 = dst + 2;
	    dstStop = dst + XDR_FLOAT_LENGTH * size + 2;
	    while (dstStop != cp3) {
		*cp3 = 0;
		cp3 += XDR_FLOAT_LENGTH;
	    }
	}
    }

    dstStop = dst + size * XDR_FLOAT_LENGTH;

    if ((precision >= 23) || (precision == -1)) {
	cp3 = src;
	cp2 = cp3 + size;
	cp1 = cp2 + size;
	cp0 = cp1 + size;
	while (dstStop != dst) {
	    *dst++ = *cp3++;
	    *dst++ = *cp2++;
	    *dst++ = *cp1++;
	    *dst++ = *cp0++;
	}

	return;
    }

    if (*src == (unsigned char)ALL_NULL_CODE) {
	f = (float *)dst;
	while (dstStop != (unsigned char *)f)
	    Rast3d_set_xdr_null_float(f++);

	return;
    }

    precision += 1;		/* treat the ls exponent bit like an addl mantissa bit */

    gt16 = precision > 16;
    gt8 = precision > 8;
    dstIncrement = 1 + (!gt8) + (!gt16);

    precision %= 8;

    nofNull = 0;
    nullBits = src + 1;
    nNullBits = 0;
    if (*src == (unsigned char)SOME_NULL_CODE) {
	f = (float *)src;
	fStop = (float *)(src + size * XDR_FLOAT_LENGTH);
	while (fStop != f++) {
	    nofNull += ((*nullBits & ((unsigned char)1 << nNullBits++)) != 0);
	    if (nNullBits == 8) {
		nullBits++;
		nNullBits = 0;
	    }
	}
    }

    nullBits = src + 1;
    if (nofNull)
	cp0 = nullBits + size / 8 + ((size % 8) != 0);
    else
	cp0 = nullBits;
    cp3 = cp0 + size - nofNull;
    cp2 = cp3 + size - nofNull;
    cp1 = cp3 + (gt8 + gt16) * (size - nofNull);

    mask = clearMask[precision];
    nBits = nNullBits = 0;

    while (dstStop != dst) {
	if (nofNull) {
	    isNull = *nullBits & ((unsigned char)1 << nNullBits++);

	    if (nNullBits == 8) {
		nullBits++;
		nNullBits = 0;
	    }

	    if (isNull) {
		Rast3d_set_xdr_null_float((float *)dst);
		dst += XDR_FLOAT_LENGTH;
		continue;
	    }
	}

	*dst++ = *cp0++;
	if (gt8)
	    *dst++ = *cp3++;
	if (gt16)
	    *dst++ = *cp2++;

	if (nBits && precision) {
	    *dst = (unsigned char)((*cp1 << nBits) & mask);

	    if (8 - nBits < precision) {
		cp1++;
		*dst |= (unsigned char)((*cp1 >> (8 - nBits)) & mask);
		nBits += precision - 8;
	    }
	    else {
		nBits = (nBits + precision) % 8;
		if (!nBits)
		    cp1++;
	    }
	}
	else {
	    *dst = (unsigned char)(*cp1 & mask);
	    nBits = (nBits + precision) % 8;
	    if (!nBits)
		cp1++;
	}

	dst += dstIncrement;
    }
}

/*--------------------------------------------------------------------------*/

static void
G_fpcompress_rearrangeDecodeDoubles(unsigned char *src, int size,
				    int precision, unsigned char *dst)
{
    unsigned int nNullBits, nBits;
    register unsigned char *dstStop;
    register unsigned char *cp0, *cp1, *cp2, *cp3;
    register unsigned char *cp4, *cp5, *cp6, *cp7, *nullBits;
    unsigned char mask, isNull;
    int gt8, gt16, gt24, gt32, gt40, gt48, dstIncrement, offs, nofNull;
    double *d, *dStop;

    if ((precision != -1) && (precision <= 44)) {
	for (offs = 7; offs >= (precision + 19) / 8; offs--) {
	    cp7 = dst + offs;
	    dstStop = dst + XDR_DOUBLE_LENGTH * size + offs;
	    while (dstStop != cp7) {
		*cp7 = 0;
		cp7 += XDR_DOUBLE_LENGTH;
	    }
	}
    }

    dstStop = dst + size * XDR_DOUBLE_LENGTH;

    if ((precision >= 52) || (precision == -1)) {
	cp7 = src;
	cp6 = cp7 + size;
	cp5 = cp6 + size;
	cp4 = cp5 + size;
	cp3 = cp4 + size;
	cp2 = cp3 + size;
	cp1 = cp2 + size;
	cp0 = cp1 + size;

	while (dstStop != dst) {
	    *dst++ = *cp7++;
	    *dst++ = *cp6++;
	    *dst++ = *cp5++;
	    *dst++ = *cp4++;
	    *dst++ = *cp3++;
	    *dst++ = *cp2++;
	    *dst++ = *cp1++;
	    *dst++ = *cp0++;
	}

	return;
    }

    if (*src == (unsigned char)ALL_NULL_CODE) {
	/*printf ("all null\n"); */
	d = (double *)dst;
	while (dstStop != (unsigned char *)d)
	    Rast3d_set_xdr_null_double(d++);

	return;
    }

    precision += 4;		/* treat the 4 ls exponent bits like addl mantissa bits */

    gt48 = precision > 48;
    gt40 = precision > 40;
    gt32 = precision > 32;
    gt24 = precision > 24;
    gt16 = precision > 16;
    gt8 = precision > 8;

    dstIncrement =
	1 + (!gt8) + (!gt16) + (!gt24) + (!gt32) + (!gt40) + (!gt48);

    precision %= 8;

    nofNull = 0;
    nullBits = src + 1;
    nNullBits = 0;
    if (*src == (unsigned char)SOME_NULL_CODE) {
	d = (double *)src;
	dStop = (double *)(src + size * XDR_DOUBLE_LENGTH);
	while (dStop != d++) {
	    nofNull += ((*nullBits & ((unsigned char)1 << nNullBits++)) != 0);
	    if (nNullBits == 8) {
		nullBits++;
		nNullBits = 0;
	    }
	}
    }

    nullBits = src + 1;
    if (nofNull)
	cp0 = nullBits + size / 8 + ((size % 8) != 0);
    else
	cp0 = nullBits;
    cp7 = cp0 + size - nofNull;
    cp6 = cp7 + size - nofNull;
    cp5 = cp6 + size - nofNull;
    cp4 = cp5 + size - nofNull;
    cp3 = cp4 + size - nofNull;
    cp2 = cp3 + size - nofNull;
    cp1 = cp7 + (gt8 + gt16 + gt24 + gt32 + gt40 + gt48) * (size - nofNull);

    mask = clearMask[precision];
    nBits = nNullBits = 0;

    while (dstStop != dst) {
	if (nofNull) {
	    isNull = *nullBits & ((unsigned char)1 << nNullBits++);

	    if (nNullBits == 8) {
		nullBits++;
		nNullBits = 0;
	    }

	    if (isNull) {
		Rast3d_set_xdr_null_double((double *)dst);
		dst += XDR_DOUBLE_LENGTH;
		continue;
	    }
	}

	*dst++ = *cp0++;
	if (gt32) {
	    *dst++ = *cp7++;
	    *dst++ = *cp6++;
	    *dst++ = *cp5++;

	    if (gt32)
		*dst++ = *cp4++;
	    if (gt40)
		*dst++ = *cp3++;
	    if (gt48)
		*dst++ = *cp2++;
	}
	else {
	    if (gt8)
		*dst++ = *cp7++;
	    if (gt16)
		*dst++ = *cp6++;
	    if (gt24)
		*dst++ = *cp5++;
	}

	if (nBits && precision) {
	    *dst = (unsigned char)((*cp1 << nBits) & mask);

	    if (8 - nBits < precision) {
		cp1++;
		*dst |= (unsigned char)((*cp1 >> (8 - nBits)) & mask);
		nBits += precision - 8;
	    }
	    else {
		nBits = (nBits + precision) % 8;
		if (!nBits)
		    cp1++;
	    }
	}
	else {
	    *dst = (unsigned char)(*cp1 & mask);
	    nBits = (nBits + precision) % 8;
	    if (!nBits)
		cp1++;
	}

	dst += dstIncrement;
    }
}

/*--------------------------------------------------------------------------*/

int
Rast3d_fpcompress_write_xdr_nums(int fd, char *src, int nofNum, int precision,
			  char *compressBuf, int isFloat)
{
    int status;
    int nBytes;
    int offsetMantissa;

    if (isFloat)
	G_fpcompress_rearrangeEncodeFloats((unsigned char *)src, nofNum, precision,
			(unsigned char *)(compressBuf + 1),
					   &nBytes, &offsetMantissa);
    else
	G_fpcompress_rearrangeEncodeDoubles((unsigned char *)src, nofNum, precision,
				(unsigned char *)(compressBuf + 1),
					    &nBytes, &offsetMantissa);

	*compressBuf = 0;
	status = G_zlib_write(fd, (unsigned char *)compressBuf, nBytes + 1);

    if (status < 0) {
	Rast3d_error("Rast3d_fpcompress_write_xdr_nums: write error");
	return 0;
    }

    return 1;
}

/*--------------------------------------------------------------------------*/

int
Rast3d_fpcompress_read_xdr_nums(int fd, char *dst, int nofNum, int fileBytes,
			 int precision, char *compressBuf, int isFloat)
{
    int status;
    int lengthEncode, lengthDecode;
    int nBytes;
    char *src, *dest, *srcStop;
    nBytes = (isFloat ? XDR_FLOAT_LENGTH : XDR_DOUBLE_LENGTH);

    status = G_zlib_read(fd, fileBytes, (unsigned char *)compressBuf,
			 nofNum * nBytes + 1);

    if (status < 0) {
	Rast3d_error("Rast3d_fpcompress_read_xdr_nums: read error");
	return 0;
    }

    /* This code is kept for backward compatibility */
    if (*compressBuf++ == 1) {
	status--;
	Rast3d_rle_decode(compressBuf, dst, nofNum * nBytes, 1,
		     &lengthEncode, &lengthDecode);
	if (*dst == ALL_NULL_CODE)
	    Rast3d_fatal_error("Rast3d_fpcompress_read_xdr_nums: wrong code");

	if (status == nofNum * nBytes)
	    status -= lengthDecode - lengthEncode;

	src = compressBuf + status - 1;
	srcStop = compressBuf + lengthEncode - 1;
	dest = compressBuf + (status - lengthEncode) + lengthDecode - 1;
	while (src != srcStop)
	    *dest-- = *src--;

	src = dst;
	srcStop = src + lengthDecode;
	dest = compressBuf;
	while (src != srcStop)
	    *dest++ = *src++;
    }

    if (isFloat)
	G_fpcompress_rearrangeDecodeFloats((unsigned char *)compressBuf, nofNum, precision,
			(unsigned char *)dst);
    else
	G_fpcompress_rearrangeDecodeDoubles((unsigned char *)compressBuf, nofNum, precision,
			(unsigned char *)dst);

    return 1;
}
