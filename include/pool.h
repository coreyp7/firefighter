#ifndef POOL_H
#define POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Pool_Node Pool_Node;
struct Pool_Node {
  Pool_Node *next;
};

typedef struct Pool Pool;
struct Pool {
  unsigned char *buf;
  size_t buffer_len;
  size_t node_size;
  Pool_Node *head;
};


void pool_init(
  Pool *pool,
  void *backing_buffer,
  size_t backing_buffer_len,
  size_t node_size
);


void pool_free_all(Pool *pool);


void *pool_alloc(Pool *pool);

void pool_free(Pool *pool, void *ptr);

void pool_print_info(Pool *pool);

bool pool_is_block_free(Pool *pool, void *ptr);

#endif
