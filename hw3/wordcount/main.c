#include "maplib.h"
#include "hash.h"

#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char **argv) {

  if (argc < 2) {
    puts("Count amount of each word in text file and list it.\n"
         "Usage:\t wordcount <path_to_archive>\n\n");
    return EXIT_FAILURE;
  }

  const char *file_path = argv[1];
  FILE *fp = fopen(file_path, "r");
  if (!fp) {
    perror("Error openinig file");
    return EXIT_FAILURE;
  }

  Map *map = map_new(pearson_hash32);
  if (!map) {
    puts("Interanl error on creating hash table\n");
    return EXIT_FAILURE;
  }

  char word[MAP_MAX_KEY_LENGTH];
  size_t len = 0;
  int ch;
  while ((ch = fgetc(fp)) != EOF && len < MAP_MAX_KEY_LENGTH-1) {
    if (isprint(ch) && !isspace(ch)) {
      word[len] = (char)ch;
      len++;
    } else if (len>0) {
      word[len] = '\0';
      intmax_t *val = map_find(map, word);
      if (val == NULL) {
        if (!map_insert(map, word, 1)) {
          puts("Error on map_insert\n");
          return EXIT_FAILURE;
        }
      } else {
        *val += 1;
      }
      len = 0; 
    }
  }

  for (MapIter it = map_iter_begin(map); it != map_iter_end(); it = map_iter_next(map, it)) {
    MapEntry *me = map_iter_get(map, it);
    printf("%s -> %ld \n", me->key, me->val);
  }

  printf("-----------------------------------------------------\nTotal unique words: %ld\n",
         map_count(map));
  
  map_destroy(&map);

  return 0;
}