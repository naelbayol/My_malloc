#include "malloc.h"

struct counta g_head = { .head = NULL };

size_t align_size(size_t size)
{
    return (size + 16 - 1) & ~(16 - 1);
}

struct meta *alloc(size_t size, struct meta *last)
{
    size_t page = sysconf(_SC_PAGESIZE);
    size_t len = (size + sizeof(struct meta) + page - 1) / page * page;
    struct meta *acc = mmap(NULL, len, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (acc == MAP_FAILED)
    {
        return NULL;
    }
    acc->allo = 'f';
    acc->size = len - sizeof(struct meta);
    acc->next = NULL;
    acc->prev = last;
    if (last == NULL)
    {
        g_head.head = acc;
    }
    else
    {
        last->next = acc;
    }
    return acc;
}

void cut(struct meta *block, size_t size)
{
    if (block->size < size + sizeof(struct meta) + 16)
    {
        return;
    }
    void *tmp = block + 1;
    char *data = tmp;
    tmp = data + size;
    struct meta *rest = tmp;
    rest->allo = 'f';
    rest->size = block->size - size - sizeof(struct meta);
    rest->next = block->next;
    rest->prev = block;
    if (block->next != NULL)
    {
        block->next->prev = rest;
    }
    block->next = rest;
    block->size = size;
}

struct meta *find_block(size_t size, struct meta **last)
{
    struct meta *acc = g_head.head;
    *last = NULL;
    while (acc != NULL)
    {
        if (acc->allo == 'f' && acc->size >= size)
        {
            return acc;
        }
        *last = acc;
        acc = acc->next;
    }
    return NULL;
}

int is_next_to(struct meta *a, struct meta *b)
{
    void *tmp = a + 1;
    char *data = tmp;
    tmp = data + a->size;
    return tmp == b;
}

void merge_next(struct meta *block)
{
    struct meta *next = block->next;
    if (next == NULL || next->allo != 'f' || !is_next_to(block, next))
    {
        return;
    }
    block->size += next->size + sizeof(struct meta);
    block->next = next->next;
    if (block->next != NULL)
    {
        block->next->prev = block;
    }
}

struct meta *merge_prev(struct meta *block)
{
    struct meta *prev = block->prev;
    if (prev == NULL || prev->allo != 'f' || !is_next_to(prev, block))
    {
        return block;
    }
    merge_next(prev);
    return prev;
}

void unmap_block(struct meta *block)
{
    if (block->prev != NULL && is_next_to(block->prev, block))
    {
        return;
    }
    if (block->next != NULL && is_next_to(block, block->next))
    {
        return;
    }
    if (block->prev != NULL)
    {
        block->prev->next = block->next;
    }
    else
    {
        g_head.head = block->next;
    }
    if (block->next != NULL)
    {
        block->next->prev = block->prev;
    }
    munmap(block, block->size + sizeof(struct meta));
}

struct meta *get_meta(void *ptr)
{
    struct meta *block = ptr;
    return block - 1;
}

void reduce_block(struct meta *block, size_t size)
{
    struct meta *next = block->next;
    cut(block, size);
    if (block->next != next)
    {
        merge_next(block->next);
    }
}
