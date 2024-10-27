#ifndef _VMA_H
#define _VMA_H
#include <stdint.h>
#include "defines.h"
#include "common.h"

#define KFRAME_USED 0xFF
#define KFRAME_FREE 0x00 
#define KFRAME_SIZE 0x1000 

#define MAP_FAILED (-1UL)

#define MAP_FIXED (0x1)
#define MAP_ANONYMOUS (0x2)

#define PAGE_UP(x) (((x) + 0xFFF) & ~(0xFFFUL))
#define PAGE_DOWN(x) ((x)  & ~(0x0FFFUL))

typedef struct kframe_allocator {
	uint64_t frame_count;
	uint64_t frame_start;
	uint8_t *bitmap_frame;
	uint64_t bitmap_count;	
	uint64_t frame_lock;
	uint64_t available_frame_count;
} kframe_alloc_t;

mem_region_t *kernel_vma_alloc(void *address, uint64_t page_count, int flags);
int mmap_region_insert(mem_region_t **head, mem_region_t *to_insert);

void create_user_translation_table(TranslationTables_t *t);
uint64_t get_shared_bit();

int kframe_allocator_init(kframe_alloc_t *fa, uint64_t bitmap_frame, uint64_t bitmap_count, uint64_t frame_start, uint64_t size);
uint64_t kframe_allocate_fixed(kframe_alloc_t *fa, uint64_t start, uint64_t count);
uint64_t kframe_allocate_range(kframe_alloc_t *fa, uint64_t count);
uint64_t kframe_allocate(kframe_alloc_t *fa);
void kframe_free(kframe_alloc_t *fa, uint64_t address);

int kframe_allocator_init_secure(uint64_t bitmap_frame, uint64_t bitmap_count, uint64_t frame_start, uint64_t size);
uint64_t kframe_allocate_fixed_secure(uint64_t start, uint64_t count);
uint64_t kframe_allocate_range_secure(uint64_t count);
uint64_t kframe_allocate_secure();
void kframe_free_secure(uint64_t address);

int kframe_allocator_init_insecure(uint64_t bitmap_frame, uint64_t bitmap_count, uint64_t frame_start, uint64_t size);
uint64_t kframe_allocate_fixed_insecure(uint64_t start, uint64_t count);
uint64_t kframe_allocate_range_insecure(uint64_t count);
uint64_t kframe_allocate_insecure();
void kframe_free_insecure(uint64_t address);

int page_table_map(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags, int is_dev);
int page_table_map_device(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags);
int page_table_unmap(TranslationTables_t *t, uint64_t va, uint64_t count);
int page_table_map_kernel(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags);
int page_table_map_user(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags);
int page_table_map_device(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags);

uint64_t k_sbrk(int64_t length);
int k_brk(void *address);
uint64_t shared_sbrk(int64_t length);
int shared_brk(void *address);

void *get_physical_address(uint64_t va);
int access_is_ok(uint64_t va);
int access_user_ok(uint64_t va, int prot);
int access_kernel_ok(uint64_t va, int prot);

#endif
