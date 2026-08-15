#include "pool.h"
#include <assert.h>

void pool_init(
  Pool *pool,
  void *backing_buffer,
  size_t backing_buffer_len,
  size_t node_size
){
  // Assert that the passed params are valid.
  assert(backing_buffer_len >= node_size);
  //printf("node_size(%zu) >= sizeof(Pool_Node)=(%zu)\n", node_size, sizeof(Pool_Node));
  assert(node_size >= sizeof(Pool_Node));

  // set values in pool.
  pool->buf = backing_buffer;
  pool->buffer_len = backing_buffer_len;
  pool->node_size = node_size;
  pool->head = NULL;
  // free the pool for the user.


  pool_free_all(pool);
}

void pool_free_all(Pool *pool){
  size_t node_count = pool->buffer_len / pool->node_size;

  pool->head = NULL;
  for(size_t i = node_count; i > 0; i--){
    void *ptr = &pool->buf[(i-1) * pool->node_size];
    Pool_Node *node = (Pool_Node *) ptr;
    node->next = pool->head;
    pool->head = node;
  }
}

void *pool_alloc(Pool *pool){
  if(pool->head == NULL){
    printf("Attempted to allocate pool but there's no memory left.");
    return NULL;
  }

  // Get latest free node
  Pool_Node *node = pool->head;

  // pop free node
  pool->head = pool->head->next;

  printf("Pool just allocated new memory block at address %p\n", node);

  // return and zero memory by default
  return memset(node, 0, pool->node_size);
}

void pool_free(Pool *pool, void *ptr){
  // Confirm that the ptr is actually in our pool.
  void *buffer_start = pool->buf;
  void *buffer_end = pool->buf + pool->buffer_len;
  assert(ptr >= buffer_start && ptr <= buffer_end );

  Pool_Node *node = (Pool_Node *) ptr;
  node->next = pool->head;
  pool->head = node;

// new for ff
//memset(node, 0, pool->node_size);
}

void pool_print_info(Pool *pool){
  printf("pool buffer address: %p\n", pool->buf);
  printf("pool buffer size: %lu\n", pool->buffer_len);
  printf("pool node size: %lu\n", pool->node_size);
  printf("pool buffer end address: %p\n", pool->buf + pool->buffer_len);
  printf("-----------------------------------------\n");
}

bool pool_is_block_free(Pool *pool, void *ptr) {
  Pool_Node *current = pool->head;
  while(current != NULL) {
    if(current == ptr) {
      return true;
    }
    current = current->next;
  }
  return false;
}
