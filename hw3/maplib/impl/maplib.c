#include "../maplib.h"
#include <assert.h>
#include <stdlib.h>

#ifndef MAP_INIT_SIZE
#define MAP_INIT_SIZE 32
#endif
#ifndef MAP_LOAD_FACTOR
#define MAP_LOAD_FACTOR 0.45f
#endif
#ifndef MAP_RESIZE_FACTOR
#define MAP_RESIZE_FACTOR 2.0f
#endif

typedef bool (*KeyCompareFunc)(MapKey, MapKey); /// return true if keys are equal
typedef MapEntry *(*CreateEntryFunc)(MapKey key, MapVal val); 
typedef void (*FreeEntryFunc)(MapEntry *pkv);

struct Map {
  HashFunc hash_func;
  KeyCompareFunc cmp_func;
  CreateEntryFunc create_entry_func;
  FreeEntryFunc free_entry_func;
  size_t data_size;
  MapEntry **data;     // то же, что и MapEntry* data[]
  size_t stored_count; // количество элементов
  size_t count; // количество элементов, в том числе помеченных как удаленные
  size_t count_limit; // по превышению, выполняется рехеширование
  MapEntry *removed_entry; // используем для пометки удаленных ключей
};

Map *map_new_impl(HashFunc hash_func, KeyCompareFunc cmp_func, CreateEntryFunc create_func,
                  FreeEntryFunc free_func) {
  static_assert(MAP_INIT_SIZE > 3, "Incorrect initial map size!");
  assert(MAP_LOAD_FACTOR < 1.0f);
  assert(MAP_INIT_SIZE * MAP_RESIZE_FACTOR > MAP_INIT_SIZE);
  assert(hash_func && "hash_func must be not null");
  assert(cmp_func && "cmp_func must be not null");
  assert(create_func);
  assert(free_func);

  if (!hash_func || !cmp_func || !create_func || !free_func) {
    return NULL;
  }

  MapEntry *pentry = (MapEntry *)malloc(sizeof(MapEntry));
  if (pentry == NULL) {
    return NULL;
  }

  void *pdata = calloc(MAP_INIT_SIZE, sizeof(MapEntry *));
  if (pdata == NULL) {
    free(pentry);
    return NULL;
  }

  Map *pmap = (Map *)malloc(sizeof(Map));
  if (pmap == NULL) {
    free(pentry);
    free(pdata);
    return NULL;
  }
  pmap->hash_func = hash_func;
  pmap->create_entry_func = create_func;
  pmap->free_entry_func = free_func;
  pmap->cmp_func = cmp_func;
  pmap->removed_entry = pentry;
  pmap->data_size = MAP_INIT_SIZE;
  pmap->data = (MapEntry **)(pdata);
  pmap->count = 0;
  pmap->stored_count = 0;
  pmap->count_limit = MAP_INIT_SIZE * MAP_LOAD_FACTOR;
  return pmap;
}

// Implementation specific map functions
// KeyCompareFunc, CreateEntryFunc, FreeEntryFunc
#if MAP_TYPE == STR_TO_INTMAX
#include "map_str_int.h"
#else
#error You need to define coorect map type!
#endif
Map *map_new(HashFunc hash_func) {
  return map_new_impl(hash_func, map_key_compare, map_create_entry, map_free_entry);
}

static inline size_t map_index(uint32_t hashcode, size_t probe, size_t modul) {
  size_t h1 = hashcode % modul;
  size_t h2 = hashcode % (modul - 1) + 1;
  return (h1 + probe * h2) % modul;
}


static MapEntry** map_find_entry(Map *map, MapKey key) {
  assert(map);
  uint32_t hash = map->hash_func(key);
  size_t probe = 0;
  size_t i = 0;
  while (i = map_index(hash, probe++, map->data_size), map->data[i] != NULL) {
    if (map->data[i] == map->removed_entry) {
      continue;
    } 
    if (map->cmp_func(map->data[i]->key, key)) {
      return &map->data[i];
    }
  };
  return NULL;
}


MapVal *map_find(Map *map, MapKey key) {
  assert(map);
  if (map == NULL) {
    return NULL;
  }
  MapEntry** ppme = map_find_entry(map, key);
  if (ppme != NULL) {
    return &((*ppme)->val);
  }
  return NULL;
}


bool map_remove(Map *map, MapKey key) {
  assert(map);
  if (map == NULL) {
    return false;
  }
  MapEntry **ppme = map_find_entry(map, key);
  if (ppme != NULL) {
    map->free_entry_func(*ppme);
    *ppme = map->removed_entry;
    map->stored_count--;
    return true;
  }
  return false;
}


static bool map_rehash(Map *map, size_t new_size) {
  MapEntry **pdata = (MapEntry **)calloc(new_size, sizeof(MapEntry *));
  if (pdata == NULL) {
    return false;
  }
  for (size_t j = 0; j < map->data_size; j++) {
    if (map->data[j] != NULL && map->data[j] != map->removed_entry) {
      uint32_t hash = map->hash_func(map->data[j]->key);
      size_t probe = 0;
      size_t i = 0;
      while (i = map_index(hash, probe++, new_size), pdata[i] != NULL)
        ;
      pdata[i] = map->data[j];
    }
  };
  map->data_size = new_size;
  map->count_limit = new_size * MAP_LOAD_FACTOR;
  map->count = map->stored_count;
  free(map->data);
  map->data = pdata;
  return true;
}


void map_destroy(Map **pmap) {
  assert(pmap);
  Map *map = *pmap;
  if (map == NULL) {
    return;
  }
  for (MapIter it = map_iter_begin(map); it != map_iter_end(); it = map_iter_next(map, it)) {
    map->free_entry_func(map_iter_get(map, it));
  }
  free(map->data);
  free(map->removed_entry);
  free(map);
  *pmap = NULL;
}


bool map_insert(Map *map, MapKey key, MapVal val) {
  assert(map);
  if (map == NULL) {
    return false;
  }

  if (map->count > map->count_limit) {
    if (!map_rehash(map, map->data_size * MAP_RESIZE_FACTOR)) {
      return false;
    };
  }

  uint32_t hash = map->hash_func(key);
  size_t probe = 0;
  size_t i = 0;
  MapEntry **pp_removed_entry = NULL;
  while (i = map_index(hash, probe++, map->data_size), map->data[i] != NULL) {
    if (map->data[i] == map->removed_entry) {
      if (pp_removed_entry == NULL) {
        pp_removed_entry = &(map->data[i]); // Запоминаем первый элемент, помеченный как удаленный
      }
    } else if (map->cmp_func(map->data[i]->key, key) == true) {
      return false; // выход, ключ уже в таблице
    }
  };
  map->data[i] = map->create_entry_func(key, val);

  // Небольшая оптимизация для ускорения доступа
  if (pp_removed_entry != NULL) {
    MapEntry *temp = *pp_removed_entry;
    *pp_removed_entry = map->data[i];
    map->data[i] = temp;
  }

  map->count++;
  map->stored_count++;
  return true;
}


size_t map_count(Map *map) {
  assert(map);
  if (!map) {
    return 0;
  }
  return map->stored_count;
}


MapIter map_iter_begin(Map *map) {
  assert(map);
  if (!map) {
    return map_iter_end();
  }
  size_t i = 0;
  for (; i < map->data_size && map->data[i] == NULL && map->data[i] != map->removed_entry; i++)
    ;
  return i < map->data_size ? i :  map_iter_end();
}

MapEntry *map_iter_get(Map *map, MapIter iter) {
  assert(map);
  if (!map) {
    return NULL;
  }
  return iter < map->data_size ? map->data[iter] : NULL;
}


MapIter map_iter_next(Map *map, MapIter iter) {
  assert(map);
  if (!map || iter >= map->data_size) {
    return map_iter_end();
  }
  size_t i = iter + 1;
  for (; i < map->data_size && map->data[i] == NULL && map->data[i] != map->removed_entry; i++)
    ;
  return i < map->data_size ? i : map_iter_end();
}
