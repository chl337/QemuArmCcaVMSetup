#ifndef _COMMON_H
#define _COMMON_H
#include <stdint.h>
#include "linker_defines.h"

typedef struct context {
	uint64_t spsr_el1;
	uint64_t elr_el1;
	uint64_t esr_el1;
	uint64_t zero;
	uint64_t x[32];
	uint64_t far;
	uint64_t sp;
} context_t;

typedef struct fd_table
{
	int is_kernel;
	int k_fd;
	int host_fd;
	int fd;
	int pid;
	struct fd_table *next;
} fd_table_t;

typedef struct memory_region
{
	uint64_t va_base;
	uint64_t va_size;
	int perm;
	int is_mapped;
	int is_used;
	int is_device;
	int is_kernel;
	int is_secure;
	int is_page_table;
	int fd;
	struct memory_region *next;
} mem_region_t;

void crash();

#endif
