#ifndef MEMORY_H
#define MEMORY_H

void memory_init(void);
void *memory_alloc(unsigned long size);
void memory_dealloc(void *ptr);

#endif
