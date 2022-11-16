/*******************************************************************************
 *             АДАПТАЦИЯ ДЛЯ КАРТЫ  (CONST CHAR* -> INTMAX_T)                  *
 *******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifndef MAP_MAX_KEY_LENGTH
#define MAP_MAX_KEY_LENGTH 8096
#endif

#if MAP_MAX_KEY_LENGTH < 2
  #error Invalid MAP_MAX_KEY_LENGTH
#endif

static bool map_key_compare(const char *ls, const char *rs) {
  if (ls == rs)
    return true;
  else if (ls == NULL || rs == NULL)
    return false;
  else
    return strncmp(ls, rs, MAP_MAX_KEY_LENGTH) == 0;
}


static MapEntry *map_create_entry(const char *rs, intmax_t val) {
  assert(rs != NULL);
  size_t len = strnlen(rs, MAP_MAX_KEY_LENGTH);
  char *pstr = (char *)malloc(len + 1);
  if (pstr == NULL) {
    return NULL;
  }
  memcpy(pstr, rs, len);
  pstr[len] = '\0';

  MapEntry *pe = (MapEntry *)malloc(sizeof(MapEntry));
  if (pe == NULL) {
    free(pstr);
    return NULL;
  }
  pe->key = pstr;
  pe->val = val;
  return pe;
}

static void map_free_entry(MapEntry *pe) {
  assert(pe);
  free((void *)pe->key);
  free(pe);
}

