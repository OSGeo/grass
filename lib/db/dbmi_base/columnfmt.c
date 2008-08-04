#include <grass/dbmi.h>

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_convert_Cstring_to_column_value(const char *Cstring, dbColumn * column)
{
    dbValue *value;
    int sqltype;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_value(column);
    return db_convert_Cstring_to_value(Cstring, sqltype, value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int
db_convert_Cstring_to_column_default_value(const char *Cstring,
					   dbColumn * column)
{
    dbValue *value;
    int sqltype;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_default_value(column);
    return db_convert_Cstring_to_value(Cstring, sqltype, value);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int db_convert_column_value_to_string(dbColumn * column, dbString * string)
{
    int sqltype;
    dbValue *value;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_value(column);
    return db_convert_value_to_string(value, sqltype, string);
}

/*!
   \fn 
   \brief 
   \return 
   \param 
 */
int
db_convert_column_default_value_to_string(dbColumn * column,
					  dbString * string)
{
    int sqltype;
    dbValue *value;

    sqltype = db_get_column_sqltype(column);
    value = db_get_column_default_value(column);
    return db_convert_value_to_string(value, sqltype, string);
}
