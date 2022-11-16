#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef MAP_INIT_SIZE
#define MAP_INIT_SIZE 32
#endif
#ifndef MAP_LOAD_FACTOR
#define MAP_LOAD_FACTOR 0.45f
#endif
#ifndef MAP_RESIZE_FACTOR
#define MAP_RESIZE_FACTOR 2.0f
#endif
#ifndef MAP_TYPE
#define MAP_TYPE STR_TO_INTMAX
#endif


// typedef for MapKey and MapVal
#if MAP_TYPE == STR_TO_INTMAX
#include "impl/map_str_int_types.h"
#else
#error You need to define coorect map type!
#endif


typedef struct {
  MapKey key;
  MapVal val;
} MapEntry;

typedef uint32_t (*HashFunc)(MapKey); 
typedef bool (*KeyCompareFunc)(MapKey, MapKey); /// return true if keys are equal
typedef MapEntry *(*CreateEntryFunc)(MapKey key, MapVal val); 
typedef void (*FreeEntryFunc)(MapEntry *pkv);

typedef struct Map Map;
Map *map_new_impl(HashFunc hash_func, KeyCompareFunc cmp_func, CreateEntryFunc create_func,
                  FreeEntryFunc free_func);

// Implementation specific map functions
// KeyCompareFunc, CreateEntryFunc, FreeEntryFunc
#if MAP_TYPE == STR_TO_INTMAX
#include "impl/map_str_int.h"
#else
#error You need to define coorect map type!
#endif


/// Create new map
static inline Map *map_new(HashFunc hash_func) {
  return map_new_impl(hash_func, map_key_compare, map_create_entry, map_free_entry);
}

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
