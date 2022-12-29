/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */
#include <string.h>


static int err_code = 0;
static char err_msg[1024];


/*!
 * \brief 
 *
 * record 'code' and 'msg' as
 * error code/msg (in static variables)
 * code==0 will clear the error (ie set msg=NULL)
 * returns 'code' so that it can be used like:
 \code
 return datetime_error (-1, "bad date");
 \endcode
 *
 *  \param code
 *  \param msg
 *  \return int
 */

int datetime_error(int code, char *msg)
{
    err_code = code;
    *err_msg = 0;
    if (code != 0 && msg)
	strcpy(err_msg, msg);	/* hope err_msg is big enough */

    return code;
}

/*!
 * \brief 
 *
 * returns an error code
 *
 *  \return int
 */

int datetime_error_code(void)
{
    return err_code;
}

/*!
 * \brief 
 *
 * returns an error message
 *
 *  \return char *
 */

char *datetime_error_msg(void)
{
    return err_msg;
}


/*!
 * \brief 
 *
 *  clears error code and message
 *
 *  \return void
 */

void datetime_clear_error(void)
{
    err_code = 0;
    *err_msg = 0;
}
