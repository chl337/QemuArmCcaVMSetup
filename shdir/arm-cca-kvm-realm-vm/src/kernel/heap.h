#ifndef _HEAP_H
#define _HEAP_H
#include <stdint.h>

int heap_init();
void *kmalloc(uint64_t size);
void *kcalloc(uint64_t nmeb, uint64_t count);
void kfree(void *address);

void *smalloc(uint64_t size);
void *scalloc(uint64_t nmeb, uint64_t count);
void sfree(void *address);

#endif
