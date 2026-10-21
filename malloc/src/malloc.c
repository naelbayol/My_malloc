#include "malloc.h"

__attribute__((visibility("default"))) void *malloc(size_t size)
{
    if (size > SIZE_MAX / 2)
    {
        errno = ENOMEM;
        return NULL;
    }
    size = align_size(size);
    struct meta *last = NULL;
    struct meta *acc = find_block(size, &last);
    if (acc == NULL)
    {
        acc = alloc(size, last);
        if (acc == NULL)
        {
            return NULL;
        }
    }
    cut(acc, size);
    acc->allo = 'a';
    return acc + 1;
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    struct meta *tofree = ptr;
    tofree = tofree - 1;
    tofree->allo = 'f';
    merge_next(tofree);
    tofree = merge_prev(tofree);
    unmap_block(tofree);
}

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return malloc(size);
    }
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }
    struct meta *block = get_meta(ptr);
    if (size > block->size)
    {
        void *res = malloc(size);
        if (res == NULL)
        {
            return NULL;
        }
        memcpy(res, ptr, block->size);
        free(ptr);
        return res;
    }
    reduce_block(block, align_size(size));
    return ptr;
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
    if (__builtin_mul_overflow(nmemb, size, &size))
    {
        errno = ENOMEM;
        return NULL;
    }
    void *res = malloc(size);
    if (res == NULL)
    {
        return NULL;
    }
    memset(res, 0, size);
    return res;
}
