#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif  

enum cq_res{
  cq_res_ok,
  cq_res_lost_tail,
  cq_res_lost_head,
  cq_res_invalid_arg,
  cq_res_not_enough_mem,
  cq_res_queue_empty,
  cq_res_bad_iterator,
  cq_res_error
};

// Свойства очереди при pop_head_on_overhead = false
// Можно безопсано читать вершину элемента в одном потоке (cq_top)
// даже если происходит добавлении в конец в другом (cq_push)


typedef struct CQueue CQueue;
typedef const char* cq_iterator_t;

CQueue* cq_new(size_t size, bool pop_head_on_overhead);

int cq_push(CQueue* cq, const char* pack, unsigned int pack_sz);

int cq_top(CQueue* cq, char** pack, unsigned int* pack_sz);

int cq_back(CQueue* cq, char** pack, unsigned int* pack_sz);

int cq_replace_back(CQueue* cq, const char* pack, unsigned int pack_sz);

int cq_back_shrink(CQueue* cq, unsigned int new_size);

void cq_pop(CQueue* cq);

size_t cq_length(CQueue* cq);

void cq_set_max_length(CQueue* cq, size_t len);

void cq_clear(CQueue* cq);

void cq_free(CQueue* cq);

cq_iterator_t cq_begin(CQueue* cq);

cq_iterator_t cq_next(CQueue* cq, cq_iterator_t iter);

int cq_get(CQueue* cq, cq_iterator_t iter, const char** pack, unsigned int* pack_sz);

int cq_push_from_iter(CQueue* cq_dest, CQueue* cq_src, cq_iterator_t src_it);


#ifdef __cplusplus
} //extern "C" 
#endif


