#ifndef _PROCESS_H
#define _PROCESS_H
#include "common.h"
#include "defines.h"

typedef enum process_state {
	NONE,
	WAITING,
	READY,
	RUNNING
} process_state_t;

typedef struct process
{
	int pid;
	int tid;
	int open_files;
	char *path;
	char *cmd_line;
	void *page_table;
	void *stack_base;
	process_state_t state;
	uint64_t stack_size;
	uint64_t entry;
	uint64_t sbrk;
	mem_region_t *head_region;	
	context_t ctx;
	fd_table_t fd_table;
	TranslationTables_t t;
	struct process *next;
} process_t;

int delete_fd(int pid, int host_fd, int k_fd);
int append_fd_table( struct fd_table *ptr);
int get_host_fd(int pid, int k_fd);
int append_fd(int pid, int k_fd);
int insert_fd(int pid, int host_fd, int k_fd);
#endif
