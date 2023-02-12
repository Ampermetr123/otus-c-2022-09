#include <libpq/libpq-fs.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char* argv[]) {
  if (argc < 4) {
    printf("Usage\n: %s <database_name>  <table_name>  <column_name>\n", argv[0]);
  }

  const char* db_name = argv[1];
  const char* table_name = argv[2];
  const char* column_name = argv[3];
  

  return EXIT_SUCCESS;
}
