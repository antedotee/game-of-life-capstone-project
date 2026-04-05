#include "memory.h"

#define MEMORY_POOL_SIZE (1024u * 512u)

typedef struct FreeBlock {
    unsigned long size;
    struct FreeBlock *next;
} FreeBlock;

static unsigned char g_memory_pool[MEMORY_POOL_SIZE];
static FreeBlock *g_free_list;
static int g_memory_ready;

static unsigned long align_up(unsigned long x)
{
    return (x + 7u) & ~7u;
}

void memory_init(void)
{
    g_free_list = (FreeBlock *)g_memory_pool;
    g_free_list->size = MEMORY_POOL_SIZE;
    g_free_list->next = 0;
    g_memory_ready = 1;
}

void *memory_alloc(unsigned long size)
{
    unsigned long need;
    FreeBlock **walk;
    FreeBlock *blk;
    if (!g_memory_ready || size == 0) {
        return 0;
    }
    need = align_up(size + sizeof(unsigned long));
    if (need < sizeof(FreeBlock)) {
        need = sizeof(FreeBlock);
    }
    walk = &g_free_list;
    while (*walk) {
        blk = *walk;
        if (blk->size >= need) {
            if (blk->size - need >= sizeof(FreeBlock) + 8u) {
                FreeBlock *rest = (FreeBlock *)((unsigned char *)blk + need);
                rest->size = blk->size - need;
                rest->next = blk->next;
                *walk = rest;
            } else {
                need = blk->size;
                *walk = blk->next;
            }
            *(unsigned long *)blk = need;
            return (unsigned char *)blk + sizeof(unsigned long);
        }
        walk = &(*walk)->next;
    }
    return 0;
}

void memory_dealloc(void *ptr)
{
    FreeBlock *blk;
    if (!ptr) {
        return;
    }
    blk = (FreeBlock *)((unsigned char *)ptr - sizeof(unsigned long));
    blk->size = *(unsigned long *)blk;
    blk->next = g_free_list;
    g_free_list = blk;
}
