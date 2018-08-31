#define DB_RETURN_ERR return(db_get_error_code());
#define DB_START_PROCEDURE_CALL(x) \
	{if(db__start_procedure_call(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_RETURN_CODE(x) \
	{if(db__recv_return_code(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_SUCCESS() \
	{if(db__send_success()!=DB_OK) DB_RETURN_ERR}
#define DB_SEND_FAILURE() \
	{if(db__send_failure()!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_STRING(x) \
	{if(db__send_string(x)!=DB_OK) DB_RETURN_ERR}
#define DB_SEND_STRING_ARRAY(x,n) \
	{if(db__send_string_array(x,n)!=DB_OK) DB_RETURN_ERR}
#define DB_SEND_C_STRING(x) \
	{if(db__send_Cstring(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_STRING(x) \
	{if(db__recv_string(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_STRING_ARRAY(x,n) \
	{if(db__recv_string_array(x,n)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_CHAR(x) \
	{if(db__send_char(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_CHAR(x) \
	{if(db__recv_char(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_SHORT(x) \
	{if(db__send_short(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_SHORT(x) \
	{if(db__recv_short(x)!=DB_OK) DB_RETURN_ERR}
#define DB_SEND_SHORT_ARRAY(x,n) \
	{if(db__send_short_array(x,n)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_SHORT_ARRAY(x,n) \
	{if(db__recv_short_array(x,n)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_INT(x) \
	{if(db__send_int(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_INT(x) \
	{if(db__recv_int(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_FLOAT(x) \
	{if(db__send_float(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_FLOAT(x) \
	{if(db__recv_float(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_DOUBLE(x) \
	{if(db__send_double(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_DOUBLE(x) \
	{if(db__recv_double(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_DATETIME(x) \
	{if(db__send_datetime(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_DATETIME(x) \
	{if(db__recv_datetime(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_HANDLE(x) \
	{if(db__send_handle(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_HANDLE(x) \
	{if(db__recv_handle(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_TABLE_DEFINITION(x) \
	{if(db__send_table_definition(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_TABLE_DEFINITION(x) \
	{if(db__recv_table_definition(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_TABLE_DATA(x) \
	{if(db__send_table_data(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_TABLE_DATA(x) \
	{if(db__recv_table_data(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_TABLE_PRIV(x) \
	{if(db__send_table_priv(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_TABLE_PRIV(x) \
	{if(db__recv_table_priv(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_COLUMN_PRIVS(x) \
	{if(db__send_column_privs(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_COLUMN_PRIVS(x) \
	{if(db__recv_column_privs(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_COLUMN_DEFINITION(x) \
	{if(db__send_column_definition(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_COLUMN_DEFINITION(x) \
	{if(db__recv_column_definition(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_COLUMN_VALUE(x) \
	{if(db__send_column_value(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_COLUMN_VALUE(x) \
	{if(db__recv_column_value(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_COLUMN_DEFAULT_VALUE(x) \
	{if(db__send_column_default_value(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_COLUMN_DEFAULT_VALUE(x) \
	{if(db__recv_column_default_value(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_TOKEN(x) \
	{if(db__send_token(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_TOKEN(x) \
	{if(db__recv_token(x)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_INDEX(x) \
	{if(db__send_index(x)!=DB_OK) DB_RETURN_ERR}
#define DB_SEND_INDEX_ARRAY(x,n) \
	{if(db__send_index_array(x,n)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_INDEX(x) \
	{if(db__recv_index(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_INDEX_ARRAY(x,n) \
	{if(db__recv_index_array(x,n)!=DB_OK) DB_RETURN_ERR}

#define DB_SEND_FK(x) \
	{if(db__send_fk(x)!=DB_OK) DB_RETURN_ERR}
#define DB_SEND_FK_ARRAY(x,n) \
	{if(db__send_fk_array(x,n)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_FK(x) \
	{if(db__recv_fk(x)!=DB_OK) DB_RETURN_ERR}
#define DB_RECV_FK_ARRAY(x,n) \
	{if(db__recv_fk_array(x,n)!=DB_OK) DB_RETURN_ERR}
