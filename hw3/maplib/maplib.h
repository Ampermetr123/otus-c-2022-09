#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef MAP_TYPE
#define MAP_TYPE STR_TO_INTMAX
#endif

#if MAP_TYPE == STR_TO_INTMAX
  typedef const char *MapKey;
  typedef intmax_t MapVal;
  #define MAP_MAX_KEY_LENGTH 8096
#else
#error Incorrect MAP_TYPE!
#endif

typedef struct {
  MapKey key;
  MapVal val;
} MapEntry;

typedef uint32_t (*HashFunc)(MapKey); 

typedef struct Map Map;

/// Create new map
Map *map_new(HashFunc hash_func);

/// Insert key-value; return false on error or if key already present
bool map_insert(Map *map, MapKey key, MapVal val);

/// Remove key-value; return false if key is not in map
bool map_remove(Map *map, MapKey key);

/// Find location of value associated with key;
/// return NULL, if key is not in map
MapVal *map_find(Map *map, MapKey key);

/// Return number of elements, stored in map
size_t map_count(Map *map);

/// Free memory
void map_destroy(Map **pmap);

// Iterators 

typedef size_t MapIter;

static inline MapIter map_iter_end() { return SIZE_MAX; }

MapIter map_iter_begin(Map *map);

MapEntry *map_iter_get(Map *map, MapIter iter);

MapIter map_iter_next(Map *map, MapIter iter);
