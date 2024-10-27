#include "defines.h"
#include "common.h"
#include "kernel.h"
#include "heap.h"
#include "vma.h"
#include "uart.h"
#include "syscall.h"
#include "psci.h"
#include "rsi.h"
#include "stdio.h"

extern char *_start;
extern char *_sstack;
extern char _kernel_stack[];
extern char *_ram_end;
extern char *_binary_end;
extern char *_binary_start;
uint64_t get_stack_base();
struct kernel_struct kernel_struct;
uint64_t current_pa_base = KERNEL_PA_BASE;
uint64_t current_pa_binary_end = KERNEL_PA_BASE;

extern char * _TEXT_START;
extern char * _TEXT_END;
extern char * _RO_START;
extern char *_RO_END;
extern char *_DATA_START;
extern char *_DATA_END;
extern char * _BSS_START;
extern char *_BSS_END;
extern char * _PAGE_TABLE_START;
extern char *_PAGE_TABLE_END;
extern char * _BITMAP_NS_START;
extern char *_BITMAP_NS_END;
extern char * _BITMAP_START;
extern char *_BITMAP_END;
extern char * _SHARED_NS_MEMORY_START;
extern char *_SHARED_NS_MEMORY_END;

static uint64_t mmap_ns_start;
static uint64_t mmap_ns_end;
static uint64_t mmap_realm_start;
static uint64_t mmap_realm_end;
static uint64_t mmap_bitmap_realm_len;
static uint64_t mmap_bitmap_ns_len;
static uint64_t *mmap_bitmap_realm;
static uint64_t *mmap_bitmap_ns;

struct kernel_struct kernel_struct;

extern uint64_t translate_pa_to_va(uint64_t parel);
extern uint64_t translate_va_to_pa(uint64_t va);
extern void _switch_to_user(void *user_ctx); 
extern void vector_table();

uint64_t get_new_address(uint64_t address) 
{
	return (address - KERNEL_PA_BASE) + KERNEL_VA_BASE;
}

void enable_mmu()
{
	SCTLR_EL1_t sctlr_el1;
	sctlr_el1.reg  = 0xC008000;
	
	sctlr_el1.I = 0b0; //disable instruction cache	
	sctlr_el1.nAA = 0b1; //non alignment does not generate fault
	sctlr_el1.CP15BEN = 0b1;//barrier enable
	sctlr_el1.SA0 = 0b1;//stack alignment check in EL0
	sctlr_el1.C = 0b0; //disable data cache
	sctlr_el1.A = 0b0; //disable aligment checks
	sctlr_el1.M = 0b0; //disable mmu

	asm volatile ("dsb ish; isb; msr sctlr_el1, %0; isb; "::"r"((uint64_t)sctlr_el1.reg));


}

void init_mem_region( mem_region_t *region, uint64_t va, uint64_t size, int perm, int secure)
{       
        region->va_base = va;
        region->va_size = size;
        region->perm = perm;
        if (secure) region->is_secure = 1;
} 

void kernel_vma_regions_alloc()
{
	struct tmp_vma_region {
		uint64_t start;
		uint64_t end;
		int perm;
		int is_secure;
	} vma_regions[] = {
	        { (uint64_t)&_TEXT_START, (uint64_t)&_TEXT_END, PROT_X, 1},
	        { (uint64_t)&_RO_START, (uint64_t)&_RO_END, PROT_R, 1},
	        { (uint64_t)&_DATA_START, (uint64_t)&_DATA_END, PROT_RW, 1},
	        { (uint64_t)&_BSS_START, (uint64_t)&_BSS_END, PROT_RW, 1},
	        { (uint64_t)&_PAGE_TABLE_START, (uint64_t)&_PAGE_TABLE_END, PROT_RW, 1},
	        { (uint64_t)&_BITMAP_NS_START, (uint64_t)&_BITMAP_NS_END, PROT_RW, 1},
	        { (uint64_t)&_BITMAP_START, (uint64_t)&_BITMAP_END, PROT_RW, 1},
                //NOTE: NOT Secure
	        { (uint64_t)&_SHARED_NS_MEMORY_START, (uint64_t)&_SHARED_NS_MEMORY_END, PROT_RW, 0},
	};

	mem_region_t *head=0,*tmp;
	uint64_t size;
	uint64_t va;
	uint64_t base;

	base = PAGE_DOWN((uint64_t) &_binary_start);

	for (int i=0; i < (sizeof(vma_regions)/sizeof(struct tmp_vma_region)); i++) {
		tmp = kcalloc(sizeof(mem_region_t),1);
		if (tmp == 0) crash();

		size =  PAGE_UP(vma_regions[i].end) - PAGE_DOWN(vma_regions[i].start);
		va = vma_regions[i].start - base + KERNEL_VA_BASE; 
		va = PAGE_DOWN(va);
		init_mem_region(tmp, va, size, vma_regions[i].perm, vma_regions[i].is_secure); 
		mmap_region_insert(&head, tmp);
	}	

	//add heap region
	tmp = kcalloc(sizeof(mem_region_t),1);
	if (tmp == 0) crash();
	size = kernel_struct.mmap_start - kernel_struct.heap_start;
	init_mem_region(tmp, kernel_struct.heap_start, size, PROT_RW, 1); 
	mmap_region_insert(&head, tmp);

	//add mmap region 
	tmp = kcalloc(sizeof(mem_region_t),1);
	if (tmp == 0) crash();
	init_mem_region(tmp, kernel_struct.mmap_start, kernel_struct.mmap_size, PROT_RW, 1); 
	mmap_region_insert(&head, tmp);
	memcpy(&kernel_struct.mmap_region_head, tmp, sizeof(*tmp));
	kernel_struct.mmap_region_head.next = 0;

	//add device region 
	tmp = kcalloc(sizeof(mem_region_t),1);
	if (tmp == 0) crash();
	va = kernel_struct.mmap_size + kernel_struct.mmap_start;
	init_mem_region(tmp, va, kernel_struct.mmap_size, PROT_RW, 1); 
	tmp->is_device = 1;
	mmap_region_insert(&head, tmp);
	kernel_struct.vma_allocation = head;

}

void enable_mmap()
{
	uint64_t start;
	uint64_t range;
	uint64_t size;
	uint64_t ram_size = (uint64_t)&_binary_end - (uint64_t)&_ram_end;
	uint64_t new_heap_start = 0;

	uint64_t shared_memory_bit = get_shared_bit();

	mmap_bitmap_realm = (uint64_t *)&_BITMAP_START;
	mmap_bitmap_ns = (uint64_t *)&_BITMAP_NS_START;
	mmap_bitmap_realm_len = 0x4000;
	mmap_bitmap_ns_len = 0x4000;

	memset((void*)mmap_bitmap_realm, 0, mmap_bitmap_realm_len);
	memset((void*)mmap_bitmap_ns, 0, mmap_bitmap_ns_len);

	kernel_struct.secure_t.start_va =  KERNEL_VA_BASE;
	///kernel_struct.secure_t.start_pa =  (uint64_t)&_binary_start & ~(0xFFFUL);
	//memory tables for secure tables
	kernel_struct.secure_t.va_PGD = (uint64_t*) (((uint64_t)kernel_struct.secure_t.PGD - current_pa_base) +  KERNEL_VA_BASE); 
	kernel_struct.secure_t.va_PUD = (uint64_t*) (((uint64_t)kernel_struct.secure_t.PUD - current_pa_base) +  KERNEL_VA_BASE);
	kernel_struct.secure_t.va_PMD = (uint64_t*) (((uint64_t)kernel_struct.secure_t.PMD - current_pa_base) +  KERNEL_VA_BASE);
	kernel_struct.secure_t.va_PT3 = (uint64_t*) (((uint64_t)kernel_struct.secure_t.PT3 - current_pa_base) +  KERNEL_VA_BASE);

	//frame allocator for shared_memory

	uint64_t shared_size = ((uint64_t)&_SHARED_NS_MEMORY_END) - ((uint64_t)&_SHARED_NS_MEMORY_START);
	uint64_t shared_bitmap_size = ((uint64_t)&_BITMAP_NS_END) - ((uint64_t)&_BITMAP_NS_START);
	uint64_t shared_start = (uint64_t)&_SHARED_NS_MEMORY_START - PAGE_DOWN(((uint64_t)&_binary_start));

	shared_start = shared_start + current_pa_base;
	shared_bitmap_size = PAGE_UP(shared_bitmap_size);
	shared_size = PAGE_UP(shared_size);
	//rsi_set_ipa_state(shared_start, shared_size, 0x1); 

 	shared_start |= shared_memory_bit;
	kframe_allocator_init_insecure((uint64_t)&_BITMAP_NS_START, shared_bitmap_size, shared_start, shared_size);

	//frame allocator private memory
	uint64_t private_size = ((uint64_t)&_ram_end) - ((uint64_t)&_SHARED_NS_MEMORY_END);
	uint64_t private_bitmap_size = ((uint64_t)&_BITMAP_END) - ((uint64_t)&_BITMAP_START);
	uint64_t private_start = ((uint64_t)&_SHARED_NS_MEMORY_END)- PAGE_DOWN(((uint64_t)&_binary_start));
	private_size = PAGE_UP(private_size);
	private_bitmap_size = PAGE_UP(private_bitmap_size);
	private_start = private_start + current_pa_base;

	kframe_allocator_init_secure((uint64_t)&_BITMAP_START, private_bitmap_size, private_start, private_size);

	new_heap_start = ((uint64_t)&_SHARED_NS_MEMORY_END) - ((uint64_t)&_binary_start);
	new_heap_start = PAGE_UP(new_heap_start);

	kernel_struct.heap_start = new_heap_start + KERNEL_VA_BASE;
	kernel_struct.heap_size = KERNEL_HEAP_SIZE;

	size = ((uint64_t)&_ram_end) - ((uint64_t)&_SHARED_NS_MEMORY_END);

	kernel_struct.mmap_start = kernel_struct.heap_start + KERNEL_HEAP_SIZE;
	kernel_struct.cur_mmap = kernel_struct.mmap_start; 
	kernel_struct.mmap_size = size - KERNEL_HEAP_SIZE; 

	size = ((uint64_t)&_SHARED_NS_MEMORY_END) - ((uint64_t)&_SHARED_NS_MEMORY_START);
	start = (uint64_t)&_SHARED_NS_MEMORY_START; //start of shared memory


	kernel_struct.shared_region_size =  size;
	size = size / 2;

	kernel_struct.shared_heap_start = start ; 
	kernel_struct.shared_heap_size = size; 

	kernel_struct.shared_mmap_start = (start + size) ; 
	kernel_struct.shared_mmap_size  = size; 

	kernel_struct.k_sbrk_base = kernel_struct.k_sbrk = 0;
	kernel_struct.shared_sbrk_base = kernel_struct.shared_sbrk = 0;


	if( k_brk((void*) (kernel_struct.heap_start)) < 0 ) crash();
	if( shared_brk((void*) (kernel_struct.shared_heap_start )) < 0 ) crash();

	/*map the heap regions*/
	uint64_t pa = (kernel_struct.shared_heap_start - KERNEL_VA_BASE) + current_pa_base;
	uint64_t va = kernel_struct.shared_heap_start;
	size = kernel_struct.shared_heap_size; 
	pa |= shared_memory_bit;
	/*convert shared memory */

	//shared heap
	if ( !kframe_allocate_fixed_insecure(pa , size / PAGE_SIZE_4KB) ) {
		while(1) asm("nop;");
	}

	for (int _i=0; _i < (size/PAGE_SIZE_4KB); _i++) {
		page_table_map(&kernel_struct.secure_t, va , pa, 0/*PT_RW*/, 0);
		va += PAGE_SIZE_4KB;
		pa += PAGE_SIZE_4KB;
	}	

	/*secure heap*/
	pa =  (kernel_struct.heap_start - KERNEL_VA_BASE) + current_pa_base;
	va =  kernel_struct.heap_start;
	size = kernel_struct.heap_size; 

	if ( !kframe_allocate_fixed_secure(pa, size / PAGE_SIZE_4KB) ) {
		while(1) asm("nop;");
	}

	for (int _i=0; _i < (size/PAGE_SIZE_4KB); _i++) {
		page_table_map(&kernel_struct.secure_t, va , pa,  0/*PT_RW*/, 0);
		va += PAGE_SIZE_4KB;
		pa += PAGE_SIZE_4KB;
	}	


	asm volatile ("dsb ish;");	
	asm volatile ("msr cpacr_el1,%0 ;"::"r" (0x3 << 20));//disable trapping of floating point registers	
	heap_init();
	/*memory allocation is done, lets create a vma allocation*/
	kernel_vma_regions_alloc();

}

void enable_exception_vectors()
{
	asm volatile("msr vbar_el1, %0;"::"r"(vector_table):);
}

char mm[] = "done configuring translation....\nenabling mmu...";
char prog[] ="./user/test.elf";
char pwd[]= "PWD=/user/";
char home[] =  "HOME=/user";
char test_elf[] = "test.elf";

char msg[] = "finished mapping kernel, going to execute user prog\n";
char km_mm[] = "doing memory remapping\n";
char km_mm1[] = "done enabling mmap\n";
char end_of_kinit[] = "this is the end of kernel init function\n";
uint8_t kernel_debug_buffer[64];

void debug_log_message(char *msg)
{
	int count = 0;
	char *ptr = msg;
	if (!msg) return;
	while (kernel_debug_buffer[0]==1) {asm("nop;");};
	while (count < 63) {
		if (ptr == '\0') 
			break;
		kernel_debug_buffer[count + 1] = *ptr;
		count++;
		ptr++;
	}
	//kernel_debug_buffer[0]=1; 
}


void kernel_init()
{
	uint64_t ram_shared_size;
	uint64_t size;

	ram_shared_size = PAGE_UP(((uint64_t)&_SHARED_NS_MEMORY_END & ~(0xFFFUL))) -  PAGE_DOWN(((uint64_t)&_SHARED_NS_MEMORY_START));

	current_pa_base = PAGE_DOWN(((uint64_t)&_binary_start));
	current_pa_binary_end = PAGE_UP( ((uint64_t)(&_binary_end)) ); 

	size = current_pa_binary_end - current_pa_base;
	size = PAGE_UP(size);	

	enable_exception_vectors();
	memset(&kernel_struct.secure_t, 0, sizeof(TranslationTables_t));
	memset(&kernel_struct.insecure_t, 0, sizeof(TranslationTables_t));

	set_upper_translation_tables(&kernel_struct.secure_t);
	set_lower_translation_tables(&kernel_struct.insecure_t);
	
	kernel_struct.secure_t.start_va =  KERNEL_VA_BASE;
	kernel_struct.secure_t.start_pa =  current_pa_base;

	kernel_struct.insecure_t.start_pa =  current_pa_base;
	kernel_struct.insecure_t.start_va =  current_pa_base;

	normal_map_kernel_4KB(&kernel_struct.secure_t, size, ram_shared_size);
	normal_map_kernel_4KB(&kernel_struct.insecure_t, size, ram_shared_size);
	
	configure_translation();
	uart_send_byte_array(mm, strlen(mm));
	//debug_log_message(end_of_kinit);
  
}

void map_uart(uint64_t va)
{
	uint64_t pa = get_uart_base();
	page_table_map_device(&kernel_struct.secure_t, va, pa , 0/*PT_RW*/);
	pa = get_uart_syscall_base();
	page_table_map_device(&kernel_struct.secure_t, va + 0x1000, pa , 0/*PT_RW*/);

	asm("dsb ish; isb;");
	uart_init(va);
	uart_syscall_init(va + 0x1000);
}

void kernel_test()
{
	uint64_t input[8];
	uint64_t ad[8];
	input[0] = (uint64_t)&mmap_bitmap_realm_len;
	input[1] = (uint64_t)&mmap_bitmap_ns_len;
	input[2] = (uint64_t)&mmap_ns_start;
	input[3] = (uint64_t)&mmap_ns_end;
        input[4] = (uint64_t)&mmap_realm_start;
	input[5] = (uint64_t)&mmap_realm_end;
	input[6] = (uint64_t)enable_mmap;
	input[7] = (uint64_t)_kernel_stack;

	uint64_t value;
	value = input[2] - ((uint64_t)&_binary_start & ~(0xFFFUL)) + KERNEL_VA_BASE;	
	value = translate_va_to_pa(value);
	value = translate_pa_to_va(((uint64_t)&_binary_start & ~(0xFFFUL)));

	enable_mmap();
	
	for( int i=0; i < 8; i++) { 
		value = translate_va_to_pa(input[i]);
		ad[i] = translate_pa_to_va(value);
	}	

	for( int i = 0; i < 24; i++) {
		input[i] = kframe_allocate_secure(1);
	}

	for( int i = 0; i < 24; i++) {
		kframe_free_secure(input[i]);
	}

	asm volatile ("nop;");
	//call scheduler	

}

void schedule()
{
	while(1) {
		for(process_t *t = kernel_struct.process_head; t; t=t->next) {
			if (t->state == READY) {
				t->state = RUNNING;
				set_current_process(t);
				uart_send_byte_array("\nswitch to user\n",16);
				uart_send_byte_array("\n\n",2);
				_switch_to_user(&t->ctx);
			}  
		}	
	}
}

void get_cpu_features() {

	kernel_struct.feat_ID_AA64ISAR0_EL1 	= get_feature_ID_AA64ISAR0_EL1();
	kernel_struct.feat_ID_AA64ISAR1_EL1 	= get_feature_ID_AA64ISAR1_EL1();
	kernel_struct.feat_ID_AA64MMFR0_EL1 	= get_feature_ID_AA64MMFR0_EL1();
	kernel_struct.feat_ID_AA64MMFR1_EL1 	= get_feature_ID_AA64MMFR1_EL1();
	kernel_struct.feat_ID_AA64PFR0_EL1 	= get_feature_ID_AA64PFR0_EL1();
	kernel_struct.feat_ID_AA64PFR1_EL1 	= get_feature_ID_AA64PFR1_EL1();
	kernel_struct.feat_ID_AA64DFR0_EL1 	= get_feature_ID_AA64DFR0_EL1();
	kernel_struct.feat_ID_AA64DFR1_EL1 	= get_feature_ID_AA64DFR1_EL1();
	kernel_struct.feat_MIDR_EL1 		= get_feature_MIDR_EL1();
	kernel_struct.feat_MPIDR_EL1 		= get_feature_MPIDR_EL1();
	kernel_struct.feat_REVIDR_EL1 	= get_feature_REVIDR_EL1(); 
}

void kernel_main()
{
	memset(kernel_debug_buffer, 0, 64);
	//debug_log_message(km_mm);

	get_cpu_features();

//	int pan = cpu_supports_PAN();
//	if (pan) 
//		uart_send_byte_array("\ncpu has feature PAN\n",21);
//
	enable_mmap();
	//debug_log_message(km_mm1);
	map_uart(KERNEL_VA_BASE + (512*1024*1024));
	uart_send_byte_array(km_mm, strlen(km_mm));
	enable_exception_vectors();

	uart_send_byte_array(msg, strlen(msg));
	char *b = smalloc(20);
	if (b) {
		sfree(b);
	}
	char *argv[2];
	argv[0]= test_elf;
	argv[1] = 0;
	char *new_env[3];
	new_env[0] = pwd;
	new_env[1] = home ;
	new_env[2] = 0;


        uart_send_byte_array("\nTest SmcccFilter - PSCI Version\n", 33);
        //psci_version();
        rsi_host_call();
     //   psci_system_off();
	sys_execve(prog, argv, new_env);
	uart_send_byte_array("\nexecuting scheduler\n",20);
	schedule();	
	while(1) {};

}


unsigned long __stack_chk_guard;
static void __stack_chk_guard_setup(void)
{
     __stack_chk_guard = 0xBAAAAAAD;//provide some magic numbers
}

static void __stack_chk_fail(void)                         
{
	asm volatile ("b .");
}// will be called when guard variable is corrupted 
