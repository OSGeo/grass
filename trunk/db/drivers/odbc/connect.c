#include <grass/dbmi.h>
#include "odbc.h"
#include "globals.h"
#include "proto.h"

int open_connection()
{
    SQLRETURN ret;

    /* Allocate Environment handle and register version */
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &ODenvi);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	db_d_append_error("SQLAllocHandle()");
	db_d_report_error();
	return DB_FAILED;
    }

    ret =
	SQLSetEnvAttr(ODenvi, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	db_d_append_error("SQLSetEnvAttr()");
	db_d_report_error();
	SQLFreeHandle(SQL_HANDLE_ENV, ODenvi);
	return DB_FAILED;
    }

    /* Allocate connection handle */
    ret = SQLAllocHandle(SQL_HANDLE_DBC, ODenvi, &ODconn);
    if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
	db_d_append_error("SQLAllocHandle()");
	db_d_report_error();
	SQLFreeHandle(SQL_HANDLE_ENV, ODenvi);
	return DB_FAILED;
    }

    /* Set timeout */
    SQLSetConnectAttr(ODconn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *) 5, 0);

    return DB_OK;
}

int close_connection()
{
    SQLFreeHandle(SQL_HANDLE_DBC, ODconn);
    SQLFreeHandle(SQL_HANDLE_ENV, ODenvi);
    return DB_OK;
}
