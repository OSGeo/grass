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
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>

extern void port_init(void);

extern int nat_dbl;
extern int nat_flt;
extern int nat_lng;
extern int nat_int;
extern int nat_shrt;

extern int dbl_order;
extern int flt_order;
extern int lng_order;
extern int int_order;
extern int shrt_order;

extern unsigned char dbl_cnvrt[sizeof(double)];
extern unsigned char flt_cnvrt[sizeof(float)];
extern unsigned char lng_cnvrt[sizeof(long)];
extern unsigned char int_cnvrt[sizeof(int)];
extern unsigned char shrt_cnvrt[sizeof(short)];

struct Port_info *Cur_Head;

static char *buffer = NULL;
static int buf_alloced = 0;

static int buf_alloc(int needed)
{
    char *p;
    int cnt;

    if (needed <= buf_alloced)
	return (0);
    cnt = buf_alloced;
    p = dig__alloc_space(needed, &cnt, 100, buffer, 1);
    if (p == NULL)
	return (dig_out_of_memory());
    buffer = p;
    buf_alloced = cnt;
    return (0);
}

/***************************** READ ************************************/
/*  These are the routines to read from the Portable Vector Format.
   These routines must handle any type size conversions between the
   portable format and the native machine.

   Return:  0 error
   1 OK

 */


/* read doubles from the PVF file */
int dig__fread_port_D(double *buf, int cnt, GVFILE * fp)
{
    int i, j, ret;
    unsigned char *c1, *c2;

    if (Cur_Head->dbl_quick) {
	ret = dig_fread(buf, PORT_DOUBLE, cnt, fp);
	if (ret != cnt)
	    return 0;
    }
    else {
	/* read into buffer */
	buf_alloc(cnt * PORT_DOUBLE);
	ret = dig_fread(buffer, PORT_DOUBLE, cnt, fp);
	if (ret != cnt)
	    return 0;
	/* read from buffer in changed order */
	c1 = (unsigned char *)buffer;
	c2 = (unsigned char *)buf;
	for (i = 0; i < cnt; i++) {
	    for (j = 0; j < PORT_DOUBLE; j++) {
		c2[Cur_Head->dbl_cnvrt[j]] = c1[j];
	    }
	    c1 += PORT_DOUBLE;
	    c2 += sizeof(double);
	}
    }
    return 1;
}

/* read floats from the PVF file */
int dig__fread_port_F(float *buf, int cnt, GVFILE * fp)
{
    int i, j, ret;
    unsigned char *c1, *c2;

    if (Cur_Head->flt_quick) {
	ret = dig_fread(buf, PORT_FLOAT, cnt, fp);
	if (ret != cnt)
	    return 0;
    }
    else {
	/* read into buffer */
	buf_alloc(cnt * PORT_FLOAT);
	ret = dig_fread(buffer, PORT_FLOAT, cnt, fp);
	if (ret != cnt)
	    return 0;
	/* read from buffer in changed order */
	c1 = (unsigned char *)buffer;
	c2 = (unsigned char *)buf;
	for (i = 0; i < cnt; i++) {
	    for (j = 0; j < PORT_FLOAT; j++) {
		c2[Cur_Head->flt_cnvrt[j]] = c1[j];
	    }
	    c1 += PORT_FLOAT;
	    c2 += sizeof(float);
	}
    }
    return 1;
}

/* read longs from the PVF file */
int dig__fread_port_L(long *buf, int cnt, GVFILE * fp)
{
    int i, j, ret;
    unsigned char *c1, *c2;

    if (Cur_Head->lng_quick) {
	if (nat_lng == PORT_LONG) {
	    ret = dig_fread(buf, PORT_LONG, cnt, fp);
	    if (ret != cnt)
		return 0;
	}
	else {
	    /* read into buffer */
	    buf_alloc(cnt * PORT_LONG);
	    ret = dig_fread(buffer, PORT_LONG, cnt, fp);
	    if (ret != cnt)
		return 0;
	    /* set buffer to zero (positive numbers) */
	    memset(buf, 0, cnt * sizeof(long));
	    /* read from buffer in changed order */
	    c1 = (unsigned char *)buffer;
	    if (lng_order == ENDIAN_LITTLE)
		c2 = (unsigned char *)buf;
	    else
		c2 = (unsigned char *)buf + nat_lng - PORT_LONG;
	    for (i = 0; i < cnt; i++) {
		/* set to FF if the value is negative */
		if (lng_order == ENDIAN_LITTLE) {
		    if (c1[PORT_LONG - 1] & 0x80)
			memset(c2, 0xff, sizeof(long));
		}
		else {
		    if (c1[0] & 0x80)
			memset(c2, 0xff, sizeof(long));
		}
		memcpy(c2, c1, PORT_LONG);
		c1 += PORT_LONG;
		c2 += sizeof(long);
	    }
	}
    }
    else {
	/* read into buffer */
	buf_alloc(cnt * PORT_LONG);
	ret = dig_fread(buffer, PORT_LONG, cnt, fp);
	if (ret != cnt)
	    return 0;
	/* set buffer to zero (positive numbers) */
	memset(buf, 0, cnt * sizeof(long));
	/* read from buffer in changed order */
	c1 = (unsigned char *)buffer;
	c2 = (unsigned char *)buf;
	for (i = 0; i < cnt; i++) {
	    /* set to FF if the value is negative */
	    if (Cur_Head->byte_order == ENDIAN_LITTLE) {
		if (c1[PORT_LONG - 1] & 0x80)
		    memset(c2, 0xff, sizeof(long));
	    }
	    else {
		if (c1[0] & 0x80)
		    memset(c2, 0xff, sizeof(long));
	    }
	    for (j = 0; j < PORT_LONG; j++)
		c2[Cur_Head->lng_cnvrt[j]] = c1[j];
	    c1 += PORT_LONG;
	    c2 += sizeof(long);
	}
    }
    return 1;
}

/* read ints from the PVF file */
int dig__fread_port_I(int *buf, int cnt, GVFILE * fp)
{
    int i, j, ret;
    unsigned char *c1, *c2;

    if (Cur_Head->int_quick) {
	if (nat_int == PORT_INT) {
	    ret = dig_fread(buf, PORT_INT, cnt, fp);
	    if (ret != cnt)
		return 0;
	}
	else {
	    /* read into buffer */
	    buf_alloc(cnt * PORT_INT);
	    ret = dig_fread(buffer, PORT_INT, cnt, fp);
	    if (ret != cnt)
		return 0;
	    /* set buffer to zero (positive numbers) */
	    memset(buf, 0, cnt * sizeof(int));
	    /* read from buffer in changed order */
	    c1 = (unsigned char *)buffer;
	    if (int_order == ENDIAN_LITTLE)
		c2 = (unsigned char *)buf;
	    else
		c2 = (unsigned char *)buf + nat_int - PORT_INT;
	    for (i = 0; i < cnt; i++) {
		/* set to FF if the value is negative */
		if (int_order == ENDIAN_LITTLE) {
		    if (c1[PORT_INT - 1] & 0x80)
			memset(c2, 0xff, sizeof(int));
		}
		else {
		    if (c1[0] & 0x80)
			memset(c2, 0xff, sizeof(int));
		}
		memcpy(c2, c1, PORT_INT);
		c1 += PORT_INT;
		c2 += sizeof(int);
	    }
	}
    }
    else {
	/* read into buffer */
	buf_alloc(cnt * PORT_INT);
	ret = dig_fread(buffer, PORT_INT, cnt, fp);
	if (ret != cnt)
	    return 0;
	/* set buffer to zero (positive numbers) */
	memset(buf, 0, cnt * sizeof(int));
	/* read from buffer in changed order */
	c1 = (unsigned char *)buffer;
	c2 = (unsigned char *)buf;
	for (i = 0; i < cnt; i++) {
	    /* set to FF if the value is negative */
	    if (Cur_Head->byte_order == ENDIAN_LITTLE) {
		if (c1[PORT_INT - 1] & 0x80)
		    memset(c2, 0xff, sizeof(int));
	    }
	    else {
		if (c1[0] & 0x80)
		    memset(c2, 0xff, sizeof(int));
	    }
	    for (j = 0; j < PORT_INT; j++)
		c2[Cur_Head->int_cnvrt[j]] = c1[j];
	    c1 += PORT_INT;
	    c2 += sizeof(int);
	}
    }
    return 1;
}

/* read shorts from the PVF file */
int dig__fread_port_S(short *buf, int cnt, GVFILE * fp)
{
    int i, j, ret;
    unsigned char *c1, *c2;

    if (Cur_Head->shrt_quick) {
	if (nat_shrt == PORT_SHORT) {
	    ret = dig_fread(buf, PORT_SHORT, cnt, fp);
	    if (ret != cnt)
		return 0;
	}
	else {
	    /* read into buffer */
	    buf_alloc(cnt * PORT_SHORT);
	    if (0 >= (ret = dig_fread(buffer, PORT_SHORT, cnt, fp)))
		if (ret != cnt)
		    return 0;
	    /* set buffer to zero (positive numbers) */
	    memset(buf, 0, cnt * sizeof(short));
	    /* read from buffer in changed order */
	    c1 = (unsigned char *)buffer;
	    if (shrt_order == ENDIAN_LITTLE)
		c2 = (unsigned char *)buf;
	    else
		c2 = (unsigned char *)buf + nat_shrt - PORT_SHORT;
	    for (i = 0; i < cnt; i++) {
		/* set to FF if the value is negative */
		if (shrt_order == ENDIAN_LITTLE) {
		    if (c1[PORT_SHORT - 1] & 0x80)
			memset(c2, 0xff, sizeof(short));
		}
		else {
		    if (c1[0] & 0x80)
			memset(c2, 0xff, sizeof(short));
		}
		memcpy(c2, c1, PORT_SHORT);
		c1 += PORT_SHORT;
		c2 += sizeof(short);
	    }
	}
    }
    else {
	/* read into buffer */
	buf_alloc(cnt * PORT_SHORT);
	ret = dig_fread(buffer, PORT_SHORT, cnt, fp);
	if (ret != cnt)
	    return 0;
	/* set buffer to zero (positive numbers) */
	memset(buf, 0, cnt * sizeof(short));
	/* read from buffer in changed order */
	c1 = (unsigned char *)buffer;
	c2 = (unsigned char *)buf;
	for (i = 0; i < cnt; i++) {
	    /* set to FF if the value is negative */
	    if (Cur_Head->byte_order == ENDIAN_LITTLE) {
		if (c1[PORT_SHORT - 1] & 0x80)
		    memset(c2, 0xff, sizeof(short));
	    }
	    else {
		if (c1[0] & 0x80)
		    memset(c2, 0xff, sizeof(short));
	    }
	    for (j = 0; j < PORT_SHORT; j++)
		c2[Cur_Head->shrt_cnvrt[j]] = c1[j];
	    c1 += PORT_SHORT;
	    c2 += sizeof(short);
	}
    }
    return 1;
}

/* read chars from the PVF file */
int dig__fread_port_C(char *buf, int cnt, GVFILE * fp)
{
    int ret;

    ret = dig_fread(buf, PORT_CHAR, cnt, fp);
    if (ret != cnt)
	return 0;
    return 1;
}

/* read plus_t from the PVF file */
/* plus_t is defined as int so we only retype pointer and use int function */
int dig__fread_port_P(plus_t * buf, int cnt, GVFILE * fp)
{
    int *ibuf;

    ibuf = (int *)buf;

    return (dig__fread_port_I(ibuf, cnt, fp));
}

/***************************** WRITE ************************************/

int dig__fwrite_port_D(double *buf,	/* DOUBLE */
		       int cnt, GVFILE * fp)
{
    int i, j;
    unsigned char *c1, *c2;

    if (Cur_Head->dbl_quick) {
	if (dig_fwrite(buf, PORT_DOUBLE, cnt, fp) == cnt)
	    return 1;
    }
    else {
	buf_alloc(cnt * PORT_DOUBLE);
	c1 = (unsigned char *)buf;
	c2 = (unsigned char *)buffer;
	for (i = 0; i < cnt; i++) {
	    for (j = 0; j < PORT_DOUBLE; j++)
		c2[j] = c1[Cur_Head->dbl_cnvrt[j]];
	    c1 += sizeof(double);
	    c2 += PORT_DOUBLE;
	}
	if (dig_fwrite(buffer, PORT_DOUBLE, cnt, fp) == cnt)
	    return 1;
    }
    return 0;
}

int dig__fwrite_port_F(float *buf,	/* FLOAT */
		       int cnt, GVFILE * fp)
{
    int i, j;
    unsigned char *c1, *c2;

    if (Cur_Head->flt_quick) {
	if (dig_fwrite(buf, PORT_FLOAT, cnt, fp) == cnt)
	    return 1;
    }
    else {
	buf_alloc(cnt * PORT_FLOAT);
	c1 = (unsigned char *)buf;
	c2 = (unsigned char *)buffer;
	for (i = 0; i < cnt; i++) {
	    for (j = 0; j < PORT_FLOAT; j++)
		c2[j] = c1[Cur_Head->flt_cnvrt[j]];
	    c1 += sizeof(float);
	    c2 += PORT_FLOAT;
	}
	if (dig_fwrite(buffer, PORT_FLOAT, cnt, fp) == cnt)
	    return 1;
    }
    return 0;
}

int dig__fwrite_port_L(long *buf,	/* LONG */
		       int cnt, GVFILE * fp)
{
    int i, j;
    unsigned char *c1, *c2;

    if (Cur_Head->lng_quick) {
	if (nat_lng == PORT_LONG) {
	    if (dig_fwrite(buf, PORT_LONG, cnt, fp) == cnt)
		return 1;
	}
	else {
	    buf_alloc(cnt * PORT_LONG);
	    if (lng_order == ENDIAN_LITTLE)
		c1 = (unsigned char *)buf;
	    else
		c1 = (unsigned char *)buf + nat_lng - PORT_LONG;
	    c2 = (unsigned char *)buffer;
	    for (i = 0; i < cnt; i++) {
		memcpy(c2, c1, PORT_LONG);
		c1 += PORT_LONG;
		c2 += sizeof(long);
	    }
	    if (dig_fwrite(buffer, PORT_LONG, cnt, fp) == cnt)
		return 1;
	}
    }
    else {
	buf_alloc(cnt * PORT_LONG);
	c1 = (unsigned char *)buf;
	c2 = (unsigned char *)buffer;
	for (i = 0; i < cnt; i++) {
	    for (j = 0; j < PORT_LONG; j++)
		c2[j] = c1[Cur_Head->lng_cnvrt[j]];
	    c1 += sizeof(long);
	    c2 += PORT_LONG;
	}
	if (dig_fwrite(buffer, PORT_LONG, cnt, fp) == cnt)
	    return 1;
    }
    return 0;
}

int dig__fwrite_port_I(int *buf,	/* INT */
		       int cnt, GVFILE * fp)
{
    int i, j;
    unsigned char *c1, *c2;

    if (Cur_Head->int_quick) {
	if (nat_int == PORT_INT) {
	    if (dig_fwrite(buf, PORT_INT, cnt, fp) == cnt)
		return 1;
	}
	else {
	    buf_alloc(cnt * PORT_INT);
	    if (int_order == ENDIAN_LITTLE)
		c1 = (unsigned char *)buf;
	    else
		c1 = (unsigned char *)buf + nat_int - PORT_INT;
	    c2 = (unsigned char *)buffer;
	    for (i = 0; i < cnt; i++) {
		memcpy(c2, c1, PORT_INT);
		c1 += PORT_INT;
		c2 += sizeof(int);
	    }
	    if (dig_fwrite(buffer, PORT_INT, cnt, fp) == cnt)
		return 1;
	}
    }
    else {
	buf_alloc(cnt * PORT_INT);
	c1 = (unsigned char *)buf;
	c2 = (unsigned char *)buffer;
	for (i = 0; i < cnt; i++) {
	    for (j = 0; j < PORT_INT; j++)
		c2[j] = c1[Cur_Head->int_cnvrt[j]];
	    c1 += sizeof(int);
	    c2 += PORT_INT;
	}
	if (dig_fwrite(buffer, PORT_INT, cnt, fp) == cnt)
	    return 1;
    }
    return 0;
}

int dig__fwrite_port_S(short *buf,	/* SHORT */
		       int cnt, GVFILE * fp)
{
    int i, j;
    unsigned char *c1, *c2;

    if (Cur_Head->shrt_quick) {
	if (nat_shrt == PORT_SHORT) {
	    if (dig_fwrite(buf, PORT_SHORT, cnt, fp) == cnt)
		return 1;
	}
	else {
	    buf_alloc(cnt * PORT_SHORT);
	    if (shrt_order == ENDIAN_LITTLE)
		c1 = (unsigned char *)buf;
	    else
		c1 = (unsigned char *)buf + nat_shrt - PORT_SHORT;
	    c2 = (unsigned char *)buffer;
	    for (i = 0; i < cnt; i++) {
		memcpy(c2, c1, PORT_SHORT);
		c1 += PORT_SHORT;
		c2 += sizeof(short);
	    }
	    if (dig_fwrite(buffer, PORT_SHORT, cnt, fp) == cnt)
		return 1;
	}
    }
    else {
	buf_alloc(cnt * PORT_SHORT);
	c1 = (unsigned char *)buf;
	c2 = (unsigned char *)buffer;
	for (i = 0; i < cnt; i++) {
	    for (j = 0; j < PORT_SHORT; j++)
		c2[j] = c1[Cur_Head->shrt_cnvrt[j]];
	    c1 += sizeof(short);
	    c2 += PORT_SHORT;
	}
	if (dig_fwrite(buffer, PORT_SHORT, cnt, fp) == cnt)
	    return 1;
    }
    return 0;
}

/* plus_t is defined as int so we only retype pointer and use int function */
int dig__fwrite_port_P(plus_t * buf,	/* PLUS_T->INT */
		       int cnt, GVFILE * fp)
{
    return (dig__fwrite_port_I((int *)buf, cnt, fp));
}

int dig__fwrite_port_C(char *buf,	/* CHAR */
		       int cnt, GVFILE * fp)
{
    if (dig_fwrite(buf, PORT_CHAR, cnt, fp) == cnt)
	return 1;

    return 0;
}

/* set portable info structure to byte order of file */
void dig_init_portable(struct Port_info *port, int byte_order)
{
    int i;

    port_init();

    port->byte_order = byte_order;

    if (port->byte_order == dbl_order)
	port->dbl_quick = TRUE;
    else
	port->dbl_quick = FALSE;

    for (i = 0; i < PORT_DOUBLE; i++) {
	if (port->byte_order == ENDIAN_BIG)
	    port->dbl_cnvrt[i] = dbl_cnvrt[i];
	else
	    port->dbl_cnvrt[i] = dbl_cnvrt[PORT_DOUBLE - i - 1];
    }

    if (port->byte_order == flt_order)
	port->flt_quick = TRUE;
    else
	port->flt_quick = FALSE;

    for (i = 0; i < PORT_FLOAT; i++) {
	if (port->byte_order == ENDIAN_BIG)
	    port->flt_cnvrt[i] = flt_cnvrt[i];
	else
	    port->flt_cnvrt[i] = flt_cnvrt[PORT_FLOAT - i - 1];
    }

    if (port->byte_order == lng_order)
	port->lng_quick = TRUE;
    else
	port->lng_quick = FALSE;

    for (i = 0; i < PORT_LONG; i++) {
	if (port->byte_order == ENDIAN_BIG)
	    port->lng_cnvrt[i] = lng_cnvrt[i];
	else
	    port->lng_cnvrt[i] = lng_cnvrt[PORT_LONG - i - 1];
    }

    if (port->byte_order == int_order)
	port->int_quick = TRUE;
    else
	port->int_quick = FALSE;

    for (i = 0; i < PORT_INT; i++) {
	if (port->byte_order == ENDIAN_BIG)
	    port->int_cnvrt[i] = int_cnvrt[i];
	else
	    port->int_cnvrt[i] = int_cnvrt[PORT_INT - i - 1];
    }

    if (port->byte_order == shrt_order)
	port->shrt_quick = TRUE;
    else
	port->shrt_quick = FALSE;

    for (i = 0; i < PORT_SHORT; i++) {
	if (port->byte_order == ENDIAN_BIG)
	    port->shrt_cnvrt[i] = shrt_cnvrt[i];
	else
	    port->shrt_cnvrt[i] = shrt_cnvrt[PORT_SHORT - i - 1];
    }

    return;
}

/* set current portable info */
int dig_set_cur_port(struct Port_info *port)
{
    Cur_Head = port;
    return 0;
}

int dig__byte_order_out()
{
    if (dbl_order == ENDIAN_LITTLE)
	return (ENDIAN_LITTLE);
    else
	return (ENDIAN_BIG);
}
