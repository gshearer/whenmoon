#define WM_DBC
#include "db.h"

void
db_cleanup(void)
{
  if(global.dbconn != NULL)
  {
    logger(C_DB, INFO, "db_cleanup", "closing connection to %s:%d", global.conf.dbhost, global.conf.dbport);
    PQfinish(global.dbconn);
    global.dbconn = NULL;
  }
}

static int
db_connect(void)
{
  char psql_conn_info[1000];

  if(global.dbconn != NULL)
  {
    if(PQstatus(global.dbconn) == CONNECTION_OK)
    {
      global.dbtimer = global.now;
      return(SUCCESS);
    }

    logger(C_DB, INFO, "db_connect", "attempting to re-establish connection to %s:%d", global.conf.dbhost, global.conf.dbport);
    PQfinish(global.dbconn);
  }

  snprintf(psql_conn_info, 999, "host=%s port=%d user=%s password=%s dbname=%s connect_timeout=5", global.conf.dbhost, global.conf.dbport, global.conf.dbuser, global.conf.dbpass, global.conf.dbname);

  logger(C_DB, DEBUG3, "db_connect", "psql conn string = %s", psql_conn_info);

  global.dbconn = PQconnectdb(psql_conn_info);

  if(PQstatus(global.dbconn) != CONNECTION_OK)
  {
    logger(C_DB, CRIT, "db_connect", "connection to %s:%d failed: %s", global.conf.dbhost, global.conf.dbport, PQerrorMessage(global.dbconn));
    db_cleanup();
    return(FAIL);
  }

  logger(C_DB, INFO, "db_connect", "psql connnection to %s:%d established.", global.conf.dbhost, global.conf.dbport);
  global.dbtimer = global.now;
  return(SUCCESS);
}

PGresult *
db_query(char *sql)
{
  PGresult *res;

  if(db_connect() == FAIL)
    return(NULL);

  global.dbtimer = global.now;

  logger(C_DB, DEBUG4, "db_query", "submitting query: %s", sql);

  res = PQexec(global.dbconn, sql);

  if(PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    logger(C_DB, CRIT, "db_query", "query failed: %s", PQerrorMessage(global.dbconn));
    PQclear(res);
    return(NULL);
  }
  return(res);
}

void
db_hook(void)
{
  if(global.dbconn != NULL && global.now - global.dbtimer >= DB_IDLETIME)
    db_cleanup();
}

void
db_init(void)
{
  logger(C_ANY, INFO, "db_init", "postgres sql db api initialized");
}
