#include "heap.h"
#include "kernel.h"

#define HEAP_MIN_SIZE (0x2000) //8 KB

typedef struct mem_chunk {
	uint64_t size;
	uint64_t used;
	uint64_t address;
	struct mem_chunk *next;
} mem_chunk_t;

#define MIN_HEAP_ALLOC  (sizeof(void*) + sizeof(mem_chunk_t))
struct group_heap {
	mem_chunk_t *head;
	mem_chunk_t *tail;
};

struct group_heap shared_heap = {0,0}; 
struct group_heap private_heap = {0,0}; 

int split(mem_chunk_t *ptr, uint64_t start_size);

uint64_t align(uint64_t n, uint64_t size)
{
	return (n + sizeof(void*) -1) & ~(sizeof(void*) -1);
}

int heap_init() 
{
	uint64_t end;
	private_heap.tail = private_heap.head = (mem_chunk_t*) k_sbrk(0);
	
	end = k_sbrk(HEAP_MIN_SIZE);

	if (!end) return 0;

	private_heap.head->size = HEAP_MIN_SIZE + PAGE_UP( ((end+1) - (uint64_t)private_heap.head) );
	private_heap.head->used = 0x0;
	private_heap.head->next = 0x0;
	
	shared_heap.tail = shared_heap.head = (mem_chunk_t*) shared_sbrk(0);
	

	end = shared_sbrk(HEAP_MIN_SIZE);

	if (!end) return 0;

	shared_heap.head->size = HEAP_MIN_SIZE + PAGE_UP( ((end+1) - (uint64_t)shared_heap.head) );
	shared_heap.head->used = 0x0;
	shared_heap.head->next = 0x0;
	
	return 1;	
}

void *_malloc(struct group_heap *heap, uint64_t size, int shared)
{
	uint64_t alloc_size = 0;
	mem_chunk_t *tmp = heap->head;

	if (size == 0) return (void*)0x0;	
	alloc_size = size + sizeof(mem_chunk_t);	
	alloc_size = align(alloc_size, sizeof(void*));

	//search for memory	
	//spin_lock(&kmalloc_lock);	
	while(tmp) { //first fit
		if ((tmp->used == 0x0) && (tmp->size >= alloc_size)) {
			break;
		}
		tmp = tmp->next;
	}

	//do we need to split? 
	if (tmp) {
		if (tmp->size > (alloc_size + MIN_HEAP_ALLOC)) {
			if (!split(tmp, size)) return 0;
		}
	}

	if (!tmp) {
		if (shared) tmp = (mem_chunk_t*) shared_sbrk(alloc_size);
		else tmp = (mem_chunk_t*) k_sbrk(alloc_size);
		//spin_lock(&malloc_lock);	
		if (!tmp) return 0x0;
		tmp->size = alloc_size;
		tmp->next = 0x0;
		tmp->used = 0x0;
		tmp->address = 0x0;
		heap->tail = tmp;
		//spin_lock(&malloc_lock);	
		goto success;
	}


success:
	tmp->used = 0x1;
	//spin_lock(&malloc_lock);	

	tmp->address = (uint64_t) (((uint64_t)tmp) + sizeof(mem_chunk_t));
	if ( ((uint64_t)tmp) == ((uint64_t)heap->tail) ) {
		heap->tail = (((uint64_t)tmp->next) == 0) ? tmp: tmp->next;
	}

	return (void*) tmp->address;
}

void *smalloc(uint64_t size)
{
	return _malloc(&shared_heap, size, 1);
}

void *kmalloc(uint64_t size)
{
	return _malloc(&private_heap, size, 0);
}

void *scalloc(uint64_t nmeb, uint64_t count)
{
	uint64_t alloc_size = nmeb * count;
	uint8_t *ptr;

	if (alloc_size == 0 ) return 0x0;	

	ptr = smalloc(alloc_size);
	if (!ptr) return 0x0;

	memset(ptr, 0, alloc_size);

	return (void*)ptr;
}

void *kcalloc(uint64_t nmeb, uint64_t count)
{
	uint64_t alloc_size = nmeb * count;
	uint8_t *ptr;

	if (alloc_size == 0 ) return 0x0;	

	ptr = kmalloc(alloc_size);
	if (!ptr) return 0x0;

	memset(ptr, 0, alloc_size);

	return (void*)ptr;
}

void _free(struct group_heap *heap, void *addr) 
{
	mem_chunk_t *ptr = heap->head;
	mem_chunk_t *prev = heap->head;

	//spin_lock(&malloc_lock);	
	while (ptr) {
		if (ptr->address == (uint64_t)addr)
			break;
		prev = ptr;
		ptr = ptr->next;
	}
//coalesce on free
	if (ptr) {
		if ( ptr->next && ptr->next->used == 0 ) {
			ptr->size += ptr->next->size;
			//update last to avoid segfault
			ptr->next = ptr->next->next;
		}

		if ( (ptr != prev) && (prev->used == 0) ) {
			prev->size += ptr->size;
			prev->address = 0x0;
			prev->next = ptr->next;
		}
	}
	//spin_unlock(&malloc_lock);	
}

void sfree(void *addr)
{
	_free(&shared_heap, addr);
}

void kfree(void *addr)
{
	_free(&private_heap, addr);
}

int split(mem_chunk_t *ptr, uint64_t start_size)
{
	mem_chunk_t *tmp;
	if (ptr->used) return 0;	

	uint64_t total_size = 2 * sizeof(mem_chunk_t);
	total_size = align(total_size, sizeof(void*)) + start_size;
	
	if (ptr->size <= total_size) return 0;
	
	start_size = start_size + sizeof(mem_chunk_t);
	start_size = align(start_size, sizeof(void*));

	tmp = (mem_chunk_t*) (((uint64_t)ptr) + start_size);

	tmp->size = ptr->size - start_size;
	tmp->next = ptr->next;
	tmp->used = 0x0;

	ptr->size = start_size;
	ptr->next = tmp;

	return 1;	
}
