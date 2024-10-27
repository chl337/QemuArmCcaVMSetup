#ifndef _KERNEL_H
#define _KERNEL_H
#include "common.h"
#include "process.h" 
#include "vma.h"
#include "defines.h"

struct device {
	uint64_t address;
	uint64_t length;
	uint8_t type;
	char name[12];
};
struct kernel_struct {
	uint64_t heap_start;
	uint64_t heap_size;

	uint64_t mmap_start;
	uint64_t mmap_size;
	uint64_t cur_mmap;

	uint64_t insecure_va_start;
	uint64_t insecure_pa_start;
	uint64_t insecure_pg_cnt;
	uint64_t device_map_start;
	uint64_t k_sbrk;
	uint64_t k_sbrk_base;

	uint64_t shared_sbrk;
	uint64_t shared_sbrk_base;
	uint64_t shared_region_size;
	uint64_t shared_heap_start;
	uint64_t shared_heap_size;
	uint64_t shared_mmap_start;
	uint64_t shared_mmap_size;

	kframe_alloc_t secure_frames;
	kframe_alloc_t insecure_frames;
	TranslationTables_t secure_t;
	TranslationTables_t insecure_t;
	/*syscall pointer*/
	struct fd_table *sys_table;
	int next_pid;
	process_t *process_head;
	process_t *current;
	mem_region_t mmap_region_head;
	mem_region_t *vma_allocation;
	fd_table_t *fd_head;

	uint64_t feat_ID_AA64ISAR0_EL1;
	uint64_t feat_ID_AA64ISAR1_EL1;
	uint64_t feat_ID_AA64MMFR0_EL1;
	uint64_t feat_ID_AA64MMFR1_EL1;
	uint64_t feat_ID_AA64PFR0_EL1;
	uint64_t feat_ID_AA64PFR1_EL1;
	uint64_t feat_ID_AA64DFR0_EL1;
	uint64_t feat_ID_AA64DFR1_EL1;
	uint64_t feat_MIDR_EL1;
	uint64_t feat_MPIDR_EL1;
	uint64_t feat_REVIDR_EL1; 

};
extern struct kernel_struct kernel_struct;

#define PROT_R 1
#define PROT_W 2
#define PROT_X 4
#define PROT_RW (PROT_R | PROT_W)
#define PROT_RWX (PROT_RW | PROT_X)

size_t copy_to_user(void *dst, const void *src, uint64_t count);
size_t copy_from_user(void *dst, const void *src, uint64_t count);
void * sys_mmap_user(TranslationTables_t *t, mem_region_t *region, void *address, uint64_t size, int prot, int flags, int fd, int64_t offset);
void set_current_process(process_t *p);
int get_current_process_pid();

uint64_t get_feature_ID_AA64ISAR0_EL1();
uint64_t get_feature_ID_AA64ISAR1_EL1();
uint64_t get_feature_ID_AA64MMFR0_EL1();
uint64_t get_feature_ID_AA64MMFR1_EL1();
uint64_t get_feature_ID_AA64PFR0_EL1();
uint64_t get_feature_ID_AA64PFR1_EL1();
uint64_t get_feature_ID_AA64DFR0_EL1();
uint64_t get_feature_ID_AA64DFR1_EL1();
uint64_t get_feature_MIDR_EL1();
uint64_t get_feature_MPIDR_EL1();
uint64_t get_feature_REVIDR_EL1(); 

int cpu_supports_PAN();
#endif
