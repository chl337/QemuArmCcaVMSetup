#include "defines.h"
#include "common.h"
#include "vma.h"
#include "kernel.h"
#include "errno.h"
#include "process.h"
#include "heap.h"
#include "vma.h"
#include <stdbool.h>

extern char *_start;
extern char *_sstack;
//extern char *_stack_base;


uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Lower_PT3[PT3_QUAD_SIZE]; //can address 6
uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Lower_PGD[PGD_QUAD_SIZE];
uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Lower_PUD[PUD_QUAD_SIZE];
uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Lower_PMD[PMD_QUAD_SIZE];
                                                                                    
                                                                                    
uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Realm_PT3[PT3_QUAD_SIZE];
uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Realm_PGD[PGD_QUAD_SIZE];
uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Realm_PUD[PUD_QUAD_SIZE];
uint64_t __attribute__((aligned(4096)/* */ )) __attribute__((section(".page_tables")))  _Realm_PMD[PMD_QUAD_SIZE];


// granularity
#define PT_PAGE     0b11        // 4k granule
#define PT_BLOCK    0b01        // 2M granule
				// accessibility
#define PT_KERNEL   (0<<6)      // privileged, supervisor EL1 access only
#define PT_USER     (1<<6)      // unprivileged, EL0 access allowed
#define PT_RW       (0<<7)      // read-write
#define PT_RO       (1<<7)      // read-only
#define PT_AF       (1<<10)     // accessed flag
#define PT_NX       (1UL<<54)   // no execute
				// shareability
#define PT_OSH      (2<<8)      // outter shareable
#define PT_ISH      (3<<8)      // inner shareable
				// defined in MAIR register
#define PT_MEM      (0<<2)      // normal memory
#define PT_DEV      (1<<2)      // device MMIO
#define PT_NC       (2<<2)      // non-cachable


#define TTBR_CNP    1

#define PROCESS_PAGE_TABLE_COUNT_L3 4 

int perm_to_pt( int prot) {
	int pt = 0;
	pt |= ((PROT_X & prot)?  0 : PT_NX);
	if (PROT_RW == prot || PROT_RWX == prot || PROT_W & prot) {
		pt = pt | PT_RW;
	} else if (PROT_R & prot) 
		pt = pt | PT_RO;
	return pt;
} 

void set_lower_translation_tables(TranslationTables_t *table)
{
	table->PGD = _Lower_PGD;
	table->PUD = _Lower_PUD;
	table->PMD = _Lower_PMD;
	table->PT3 = _Lower_PT3;
}

void set_upper_translation_tables(TranslationTables_t *table)
{
	table->PGD = _Realm_PGD;
	table->PUD = _Realm_PUD;
	table->PMD = _Realm_PMD;
	table->PT3 = _Realm_PT3;
}

void create_user_translation_table(TranslationTables_t *t)
{
	int prot = PROT_RW;
	mem_region_t *region;

	t->PGD  = (void*) kframe_allocate_secure(1); // 0x1000
	t->PUD  = (void*) kframe_allocate_secure(1); // 0x1000 entire process in 64GB 
	t->PMD  = (void*) kframe_allocate_secure(1); // 0x1000 need only 64 pmds
	t->PT3  = (void*) kframe_allocate_secure(4);//need only  

	if ( (t->PGD == (void*)(-1UL)) || (t->PUD ==(void*)(-1UL)) ||  
	     (t->PMD == (void*)(-1UL)) || (t->PT3 == (void*)(-1UL)) )
		crash();

	region = kernel_vma_alloc(0, 8, 0);
	if (!region) crash();

	uint64_t pa = 0;  
	uint64_t va = region->va_base; 
	page_table_map_kernel(&kernel_struct.secure_t, region->va_base , (uint64_t) t->PGD, prot);
	page_table_map_kernel(&kernel_struct.secure_t, region->va_base + 0x1000, (uint64_t) t->PUD, prot);
	page_table_map_kernel(&kernel_struct.secure_t, region->va_base + 0x2000 , (uint64_t) t->PMD, prot);

	pa = (uint64_t) t->PT3;
	va = region->va_base + 0x3000;
	for( uint64_t count = 0; count < 4; count++) {
		page_table_map_kernel(&kernel_struct.secure_t, va, pa , PROT_RW);
		va += PAGE_SIZE_4KB;
		pa += PAGE_SIZE_4KB;
	}

	t->va_PGD  = region->va_base + 0x0; 
	t->va_PUD  = region->va_base + 0x1000; 
	t->va_PMD  = region->va_base + 0x2000; 
	t->va_PT3  = region->va_base + 0x3000; 

};

void normal_map_kernel_4KB(TranslationTables_t *table, uint64_t ram_size, uint64_t ram_shared_size)
{
	uint64_t offset = 0;
	uint64_t id_vma = table->start_va;

	uint64_t *l0 = table->PGD;
	uint64_t *l1 = table->PUD;
	uint64_t *l2 = table->PMD;
	uint64_t *l3 = table->PT3;

	uint64_t count = 0;

	//connect
	union TABLE_FORMAT_4KB_48_OA pgd_entry = {0};
	union TABLE_FORMAT_4KB_48_OA pud_entry = {0};
	union TABLE_FORMAT_4KB_48_OA pmd_entry = {0};
	union block_page_descriptor_4KB_48_OA  pt3_entry = {0};


	//only 1 pgd
	pgd_entry.is_valid = 1;
	pgd_entry.is_link  = 1;
	pgd_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(table->PUD);

	offset = VA_OFFSET_48_L0(id_vma); 
	l0[offset] = (uint64_t)pgd_entry.reg;

	//1 pud --> can address 512GB

	pud_entry.is_valid = 1;
	pud_entry.is_link  = 1;
	pud_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(table->PMD);
	offset = VA_OFFSET_48_L1(id_vma);
	l1[offset] = (uint64_t)pud_entry.reg;

	count = (ram_size + (2 * 1024 * 1024) - 1)/ (2 * 1024 * 1024);
	uint64_t i = 0;

	do {
		pmd_entry.is_valid = 1;
		pmd_entry.is_link  = 1;
		pmd_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(&table->PT3[i*512]);
		offset = VA_OFFSET_48_L2(id_vma);
		l2[offset] = (uint64_t)pmd_entry.reg; 
		id_vma += (2 * 1024 * 1024);
	} while (i++ <= count);
	/*map first 2MB in 4KB pages
	 *one pt3 page will be used*/
	uint64_t page_va = table->start_va;
	uint64_t page_pa = table->start_pa;
	uint64_t flags = 0;
	count = 0;

	for(i=0; i < ram_size/PAGE_SIZE_4KB; i++) {

		pt3_entry.l3.is_valid = 1;
		pt3_entry.l3.is_link  = 0;
		pt3_entry.l3.oa = BD_OA_l3_get(page_pa);

		flags = (PT_PAGE | PT_KERNEL| PT_RW | PT_ISH | PT_AF | PT_MEM);

		pt3_entry.reg = pt3_entry.reg  | flags; 

		offset = VA_OFFSET_48_L3(page_va);
		l3[offset + count] = pt3_entry.reg;
		page_va = page_va + PAGE_SIZE_4KB;
		page_pa = page_pa + PAGE_SIZE_4KB;
		count = count + ((offset == 511) ? 512: 0);
	}
}

void identity_map_kernel_4KB(uint64_t ram_size, uint64_t ram_shared_size, uint64_t start_pa, uint64_t start_va)
{
	uint64_t offset = 0;
	uint64_t id_vma = start_va;

	uint64_t *l0 = _Lower_PGD;
	uint64_t *l1 = _Lower_PUD;
	uint64_t *l2 = _Lower_PMD;
	uint64_t *l3 = _Lower_PT3;


	//connect
	union TABLE_FORMAT_4KB_48_OA pgd_entry = {0};
	union TABLE_FORMAT_4KB_48_OA pud_entry = {0};
	union TABLE_FORMAT_4KB_48_OA pmd_entry = {0};
	union block_page_descriptor_4KB_48_OA  pt3_entry = {0};


	//only 1 pgd
	pgd_entry.is_valid = 1;
	pgd_entry.is_link  = 1;
	pgd_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(_Lower_PUD);

	offset = VA_OFFSET_48_L0(id_vma); 
	l0[offset] = (uint64_t)pgd_entry.reg;

	//1 pud --> can address 512GB

	pud_entry.is_valid = 1;
	pud_entry.is_link  = 1;
	pud_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(_Lower_PMD);
	offset = VA_OFFSET_48_L1(id_vma);
	l1[offset] = (uint64_t)pud_entry.reg;

	pmd_entry.is_valid = 1;
	pmd_entry.is_link  = 1;
	pmd_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(_Lower_PT3);
	offset = VA_OFFSET_48_L2(id_vma);
	l2[offset] = (uint64_t)pmd_entry.reg; 

	/*map first 2MB in 4KB pages
	 *one pt3 page will be used*/
	uint64_t page_va=start_va;
	uint64_t page_pa=start_pa;
	uint64_t flags = 0;
	for(int i=0; i < 512; i++) {

		pt3_entry.l3.is_valid = 1;
		pt3_entry.l3.is_link  = 0;
		pt3_entry.l3.oa = BD_OA_l3_get(page_pa);

		flags = (PT_PAGE | PT_KERNEL| PT_RW | PT_ISH | PT_AF | PT_MEM);

		pt3_entry.reg = pt3_entry.reg  | flags; 

		offset = VA_OFFSET_48_L3(page_va);
		l3[offset] = pt3_entry.reg;
		page_va = page_va + PAGE_SIZE_4KB;
		page_pa = page_pa + PAGE_SIZE_4KB;
	}

}

void identity_map_kernel_2MB(uint64_t ram_size, uint64_t ram_shared_size)
{
	uint64_t offset = 0;
	uint64_t ram_address = (uint64_t) &_start;
	uint64_t id_vma = ram_address;

	uint64_t *l0 = _Lower_PGD;
	uint64_t *l1 = _Lower_PUD;
	uint64_t *l2 = _Lower_PMD;


	//connect
	union TABLE_FORMAT_4KB_48_OA pgd_entry = {0};
	union TABLE_FORMAT_4KB_48_OA pud_entry = {0};
	union block_page_descriptor_4KB_48_OA  pmd_entry = {0};


	//only 1 pgd
	pgd_entry.is_valid = 1;
	pgd_entry.is_link  = 1;
	pgd_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(_Lower_PUD);

	offset = VA_OFFSET_48_L0(id_vma); 
	l0[offset] = (uint64_t)pgd_entry.reg;

	//1 pud --> can address 512GB

	pud_entry.is_valid = 1;
	pud_entry.is_link  = 1;
	pud_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(_Lower_PMD);
	offset = VA_OFFSET_48_L1(id_vma);
	l1[offset] = (uint64_t)pud_entry.reg;

	pmd_entry.l2.is_valid = 1;
	pmd_entry.l2.is_link  = 0;
	pmd_entry.l2.oa = BD_OA_l2_get(id_vma);
	offset = VA_OFFSET_48_L2(id_vma);

	pmd_entry.reg = pmd_entry.reg  | (PT_BLOCK | PT_KERNEL| PT_RW | PT_ISH | PT_AF | PT_MEM);
	l2[offset] = (uint64_t)pmd_entry.reg; 
}

void configure_translation()
{
	//union TCR_EL1_table config = {0};

	//configure MAIR register
	//memory attributes
	uint64_t  mair_conf = 0;
	mair_conf = (0xff << 0 ) | //attr index 0: normal, IWBWA, OWMBWA, NTR
		(0x04 << 8 ) | //attr index 1: device, nGnRE (bust be outer shareable)
		(0x44 << 16);  //attr index=2, non-cacheable

	asm volatile("msr mair_el1, %0" :: "r"(mair_conf));

	asm volatile ("mrs %0, tcr_el1; isb":"=r"(mair_conf));

	mair_conf |= (0b00LL << 37) | // TBI=0, no tagging
		(0b100LL << 32) |      // IPS=autodetected
		(0b10LL << 30) | // TG1=4k
		(0b11LL << 28) | // SH1=3 inner
		(0b01LL << 26) | // ORGN1=1 write back
		(0b01LL << 24) | // IRGN1=1 write back
		(0b0LL  << 23) | // EPD1 enable higher half
		(16LL   << 16) | // T1SZ=16, 3 levels (512G)
		(0b00LL << 14) | // TG0=4k
		(0b11LL << 12) | // SH0=3 inner
		(0b01LL << 10) | // ORGN0=1 write back
		(0b01LL << 8) |  // IRGN0=1 write back
		(0b0LL  << 7) |  // EPD0 enable lower half
		(16LL   << 0);   // T0SZ=16, 3 levels (512G)

	asm volatile ("msr tcr_el1, %0; isb"::"r"(mair_conf));

	//upper half
	asm volatile("msr ttbr0_el1, %0"::"r"(_Lower_PGD));
	//lower half
	asm volatile("msr ttbr1_el1, %0"::"r"(_Realm_PGD));

};

/*0 is error, otherwise success*/

int kframe_allocator_init(kframe_alloc_t *fa, uint64_t bitmap_frame, uint64_t bitmap_size, uint64_t frame_start, uint64_t size)
{
	if (size == 0 || !fa) return 0;

	fa->frame_start  = frame_start;
	fa->frame_count  = size / KFRAME_SIZE;
	fa->bitmap_frame = (uint8_t*) bitmap_frame;
	fa->available_frame_count = size / PAGE_SIZE_4KB;
	fa->bitmap_count = bitmap_size; /*each frame corresponds to a byte*/

	for(uint64_t i=0; i < fa->bitmap_count; i++) {
		fa->bitmap_frame[i] = KFRAME_FREE;
	}

	//spin_unlock(&frame_lock);
	return 1;
}
uint64_t kframe_allocate_single_frame(kframe_alloc_t *fa)
{
	uint64_t start = 0;
	if (!fa) goto kfa_error;
	if (fa->available_frame_count == 0) return 0;
	//spin_lock(&frame_lock);
	for(; start < fa->frame_count; start++) {
		if( fa->bitmap_frame[start] == KFRAME_FREE) {
			fa->available_frame_count--;
			fa->bitmap_frame[start] = KFRAME_USED;
			//spin_unlock(&frame_lock);
			return ((uint64_t)fa->frame_start) + (start  * KFRAME_SIZE);
		}
	}
	//spin_unlock(&frame_lock);
kfa_error:
	return (uint64_t)(-1);
}

void kframe_free(kframe_alloc_t *fa, uint64_t address) 
{
	uint64_t index;
	if (!fa) return;
	index = (address - fa->frame_start)/ KFRAME_SIZE;
	if ( (address < fa->frame_start ) ||
	     (address > (fa->frame_start + (fa->frame_count * KFRAME_SIZE))) )
		return;

	//the cache is full, look for it in the frame
	fa->bitmap_frame[index] = KFRAME_FREE;
	fa->available_frame_count++;
	return;
}

uint64_t kframe_allocate_fixed(kframe_alloc_t *fa, uint64_t start, uint64_t count)
{
	uint64_t index;
	uint64_t end;

	if (!fa) return (-1ULL);

	end = (fa->frame_start + (fa->frame_count*KFRAME_SIZE));

	if ((start < fa->frame_start) || (end < (start + (count*KFRAME_SIZE))))
		return  (-1ULL);

	index = (start - (uint64_t)fa->frame_start) / KFRAME_SIZE;

	//spin_lock(&fa->frame_lock);
	for (uint64_t i=0; i < count; i++) {
		if (fa->bitmap_frame[index + i] == KFRAME_USED) {
			//spin_unlock(&fa->frame_lock)
			return (-1ULL);
		}
	}
	//available
	for (uint64_t i=0; i < count; i++ ) {
		fa->bitmap_frame[index + i] = KFRAME_USED; 
		fa->available_frame_count--;
	}
	//spin_unlock(&fa->frame_lock);
	return start;
}

uint64_t kframe_allocate_range(kframe_alloc_t *fa, uint64_t count)
{
	uint64_t target = (-1ULL);
	uint64_t l_count = 0;
	uint64_t i = 0;
	if (!fa) return (-1ULL);

	if (count > fa->available_frame_count) return (-1ULL);

	//search the frame bitmap
	for (i=0, l_count=0; i < fa->frame_count; i++) {
		if (fa->bitmap_frame[i] == KFRAME_FREE) {
			target = (target == (-1ULL)) ? (i*KFRAME_SIZE + fa->frame_start): target;
			l_count++;
		} else {
			target = (-1ULL);
			l_count = 0;
		}
		if (l_count == count)
			break;
	}
	if (l_count == count) {
		for(i=0, l_count = (target - fa->frame_start) /KFRAME_SIZE; i<count; i++) {
			fa->bitmap_frame[l_count + i] = KFRAME_USED;
			fa->available_frame_count--;
		}
	}
	//spin_unlock(&fa->frame_lock);
	return target;
}

uint64_t kframe_allocate_fixed_insecure(uint64_t start, uint64_t count)
{
	return kframe_allocate_fixed(&kernel_struct.insecure_frames, start, count);
}

uint64_t kframe_allocate_insecure(uint64_t start, uint64_t count)
{
	return kframe_allocate_range(&kernel_struct.insecure_frames, count);
}

int kframe_allocator_init_insecure(uint64_t bitmap_frame, uint64_t bitmap_count, uint64_t frame_start, uint64_t size)
{
	return kframe_allocator_init(&kernel_struct.insecure_frames, bitmap_frame,bitmap_count, frame_start, size);
}
void kframe_free_insecure(uint64_t address) 
{
	kframe_free(&kernel_struct.insecure_frames, address);
}

uint64_t kframe_allocate_fixed_secure(uint64_t start, uint64_t count)
{
	return kframe_allocate_fixed(&kernel_struct.secure_frames, start, count);
}

uint64_t kframe_allocate_secure(uint64_t count)
{
	return kframe_allocate_range(&kernel_struct.secure_frames, count);
}

int kframe_allocator_init_secure(uint64_t bitmap_frame, uint64_t bitmap_count, uint64_t frame_start, uint64_t size)
{
	return kframe_allocator_init(&kernel_struct.secure_frames, bitmap_frame, bitmap_count, frame_start, size);
}

void kframe_free_secure(uint64_t address) 
{
	kframe_free(&kernel_struct.secure_frames, address);
}

bool address_in_mmap_region(mem_region_t *src, uint64_t address)
{
	if (!src) return false;
	return  (address >= src->va_base && address <= (src->va_base + src->va_size));
}

void *split_mmap_region(mem_region_t *src, uint64_t address)
{
	mem_region_t *new;
	if (!src) return (void*)0;

	new = kmalloc(sizeof(mem_region_t));
	if (!new) return (void*)0;
	memcpy(new, src, sizeof(mem_region_t));
	
	new->va_size = address - new->va_base; 
	new->va_base = address;
	src->va_size -= new->va_size;

	return new;
}

/*
* void *search_mmap_region(mem_region_t *region, uint64_t size);
* search the passed mem_region_t linked list for a region with the given
* size. return the vma region if it exists, else return 0
*/
void *search_mmap_region(mem_region_t * region, uint64_t size)
{
	mem_region_t *head = region;
	mem_region_t *next;
	uint64_t range;

	if (head == 0)  return 0;
	next = head->next;
	
	if (next == 0 ) {
		return (void*)head;	
	}
		
	for(;next;) {
		if (head->va_size >= size)
			return (void*) head;	
		head = next;
		next = next->next;
	}
	
	return (void*) 0;
}
/*
* void *search_mmap_region(mem_region_t *region, uint64_t address, uint64_t size);
* search the passed mem_region_t linked list for a region with the given
* size. return the vma region if it exists, else return 0
*/

void *search_mmap_region_fixed(mem_region_t * region, uint64_t address, uint64_t size)
{
	mem_region_t *head = region;	
	uint64_t range = 0;
	uint64_t request_end = address + size;

	if (head == 0) 
		return (void*) 0;

	range = head->va_base + head->va_size;
	if (range >= request_end || (address < range && address >= head->va_base))
		return (void*) head;
	//assume the mmap_region are added linearly
	for(;head->next; head=head->next) {
		range = head->va_base + head->va_size;
		if (address >= range && address < head->next->va_base ) {
			if ((request_end) < head->next->va_base ) //fits between current and next
				return (void*) head;
			return (void*)head; //exists but used up
		}
	}
	
	range = head->va_base + head->va_size;
	if (request_end <= range)
		return (void*)head;
	return 0;
}

int mmap_region_insert(mem_region_t **head, mem_region_t *to_insert)
{
	mem_region_t *cptr,*prev;
	uint64_t cur;
	uint64_t insert_end;
 
	if (!*head) {
		*head = to_insert;
		return 1;
	}

	insert_end  = to_insert->va_base + to_insert->va_size;
	prev = cptr = *head;

	while(cptr->next) {
		cur = cptr->va_base + cptr->va_size;

		if (cptr->va_base > to_insert->va_base ) {
			if (*head == cptr) {
				to_insert->next = cptr;
				*head = to_insert;
				return 1;
			}
			to_insert->next = prev->next;
			prev->next = to_insert;
			return 1;
		}
		
		if (to_insert->va_base >= cur && insert_end < cptr->next->va_base) {
			to_insert->next = cptr->next;
			cptr->next = to_insert;
			return 1;
		}
		prev = cptr;
		cptr = cptr->next;
	}
	
	cur = cptr->va_base + cptr->va_size;

	if (cur <= to_insert->va_base) {
		cptr->next = to_insert;
		return 1;
	}
	return 0;	
}

void * generic_mmap(TranslationTables_t *t, mem_region_t* head , int is_kernel, int is_secure, void *address, uint64_t size, int prot, int flags, int fd, int64_t offset)
{
	int fixed = 0;
	void *search;
 	mem_region_t *region = head;
	uint64_t frame, ret; 

	if (size == 0 || head == 0) return (void*) MAP_FAILED;
	
	size = PAGE_UP(size);
		
	if (flags & MAP_FIXED) fixed = 1;

	if (head->is_mapped == 0 ) {
		region = head;
		goto start_mapping;
	}
	
	
	if (fixed) {
		search = search_mmap_region_fixed(head, (size_t)address, size);	
		if (search == (void*) MAP_FAILED) 
			return (void*) MAP_FAILED;
	} else {
		search = search_mmap_region(head, size);
	}

	if(search == 0) 
		search = address;
	region = (mem_region_t*) kcalloc(sizeof(struct kernel_struct), 1);
	if (!region) return (void*)MAP_FAILED;

start_mapping:
	region->is_kernel = is_kernel;
	region->is_secure = is_secure;
	region->va_size = size;
	region->perm = prot;
	if (head->is_mapped == 0 )
		region->va_base =  fixed ? (uint64_t)address : (uint64_t)kernel_struct.mmap_start; 
	else
		region->va_base = (uint64_t) search;

	region->is_mapped = 1;
	
	if (is_secure) {
		for (uint64_t count = 0; count < size; count+=PAGE_SIZE_4KB) {	
			frame = kframe_allocate_secure(1);
			if (frame == (uint64_t)(-1)) 
				crash();
			ret = page_table_map(t, region->va_base + count, frame, prot, 0);
			if (ret != 0 ) 
				crash();
		}
	} else {
		for (uint64_t count = 0; count < size; count+=PAGE_SIZE_4KB) {	
			frame = kframe_allocate_secure(1);
			if (frame == (uint64_t)(-1)) 
				crash();
			ret = page_table_map(t, region->va_base + count, frame, prot, 0);
			if (ret != 0 ) 
				crash();
		}
	}

	if (region !=  head)
		mmap_region_insert(&head->next, region);
	return (void*)region->va_base;
}

void * kernel_mmap_insecure( void *address, uint64_t size, int prot, int flags, int fd, int64_t offset)
{
	TranslationTables_t *t = &kernel_struct.secure_t;
	int new_prot = perm_to_pt(prot);
	new_prot  |= PT_KERNEL;

	return generic_mmap(t, &kernel_struct.mmap_region_head, 1, 0, address, size, new_prot, flags, fd, offset);
}

void * kernel_mmap( void *address, uint64_t size, int prot, int flags, int fd, int64_t offset)
{
	TranslationTables_t *t = &kernel_struct.secure_t;
	int new_prot = perm_to_pt(prot);
	new_prot  |= PT_KERNEL;

	return generic_mmap(t, &kernel_struct.mmap_region_head, 1, 1, address, size, new_prot, flags, fd, offset);
}

void * sys_mmap_user(TranslationTables_t *t, mem_region_t *region, void *address, uint64_t size, int prot, int flags, int fd, int64_t offset)
{
	int new_prot = perm_to_pt(prot);
	new_prot  |= PT_USER;

	return generic_mmap(t, region , 0, 1,  address, size, new_prot, flags, fd, offset);
}

bool can_extend_vma(mem_region_t *src, uint64_t final_size)
{
	if (!src) return false;
	if (src->next == 0) return true;
	if (src->next->is_mapped == 1) return false; 

	if (!(final_size + src->va_base))
		return true;
	if (src->va_base + final_size <= (src->next->va_base + src->next->va_size))
		return true;
	return false;
}

mem_region_t *extend_vma(mem_region_t *target, uint64_t final_size) 
{
	uint64_t extension;
	if (!target) return  (mem_region_t*)0;
	if (target->va_size >= final_size) return target;

	if (target->next) {
		if( (target->next->va_size + target->va_size) <= final_size) {
			extension = final_size - target->va_size;
			target->next->va_size -= extension; 
			target->next->va_base += extension;
			target->va_size += extension;
		}
		return (mem_region_t*)0;
	}

	target->va_size = final_size;
	return target;
}

mem_region_t *coalesce_vma(mem_region_t *start)
{
	mem_region_t *next;
	if (!start) return (mem_region_t*)0;
	if (!start->next) return start;
	next = start->next;
	start->va_size = (next->va_base + next->va_size) - start->va_base;
	start->next = next->next;
	kfree(next);
	return start;
}

mem_region_t *split_vma(mem_region_t *cwr, uint64_t address, uint64_t final_size)
{
	mem_region_t *second;
	if (!cwr) return (mem_region_t*)0;

	if (cwr->va_base > address) return (mem_region_t*)0;
	if (cwr->va_size <= final_size) return cwr;
	
	if (cwr->va_base == address) {
		second = kmalloc(sizeof(mem_region_t));
		if (!second) return (mem_region_t*)0;
		memcpy(second,cwr, sizeof(mem_region_t));
		cwr->next = second;
		cwr->va_size = final_size;
		second->va_size -= final_size;
		second->va_base = address + final_size;
		return cwr;
	}
	
	if (cwr->va_base < address) {
		//check if we need 2 splits
		if( (address + final_size) == (cwr->va_base + cwr->va_size)) { //single split

			second = kmalloc(sizeof(mem_region_t));
			if (!second) return (mem_region_t*)0;
			memcpy(second,cwr, sizeof(mem_region_t));
			cwr->next = second;
			second->va_size = final_size;
			second->va_base = address;
			cwr->va_size -= final_size;
			return second;
		}  else { //2 splits <first><second><third>
			second = split_mmap_region(cwr, address);
			if (!second) return (mem_region_t*)0;
			uint64_t new_address = address + final_size;
			mem_region_t *third = split_mmap_region(second, new_address);
			if (!third) {
				coalesce_vma(cwr);
				return (mem_region_t*)0;
			}
			return third;	
		}
	}

	return (mem_region_t*)0;
}

mem_region_t *user_vma_alloc(mem_region_t *head, void *address, uint64_t page_count, int flags, int perm)
{
	mem_region_t *new_region;
	uint64_t request_size = (page_count *PAGE_SIZE_4KB);
	uint64_t target;
	uint64_t size;
	mem_region_t *location = 0;
	int split=0;

	if (head == 0) goto allocate_new;

//search for a given region if it exists 
// or where to place it
	coalesce_vma(head);	
	if (flags == MAP_FIXED)
		location = search_mmap_region_fixed(head, (uint64_t)address, request_size);
	else
		location = search_mmap_region(head, request_size);

	if (location) {
		if (location->is_mapped) 
			return (mem_region_t*) location;	
		if (location->va_size == request_size) return location;
	}

allocate_new:
	new_region = (mem_region_t*) kcalloc(sizeof(mem_region_t), 1);	
	if (new_region == 0) return 0;
		
	new_region->is_mapped = 0;
	new_region->is_kernel = 0;
	new_region->perm = perm;
	new_region->is_page_table = 0;	

	if (!location && location->va_size > request_size) goto req_size_is_less;
	if (!location && location->va_size < request_size) goto req_size_is_larger;

	new_region->va_base = target;
	new_region->va_size = page_count * PAGE_SIZE_4KB;
	return new_region;

req_size_is_larger:
	if ( can_extend_vma(location, request_size) ) {
		location = extend_vma(location, request_size);
		kfree(new_region);	
		return location;
	}
	kfree(new_region);
	return (void*)0;

req_size_is_less:
	kfree(new_region);
	mem_region_t *new = split_vma(location, address, request_size);
	new->is_mapped = 0;
	new->is_kernel = 0;
	new->perm = perm;
	new->is_page_table = 0;
	return new;
}

mem_region_t *kernel_vma_alloc(void *address, uint64_t page_count, int flags)
{
	uint64_t target;
	uint64_t request_size = (page_count *PAGE_SIZE_4KB);
	mem_region_t *new_region;
	if (kernel_struct.mmap_region_head.is_mapped) {
		if (flags == MAP_FIXED) 
			target = (uint64_t) search_mmap_region_fixed(&kernel_struct.mmap_region_head, (uint64_t)address, request_size);
		else
			target = (uint64_t) search_mmap_region(&kernel_struct.mmap_region_head, request_size);
		if (target == 0 || target == (uint64_t)(-1)) 
			return 0;
	} else {
		new_region = (mem_region_t*) kcalloc(sizeof(mem_region_t), 1);	
		if (new_region == 0) return 0;
		memcpy(new_region, &kernel_struct.mmap_region_head, sizeof(mem_region_t));

		kernel_struct.mmap_region_head.next = new_region;
		if (flags == MAP_FIXED) {
			if ((uint64_t)address > kernel_struct.mmap_region_head.va_base) {
				new_region->va_base = (uint64_t)address;
				new_region->va_size = request_size;
				kernel_struct.mmap_region_head.va_size -= request_size;
			} else if ((uint64_t)address == kernel_struct.mmap_region_head.va_base) {
				kernel_struct.mmap_region_head.va_size = request_size;	
				new_region->va_size -= request_size;
				return &kernel_struct.mmap_region_head;
			} else {
				return 0;
			}
		} else {
			kernel_struct.mmap_region_head.va_size = request_size;
			new_region->va_size -= request_size;
			new_region->va_base = kernel_struct.mmap_region_head.va_base + request_size;
			return &kernel_struct.mmap_region_head;
		}
	}

	new_region = (mem_region_t*) kcalloc(sizeof(mem_region_t), 1);	
	if (new_region == 0) return 0;
	
	new_region->is_mapped = 1;
	new_region->is_kernel = 1;
	new_region->va_base = target;
	new_region->va_size = page_count * PAGE_SIZE_4KB;
	new_region->perm = PROT_R | PROT_W;
	new_region->is_page_table = 1;

	return new_region;
}

int page_table_map(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags, int is_dev)
{		
	union TABLE_FORMAT_4KB_48_OA pgd_entry = {0};
	union TABLE_FORMAT_4KB_48_OA pud_entry = {0};
	union TABLE_FORMAT_4KB_48_OA pmd_entry = {0};
	union block_page_descriptor_4KB_48_OA  pt3_entry = {0};

	uint64_t *l0 = t->va_PGD;
	uint64_t *l1 = t->va_PUD;
	uint64_t *l2 = t->va_PMD;
	uint64_t *l3 = t->va_PT3;

	uint64_t offset = 0;	
	uint64_t index = 0;
	uint64_t which_2mb = 0;
	uint64_t which_1gb = 0;
	uint64_t which_512gb = 0;

	//check if entry  of va is in table	
	index =  va - t->start_va;

	which_512gb = index / (512 * 0x400 * 0x400 * 0x400);//which 512GB
	which_1gb   = index / (0x400 * 0x400 * 0x400); //which 1 GB
	which_2mb   = index / (0x2 * 0x400 * 0x400); //which 2MB
  
	if (which_512gb > 0) crash();

	offset = VA_OFFSET_48_L0(va);
	if (offset > 512) return -1;

	index = which_512gb * PAGE_SIZE_4KB;
	pgd_entry.reg = l0[offset];
	if (!pgd_entry.is_valid) {
		pgd_entry.is_valid = 1;
		pgd_entry.is_link = 1;
		pgd_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS((((uint64_t)t->PUD) + index ));
		l0[offset] = (uint64_t) pgd_entry.reg;
	}
	
	offset = VA_OFFSET_48_L1(va);
	pud_entry.reg = l1[offset];
	index = which_1gb * PAGE_SIZE_4KB; //selects which 4kb page in l2 to use

	if (!pud_entry.is_valid) {
		pud_entry.is_valid = 1;
		pud_entry.is_link  = 1;
		pud_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS(( ((uint64_t)t->PMD) + index));
		offset = VA_OFFSET_48_L1(va);
		l1[offset] = (uint64_t)pud_entry.reg;
	}

	offset = VA_OFFSET_48_L2(va);
	index = which_2mb * PAGE_SIZE_4KB;

	pmd_entry.reg = l2[offset + (which_1gb * 512)];//which entry 
	if (!pmd_entry.is_valid) {
		pmd_entry.is_valid = 1;
		pmd_entry.is_link  = 1;
		pmd_entry.next_level_table_address = MASK_VA_36_TABLE_ADDRESS((((uint64_t)t->PT3) + index));//correct table
		offset = VA_OFFSET_48_L2(va);
		l2[offset + ((which_1gb * 512)/PAGE_SIZE_4KB)] = (uint64_t)pmd_entry.reg; 
	}

	pt3_entry.l3.is_valid = 1;
	pt3_entry.l3.is_link  = 0;
	pt3_entry.l3.oa = BD_OA_l3_get(pa);
	
	uint64_t of = PT_PAGE | PT_ISH  | flags;
        //NOTE: MMIO Region flags
	of |= (is_dev == 1) ? (PT_NX | PT_DEV | PT_AF): (PT_MEM | PT_AF );
	
	pt3_entry.reg = pt3_entry.reg  | of; 
	offset = VA_OFFSET_48_L3(va);
	l3[offset + (which_2mb*512)] = pt3_entry.reg;

	return 0;
}

int page_table_map_user(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags)
{
	int new_flags = perm_to_pt(flags);
	new_flags  |= PT_USER;

	return page_table_map(t, va, pa, new_flags, 0);	
}

int page_table_map_kernel(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags)
{
	int new_flags = perm_to_pt(flags);
	new_flags  |= PT_KERNEL;

	return page_table_map(t, va, pa, new_flags, 0);	
}

int page_table_map_device(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags)
{
	int new_flags = perm_to_pt(flags);
	new_flags  |= (PT_KERNEL | PT_NX);

	return page_table_map(t, va, pa, new_flags, 1);	

}

uint64_t generic_sbrk(TranslationTables_t *t, mem_region_t *head, uint64_t *sbrk, uint64_t length, int kernel);
uint64_t generic_sbrk(TranslationTables_t *t, mem_region_t *head, uint64_t *sbrk, uint64_t length, int kernel)
{
	mem_region_t *ptr = head;
	uint64_t cur_end;

	int (*mapper_ptr)(TranslationTables_t *t, uint64_t va, uint64_t pa, int flags);
	
	if (kernel) mapper_ptr = page_table_map_kernel;
	else mapper_ptr  = page_table_map_user;

	while(ptr) {
		if (ptr->va_base <= *sbrk && (ptr->va_base + ptr->va_size) >= *sbrk)
			break;
		ptr = (mem_region_t*) ptr->next;
	}

	if (!ptr) return (uint64_t) (-1);

	uint64_t new_end = *sbrk +  length;
	uint64_t frame = 0;
	cur_end = ptr->va_base + ptr->va_size;

	if (new_end < cur_end) { 
		*sbrk = new_end;
		return new_end;
	}

	//need to allocate memory
	uint64_t unused_space = cur_end - *sbrk;
	new_end = length - unused_space;
	new_end = PAGE_UP(new_end);
	
	for( uint64_t i=0; i <= new_end/PAGE_SIZE_4KB; i++) {
		frame = kframe_allocate_secure(1);
		if (frame == MAP_FAILED) return MAP_FAILED;

		if (mapper_ptr(t, cur_end +  (i*PAGE_SIZE_4KB), frame, PROT_R|PROT_W) != MAP_FAILED) 
			ptr->va_size += PAGE_SIZE_4KB;
	}
	
	*sbrk  += length;
	return *sbrk;	
} 

uint64_t shared_sbrk(int64_t length) 
{
	if ( length < 0 ) {
		length = 0 - length;
		kernel_struct.shared_sbrk -=  length;
		if (kernel_struct.shared_sbrk < kernel_struct.shared_sbrk_base) 
			kernel_struct.shared_sbrk = kernel_struct.shared_sbrk_base;
		return kernel_struct.shared_sbrk;
	}

	if (length == 0) 
		return kernel_struct.shared_sbrk;

	if ( (length + kernel_struct.shared_sbrk) > (kernel_struct.shared_heap_start + kernel_struct.shared_heap_size))
		return -ENOMEM;

	if ( (length + kernel_struct.shared_sbrk) < (kernel_struct.shared_heap_start)) {
		kernel_struct.shared_sbrk = kernel_struct.shared_sbrk_base;
		return kernel_struct.shared_sbrk;
	}

	kernel_struct.shared_sbrk += length;
	return kernel_struct.shared_sbrk;

}

int shared_brk(void *address) 
{
	if (kernel_struct.shared_sbrk_base != 0 ) 
		return -1; /*FIXME: check return value*/
	
	if ( ((uint64_t)address) < kernel_struct.shared_heap_start || 
		(((uint64_t)address) > (kernel_struct.shared_heap_start + kernel_struct.shared_heap_size )) )
		return -ENOMEM;

	kernel_struct.shared_sbrk_base = (uint64_t )address;
	kernel_struct.shared_sbrk = ((uint64_t )address);// + PAGE_SIZE_4KB;

	return 0;
}


uint64_t k_sbrk(int64_t length) 
{
	if ( length < 0 ) {
		length = 0 - length;
		kernel_struct.k_sbrk -=  length;
		if (kernel_struct.k_sbrk < kernel_struct.k_sbrk_base) 
			kernel_struct.k_sbrk = kernel_struct.k_sbrk_base;
		return kernel_struct.k_sbrk;
	}

	if (length == 0) 
		return kernel_struct.k_sbrk;
	if ( (length + kernel_struct.k_sbrk) > (kernel_struct.heap_start + kernel_struct.heap_size))
		return -ENOMEM;

	kernel_struct.k_sbrk += length;
	return kernel_struct.k_sbrk;
}

int k_brk(void *address) 
{
	if (kernel_struct.k_sbrk_base != 0 ) 
		return -1; /*FIXME: check return value*/

	if ( ((uint64_t)address) < kernel_struct.heap_start || 
		(((uint64_t)address) > (kernel_struct.heap_start + kernel_struct.heap_size )) )
		return -ENOMEM;

	kernel_struct.k_sbrk_base = (uint64_t )address;
	kernel_struct.k_sbrk = ((uint64_t )address);// + PAGE_SIZE_4KB;

	return 0;
}


/* check if page is mapped
 * check if permission to read/write
 * proceed with respective action
 */

/*turn access to user on, return true if it was on*/
int enable_access_to_user()
{
	asm volatile("isb; dsb ish;");
	uint64_t pan;
	asm volatile("mrs %0,PAN;":"=r"(pan));
	if (pan) {
		pan=0;
		asm volatile("msr PAN,%0;"::"r"(pan));
	}
	return 1;
}
/*disable kernel access to user pages */
void disable_access_to_user()
{
	uint64_t pan=1;
	asm volatile("msr PAN,%0;"::"r"(pan));
}
size_t copy_to_user(void *dst, const void *src, uint64_t count)
{
	int enable = 0;
	size_t copied = 0;

	enable = enable_access_to_user();
	if ( access_kernel_ok((uint64_t)dst, PROT_RW) )  {
		copied = -EINVAL;
	} else {
		while(copied < count) {
			((char*)dst)[copied] = ((const char*)src)[copied];
			copied++;
		}
	}

	if (enable)
		disable_access_to_user();

	return copied;
}

size_t copy_from_user(void *dst, const void *src, uint64_t count)
{
	int enable = 0;
	size_t copied = 0;

	enable = enable_access_to_user();
	if ( access_kernel_ok((uint64_t)src, PROT_R) )  {
		copied = -EINVAL;
	} else {
		while(copied < count) {
			((char*)dst)[copied] = ((const char*)src)[copied];
			copied++;
		}
	}

	if (enable)
		disable_access_to_user();

	return copied;
}

int access_user_ok(uint64_t va, int prot)
{
	uint64_t pa = 0;

	switch(prot) {
	case PROT_RW:
	case PROT_W:
		asm volatile("AT S1E0W, %1; mrs %0,PAR_EL1;":"=r"(pa):"r"(va));
		break;
	case PROT_R:
		asm volatile("AT S1E0R, %1; mrs %0,PAR_EL1;":"=r"(pa):"r"(va));
		break;
	case PROT_RWX:
		asm volatile("AT S1E0W, %1; mrs %0,PAR_EL1;":"=r"(pa):"r"(va));
		break;
	default:
		return 0;
	}

	return (pa & 0x01)? 1: 0; 
}

int access_kernel_ok(uint64_t va, int prot)
{
	uint64_t pa = 0;

	switch(prot) {
	case PROT_RW:
	case PROT_W:
		asm volatile("AT S1E1W, %1; mrs %0,PAR_EL1;":"=r"(pa):"r"(va));
		break;
	case PROT_R:
		asm volatile("AT S1E1R, %1; mrs %0,PAR_EL1;":"=r"(pa):"r"(va));
		break;
	case PROT_RWX:
		asm volatile("AT S1E1W, %1; mrs %0,PAR_EL1;":"=r"(pa):"r"(va));
		break;
	default:
		return 0;
	}

	return (pa & 0x01)? 1: 0; 
}
uint64_t translate_va_to_pa(uint64_t va)
{
	uint64_t pa = 0;
	asm volatile("AT S1E1R, %1; mrs %0,PAR_EL1;":"=r"(pa):"r"(va));
	
	return (pa & 0x01) ? 0: pa ;
}

uint64_t translate_pa_to_va(uint64_t parel)
{
	return (MASK_VA_36_TABLE_ADDRESS(parel)) << 12;	
}
void *get_physical_address(uint64_t va)
{
	uint64_t pa = translate_va_to_pa(va);
	pa = (pa & ~(0x0FFFUL)) | (va & 0x0FFF);
	pa =  (pa & ~((0xFFUL) << 56));
	
	return (void*)pa;
}

int access_is_ok(uint64_t va)
{
	int ret = 0;	
	ret = access_kernel_ok(va, PROT_R);
	if (ret) return ret;
	return access_user_ok(va, PROT_R);
}

