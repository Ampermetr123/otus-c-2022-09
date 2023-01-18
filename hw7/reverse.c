#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define UINT_TO_PTR(v) (void *)(v)
#define PTR_TO_UINT(v) (uint64_t)(v)

uint64_t data[] = {4, 8, 15, 16, 23, 42};
uint64_t data_length = sizeof(data) / sizeof(data[0]);

enum { ival, iprev }; // indexes of 'stucture as array' uint64_t[]

void print_int(uint64_t *v) {
  fprintf(stdout, "%" PRIu64 " ", *v);
  fflush(stdout);
}

void free_cb(uint64_t *v) { free(v); }

/// return: 1 - v is odd, 0 - v is eval
uint64_t p(uint16_t v) { return v & 1; }

/// p - top of stack, or NULL to create new stack
/// v - stored value
uint64_t *add_element(uint64_t *p, uint64_t v) {
  uint64_t *ret = (uint64_t *)malloc(16);
  if (!ret)
    return NULL;
  ret[iprev] = p ? PTR_TO_UINT(p) : 0;
  ret[ival] = v;
  return ret;
}

// for_each
void m(uint64_t *p, void (*f)(uint64_t *)) {
  if (p) {
    uint64_t *prev = UINT_TO_PTR(p[iprev]);
    f(p);
    m(prev, f);
  }
}

// make copy with filtering (transform)
uint64_t *f(uint64_t *p, uint64_t *op, uint64_t (*filter)(uint16_t)) {
  if (!p)
    return op;
  if (filter(p[ival])) {
    op = add_element(op, p[ival]);
  }
  return f(UINT_TO_PTR(p[iprev]), op, filter);
}


int main() {
  uint64_t *s1 = NULL, *s2 = NULL;

  uint64_t len = data_length;
  while (len) {
    s1 = add_element(s1, data[--len]);
  }

  m(s1, print_int);
  puts("");

  s2 = f(s1, s2, p);

  m(s2, print_int);
  puts("");

  m(s1, free_cb);
  m(s2, free_cb);

  return 0;
}