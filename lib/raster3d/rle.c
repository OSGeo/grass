#include <stdio.h>
#include <grass/raster3d.h>

#define G_254_SQUARE 64516
#define G_254_TIMES_2 508

#define G_RLE_OUTPUT_CODE(code) (*((unsigned char *) dst++) = (code))
#define G_RLE_INPUT_CODE(codeP) (*(codeP) = *((unsigned char *) src++))

/*---------------------------------------------------------------------------*/

static int G_rle_codeLength(int length)
{
    register int lPrime;
    int codeLength;

    if (length == -1)
	return 2;
    if (length < 254)
	return 1;
    if (length < G_254_TIMES_2)
	return 2;
    if (length < G_254_SQUARE)
	return 3;

    codeLength = 0;
    lPrime = length;
    while ((lPrime = lPrime / 254) != 0)
	codeLength++;
    return codeLength + 2;
}

/*---------------------------------------------------------------------------*/

static char *rle_length2code(int length, char *dst)
{
    register int lPrime;

    if (length == -1) {		/* stop code */
	G_RLE_OUTPUT_CODE(255);
	G_RLE_OUTPUT_CODE(255);

	return dst;
    }

    if (length < 254) {
	G_RLE_OUTPUT_CODE(length);

	return dst;
    }

    if (length < G_254_TIMES_2) {	/* length == 254 + a; a < 254 */
	G_RLE_OUTPUT_CODE(255);
	G_RLE_OUTPUT_CODE(length % 254);

	return dst;
    }

    if (length < G_254_SQUARE) {	/* length = 254 * b + a; b, a < 254 */
	G_RLE_OUTPUT_CODE(254);	/* this if-clause included for efficiency only */
	G_RLE_OUTPUT_CODE(length / 254);
	G_RLE_OUTPUT_CODE(length % 254);

	return dst;
    }

    /* TODO implement a corrected version for larger strings */
    /* This code is simply wrong, it works only for c == 2, critical number for wrong computation is 254*254*2 = 129032 */
    /* CORRECT: length = 254 ^ 2 + 254 * b + a; b, a < 254 */

    /* WRONG: length = 254 ^ c + 254 * b + a; b, a < 254 */

    lPrime = length;
    while ((lPrime = lPrime / 254) != 0)
	G_RLE_OUTPUT_CODE(254);

    length %= G_254_SQUARE;

    G_RLE_OUTPUT_CODE(length / 254);
    G_RLE_OUTPUT_CODE(length % 254);

    /* Next should be: length = 254 ^ 3 + 254 ^ 2 * c + 254 * b + a; c, b, a < 254 */

    return dst;
}

/*---------------------------------------------------------------------------*/

static char *rle_code2length(char *src, int *length)
{
    int code;

    if (G_RLE_INPUT_CODE(length) < 254)
	return src;		/* length < 254 */

    if (*length == 255) {	/* length == 254 + a; a < 254 */
	if (G_RLE_INPUT_CODE(length) == 255) {
	    *length = -1;
	    return src;
	}

	*length += 254;
	return src;
    }

    G_RLE_INPUT_CODE(&code);
    if (code < 254) {		/* length = 254 * b + a; b, a < 254 */
	G_RLE_INPUT_CODE(length);	/* this if-clause included for efficiency only */
	*length += 254 * code;

	return src;
    }

    /* TODO implement a corrected version for larger strings */
    /* This code is simply wrong, it works only for c == 2, critical number for wrong computation is 254*254*2 = 129032 */
    /* CORRECT: length = 254 ^ 2 + 254 * b + a; b, a < 254 */

    /* WRONG: length = 254 ^ c + 254 * b + a; b, a < 254 */

    *length = G_254_SQUARE;
    while (G_RLE_INPUT_CODE(&code) == 254)
	*length *= 254;

    *length += 254 * code;
    G_RLE_INPUT_CODE(&code);
    *length += code;

    /* Next should be: length = 254 ^ 3 + 254 ^ 2 * c + 254 * b + a; c, b, a < 254 */

    return src;
}

/*---------------------------------------------------------------------------*/

int Rast3d_rle_count_only(char *src, int nofElts, int eltLength)
{
    int length, nofEqual;
    char *head, *tail, *headStop, *headStop2;

    if (nofElts <= 0)
	Rast3d_fatal_error("trying to encode 0-length list");

    length = 0;
    nofEqual = 1;
    head = src + eltLength;
    tail = src;

    headStop = src + nofElts * eltLength;

    while (head != headStop) {
	headStop2 = head + eltLength;

	while (head != headStop2) {
	    if (*head != *tail) {
		length += G_rle_codeLength(nofEqual) + eltLength;
		nofEqual = 1;
		tail = headStop2 - eltLength;
		break;
	    }
	    head++;
	    tail++;
	}

	if (head == headStop2) {
	    nofEqual++;
	    continue;
	}

	head = headStop2;
    }
    length += G_rle_codeLength(nofEqual) + eltLength;

    return length + G_rle_codeLength(-1);
}

/*---------------------------------------------------------------------------*/

void Rast3d_rle_encode(char *src, char *dst, int nofElts, int eltLength)
{
    int length, nofEqual;
    char *head, *tail, *headStop, *headStop2;

    if (nofElts <= 0)
	Rast3d_fatal_error("trying to encode 0-length list");

    length = 0;
    nofEqual = 1;
    head = src + eltLength;
    tail = src;

    headStop = src + nofElts * eltLength;

    while (head != headStop) {
	headStop2 = head + eltLength;

	while (head != headStop2) {
	    if (*head != *tail) {
		dst = rle_length2code(nofEqual, dst);
		tail = headStop2 - eltLength * (nofEqual + 1);
		head = tail + eltLength;
		/*      printf ("equal %d char %d\n", nofEqual, *tail); */
		while (tail != head)
		    *dst++ = *tail++;
		length += G_rle_codeLength(nofEqual) + eltLength;
		nofEqual = 1;
		tail = headStop2 - eltLength;
		break;
	    }
	    head++;
	    tail++;
	}

	if (head == headStop2) {
	    nofEqual++;
	    continue;
	}

	head = headStop2;
    }

    dst = rle_length2code(nofEqual, dst);
    tail = headStop - eltLength * nofEqual;
    head = tail + eltLength;
    while (tail != head)
	*dst++ = *tail++;
    length += G_rle_codeLength(nofEqual) + eltLength;
    dst = rle_length2code(-1, dst);
    length += G_rle_codeLength(-1);
    rle_code2length(dst - 2, &nofEqual);
}

/*---------------------------------------------------------------------------*/

void
Rast3d_rle_decode(char *src, char *dst, int nofElts, int eltLength,
	     int *lengthEncode, int *lengthDecode)
{
    int nofEqual;
    char *src2, *srcStop, *src2Stop, *dstFirst;

    srcStop = src + nofElts * eltLength;
    dstFirst = dst;

    while (src != srcStop) {
	src = rle_code2length(src, &nofEqual);

	if (nofEqual == -1) {
	    *lengthEncode = src - (srcStop - nofElts * eltLength);
	    *lengthDecode = dst - dstFirst;
	    return;
	}

	while (nofEqual--) {
	    src2 = src;
	    src2Stop = src2 + eltLength;
	    while (src2 != src2Stop)
		*dst++ = *src2++;
	}
	src += eltLength;
    }

    Rast3d_fatal_error("Rast3d_rle_decode: string ends prematurely");
}

/*---------------------------------------------------------------------------*/

/* TODO: Find out if this function used at all.
 * Seems to be some leftover from the early pre-SVN days of GRASS GIS.
 * Maris, 2018.
 */
void test_rle()
{
    char c[100];
    int length;

    do {
	printf("length? ");
	if (scanf("%d", &length) != 1)
        Rast3d_fatal_error("Error reading length");
	printf("length = %d\n", length);
	printf("codeLength %d   ", G_rle_codeLength(length));
	(void)rle_length2code(length, c);
	length = 0;
	(void)rle_code2length(c, &length);
	printf("output length %d\n\n", length);
    } while (1);
}
