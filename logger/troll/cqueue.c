#include "cqueue.h"

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>


typedef struct PackHeader {
  unsigned int size; // размер данных пакета (без заголовка)
  struct PackHeader *next;
} PackHeader;


#define CQ_ALIGN sizeof(size_t)


struct CQueue {
  char *buf;
  size_t buf_size;
  PackHeader *head;
  PackHeader *tail;
  size_t length;
  size_t packets_limit;
  bool pop_head_on_overhead;
};


CQueue *cq_new(size_t size, bool pop_head_on_overhead) {
  CQueue *cq = aligned_alloc(alignof(PackHeader), sizeof(CQueue));
  if (!cq)
    return NULL;
  cq->buf = aligned_alloc(CQ_ALIGN, size);
  cq->buf_size = size;
  cq->head = NULL;
  cq->tail = NULL;
  cq->length = 0;
  cq->pop_head_on_overhead = pop_head_on_overhead;
  cq->packets_limit = 0; // unlimited
  return cq;
}


void cq_clear(CQueue *cq) {
  assert(cq);
  if (!cq)
    return;
  cq->head = cq->tail = NULL;
}


void cq_free(CQueue *cq) {
  assert(cq);
  if (!cq)
    return;
  free(cq->buf);
  free(cq);
}


size_t cq_length(CQueue *cq) {
  assert(cq);
  if (cq)
    return cq->length;
  else
    return 0;
}


void cq_set_max_length(CQueue *cq, size_t len) {
  if (!cq)
    return;
  cq->packets_limit = len;
  if (len > 0) {
    while (cq_length(cq) > len) {
      cq_pop(cq);
    }
  }
}


void cq_pop(CQueue *cq) {
  assert(cq);
  if (!cq)
    return;
  if (cq->head == NULL)
    return;

  if (cq->head->next == NULL) {
    cq->head = NULL;
  } else {
    cq->head = cq->head->next;
  }
  cq->length -= 1;
}


int cq_top(CQueue *cq, char **pack, unsigned int *pack_sz) {
  if (!cq || !pack || !pack_sz)
    return cq_res_invalid_arg;
  if (cq->head) {
    *pack = (char *)(cq->head) + sizeof(PackHeader);
    *pack_sz = cq->head->size;
    return cq_res_ok;
  }
  return cq_res_queue_empty;
}


int cq_back(CQueue *cq, char **pack, unsigned int *pack_sz) {
  if (!cq || !pack)
    return cq_res_invalid_arg;
  if (cq->tail) {
    *pack = (char *)(cq->tail) + sizeof(PackHeader);
    if (pack_sz)
      *pack_sz = cq->tail->size;
    return cq_res_ok;
  }
  return cq_res_queue_empty;
}


int cq_back_shrink(CQueue *cq, unsigned int new_size) {
  if (!cq)
    return cq_res_invalid_arg;
  if (cq->tail) {
    if (cq->tail->size < new_size) {
      return cq_res_invalid_arg;
    }
    cq->tail->size = new_size;
    return cq_res_ok;
  }
  return cq_res_queue_empty;
}


static inline bool is_in_busy_area(CQueue *cq, const char *ptr) {
  if (!cq->head || !cq->tail) {
    return false;
  }
  char *start = (char *)cq->head;
  char *end = (char *)cq->tail + cq->tail->size + sizeof(PackHeader) - 1;
  if (start < end) {
    return ptr >= start && ptr <= end;
  } else {
    return ptr <= end || ptr >= start;
  }
}


int cq_push(CQueue *cq, const char *pack, unsigned int pack_sz) {
  assert(cq);
  if (!cq || !cq->buf || pack_sz == 0)
    return cq_res_invalid_arg;
  if (pack_sz + sizeof(PackHeader) > cq->buf_size) {
    return cq_res_not_enough_mem;
  }

  int ret_val = cq_res_ok;
  char *start_ptr;

  if (cq->tail != NULL) {
    start_ptr = (char *)cq->tail + cq->tail->size + sizeof(PackHeader);
    size_t align = (uintptr_t)start_ptr % CQ_ALIGN;
    if (align != 0)
      start_ptr += align;
    char *end_ptr = start_ptr + sizeof(PackHeader) + pack_sz;
    if (end_ptr > cq->buf + cq->buf_size) {
      start_ptr = cq->buf;
      end_ptr = start_ptr + sizeof(PackHeader) + pack_sz;
    }

    // bool overlap;
    while (is_in_busy_area(cq, start_ptr) || is_in_busy_area(cq, end_ptr - 1)) {
      if (cq->pop_head_on_overhead) {
        cq_pop(cq);
        ret_val = cq_res_lost_head;
      } else {
        return cq_res_lost_tail;
      }
    }

    cq->tail->next = (PackHeader *)(start_ptr);
    cq->tail = cq->tail->next;
    if (cq->head == NULL)
      cq->head = cq->tail;
  } else {
    start_ptr = cq->buf;
    cq->tail = (PackHeader *)cq->buf;
    cq->head = (PackHeader *)cq->buf;
    
  }

  cq->tail->size = pack_sz;
  cq->tail->next = NULL;
  if (pack) {
    memcpy(start_ptr + sizeof(PackHeader), pack, pack_sz);
  }
  cq->length += 1;

  if (cq->packets_limit > 0 && cq->length > cq->packets_limit) {
    cq_pop(cq);
    return cq_res_lost_head;
  }

  return ret_val;
}

int cq_push_from_iter(CQueue *cq_dest, CQueue *cq_src, cq_iterator_t src_it) {
  // Проверки
  if (src_it == NULL)
    return cq_res_bad_iterator;
  if (cq_dest == NULL || cq_src == NULL)
    return cq_res_invalid_arg;
  PackHeader *src_node = (PackHeader *)(src_it);
  char *src_end = (char *)src_node + sizeof(PackHeader) + src_node->size - 1;
  if (src_node->size == 0 || !is_in_busy_area(cq_src, (char *)src_it) ||
      !is_in_busy_area(cq_src, src_end)) {
    return cq_res_bad_iterator;
  }
  return cq_push(cq_dest, (char *)src_it + sizeof(PackHeader), src_node->size);
}

int cq_replace_back(CQueue *cq, const char *pack, unsigned int pack_sz) {
  assert(cq);
  if (!cq || !cq->buf || pack_sz == 0)
    return cq_res_invalid_arg;
  if (pack_sz + sizeof(PackHeader) > cq->buf_size) {
    return cq_res_not_enough_mem;
  }
  if (cq->tail == NULL) {
    return cq_res_queue_empty;
  }

  int ret_val = cq_res_ok;
  char *start_ptr = (char *)cq->tail;
  char *end_ptr = start_ptr + sizeof(PackHeader) + pack_sz;
  if (end_ptr > cq->buf + cq->buf_size) {
    start_ptr = cq->buf;
    end_ptr = start_ptr + sizeof(PackHeader) + pack_sz;
  }

  // bool overlap;
  while (is_in_busy_area(cq, start_ptr) || is_in_busy_area(cq, end_ptr - 1)) {
    if (cq->pop_head_on_overhead) {
      cq_pop(cq);
      ret_val = cq_res_lost_head;
    } else {
      return cq_res_lost_tail;
    }
  }

  if (cq->head == NULL) {
    cq->head = cq->tail = (PackHeader *)cq->buf;
    cq->length = 1;
  } else if (start_ptr != (char *)cq->tail) {
    // адрес хвоста изменился
    PackHeader *node = cq->head;
    while (node->next != NULL && node->next != cq->tail) {
      node = node->next;
    }
    if (node->next) {
      node->next = (PackHeader *)start_ptr;
      cq->tail = (PackHeader *)start_ptr;
    } else {
      return cq_res_error;
    }
  }
  cq->tail->size = pack_sz;
  cq->tail->next = NULL;
  if (pack) {
    memcpy(start_ptr + sizeof(PackHeader), pack, pack_sz);
  }
  return ret_val;
}


cq_iterator_t cq_begin(CQueue *cq) { return (cq_iterator_t)cq->head; }


cq_iterator_t cq_next(CQueue *cq, cq_iterator_t iter) {
  if (iter) {
    PackHeader *next = ((PackHeader *)iter)->next;
    if (is_in_busy_area(cq, (char *)next)) {
      return (char *)(next);
    }
  }
  return NULL;
}


int cq_get(CQueue *cq, cq_iterator_t iter, const char **pack, unsigned int *pack_sz) {
  if (iter == NULL)
    return cq_res_invalid_arg;
  PackHeader *node = (PackHeader *)(iter);
  char *end = iter + sizeof(PackHeader) + node->size - 1;
  if (is_in_busy_area(cq, iter) && is_in_busy_area(cq, end)) {
    if (pack){
      *pack = iter+sizeof(PackHeader);
    }
    if (pack_sz)
      *pack_sz = node->size;
    return cq_res_ok;
  }
  return cq_res_bad_iterator;
}