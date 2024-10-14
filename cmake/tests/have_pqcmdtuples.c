#include <libpq-fe.h>
int main()
{
    PGresult *res = NULL;
    PGconn *conn = PQconnectdb(NULL);
    res = PQexec(conn, NULL);
    PQcmdTuples(res);
    return 0;
}
