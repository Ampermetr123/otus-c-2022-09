#include <assert.h>
#include <libpq-fe.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONNECT_TIMEOUT "5"

void print_usage(const char *program_name) {
  printf("Вывод статистики по заданной колоке в таблице БД\n"
         "Запуск:\n%s <database_name>  <table_name>  <column_name>\n"
         "Параметры подключения к БД задаеются через параметры окружения:\n \
    DB_ADDR -  database server (default localhost)\n \
    DB_PORT -  database port (default 5432)\n \
    DB_USER -  user (default postgres)\n \
    DB_PASSWORD - password (default qwe123)\n",
         program_name);
}

typedef struct {
  const char *db_addr;
  const char *db_port;
  const char *db_user;
  const char *db_password;
  const char *db_name;
  const char *table_name;
  const char *column_name;
} Target;


static bool load_target(int argc, char *argv[], Target *t);
static PGconn *connect_to_db(Target *tg);
static PGresult *exec_stat_column_query(PGconn *conn, Target *tg);
static void __attribute__((noreturn)) exit_on_pq_error(PGconn *conn);
static void print_tuple(PGresult *res, size_t row);

int main(int argc, char *argv[]) {
  Target tg;
  if (!load_target(argc, argv, &tg)) {
    print_usage(argv[0]);
    return 1;
  };

  PGconn *conn = connect_to_db(&tg);
  if (PQstatus(conn) != CONNECTION_OK)
    exit_on_pq_error(conn);

  PGresult *res = exec_stat_column_query(conn, &tg);
  if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK) {
    PQclear(res);
    exit_on_pq_error(conn);
  }
  print_tuple(res, 0);
  PQclear(res);
  PQfinish(conn);
  return EXIT_SUCCESS;
}


bool load_target(int argc, char *argv[], Target *t) {
  assert(t);
  if (argc < 4) {
    return false;
  }
  t->db_name = argv[1];
  t->table_name = argv[2];
  t->column_name = argv[3];

  const char *env_var = getenv("DB_ADDR");
  t->db_addr = env_var ? env_var : "localhost";

  env_var = getenv("DB_PORT");
  t->db_port = env_var ? env_var : "5432";

  env_var = getenv("DB_USER");
  t->db_user = env_var ? env_var : "postgres";

  env_var = getenv("DB_PASSWORD");
  t->db_password = env_var ? env_var : "qwe123";

  return true;
}


static PGconn *connect_to_db(Target *tg) {
  assert(tg);
  const char *keys[] = {"host", "port", "user", "password", "dbname", "connect_timeout", NULL};
  const char *values[] = {tg->db_addr, tg->db_port,     tg->db_user, tg->db_password,
                          tg->db_name, CONNECT_TIMEOUT, NULL};
  return PQconnectdbParams(keys, values, 0);
}


static PGresult *exec_stat_column_query(PGconn *conn, Target *tg) {
  assert(tg);
  const char *query_format = "SELECT AVG(%s), MAX(%s), MIN(%s), VARIANCE(%s), SUM(%s) FROM %s";
  int query_size = strlen(query_format) + 5 * strlen(tg->column_name) + strlen(tg->table_name);
  char *query = (char *)malloc(query_size);
  if (!query) {
    puts("Memory allocation error");
    return NULL;
  }
  snprintf(query, query_size, query_format, tg->column_name, tg->column_name, tg->column_name,
           tg->column_name, tg->column_name, tg->table_name);
  PGresult *res = PQexec(conn, query);
  free(query);
  return res;
}


static void print_tuple(PGresult *res, size_t row) {
  if (res && row < PQntuples(res)) {
    int ncols = PQnfields(res);
    for (int i = 0; i < ncols; i++) {
      char *name = PQfname(res, i);
      char *val = PQgetvalue(res, row, i);
      printf(" %s = %s \n", name, val);
    }
    printf("\n");
  }
}


static void __attribute__((noreturn)) exit_on_pq_error(PGconn *conn) {
  puts(PQerrorMessage(conn));
  PQfinish(conn);
  exit(EXIT_FAILURE);
}