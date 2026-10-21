#ifndef MALLOC_H
#define MALLOC_H

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

struct meta
{
    char allo;
    size_t size;
    struct meta *next;
    struct meta *prev;
};

struct counta
{
    struct meta *head;
};

extern struct counta g_head;

size_t align_size(size_t size);
struct meta *alloc(size_t size, struct meta *last);
void cut(struct meta *block, size_t size);
struct meta *find_block(size_t size, struct meta **last);
int is_next_to(struct meta *a, struct meta *b);
void merge_next(struct meta *block);
struct meta *merge_prev(struct meta *block);
void unmap_block(struct meta *block);
struct meta *get_meta(void *ptr);
void reduce_block(struct meta *block, size_t size);

__attribute__((visibility("default"))) void *malloc(size_t size);
__attribute__((visibility("default"))) void free(void *ptr);
__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size);
__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size);

#endif /* ! MALLOC_H */
