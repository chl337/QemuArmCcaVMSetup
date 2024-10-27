#include <string.h>
#include "process.h"
#include "elf.h"
#include "fd.h"
#include "errno.h"
#include "vma.h"
#include "kernel.h"
#include "heap.h"
#include "syscall.h"

extern struct kernel_struct kernel_struct;
/* Declaration of symbols (any type can be used) */
char user_binary[8192];

void set_current_process(process_t *p)
{
	kernel_struct.current = p;	
}

int get_current_process_pid()
{
	if (!kernel_struct.current)
		return -1;
	return kernel_struct.current->pid;
}

int add_new_process(process_t *p)
{
	process_t *ptr = kernel_struct.process_head;

	if (!p) return -1;

	if (ptr == 0) {
		ptr = kernel_struct.process_head = p;
	} else {
		while(1) {
			if (ptr->next == 0) {
				*(&ptr->next) = p;
				break;
			 }
			ptr = ptr->next;
		}
	}

	p->pid = p->tid = ++kernel_struct.next_pid;
	return p->pid;	
}

int elf_pf_to_prot(int pflags)
{
	int prot = 0;
	
	prot = (pflags & PF_R) ? PROT_R : 0;
	prot = prot | (pflags & PF_W ? PROT_W : 0);
	prot = prot | (pflags & PF_X ? PROT_X : 0);
	return prot;
}

mem_region_t **get_mem_region_tail(mem_region_t *head)
{
	mem_region_t *ptr = head;
	
	while(ptr->next) ptr= ptr->next;
	return &ptr->next;
}

int load_executable(int fd, process_t *p, void *data, uint64_t size)
{
	void *buf = kmalloc(0x2000);	
	Elf64_Ehdr *hdr; 
	int64_t count;

	if (buf == 0) return -ENOMEM;
	
	if ( !(data && size > 0 ) ) {
		count = sys_read(fd, buf, sizeof(Elf64_Ehdr*));
		if (count <= 0) return -ENOEXEC; 
	} else {
		memcpy(buf, data, size);
	}

	hdr = (Elf64_Ehdr*) buf;

	if (! (hdr->e_type == ET_EXEC || hdr->e_type == ET_DYN) ) return -ENOEXEC;

	p->entry = (uint64_t) hdr->e_entry;
	Elf64_Phdr *phdr = (Elf64_Phdr*)((uint64_t)buf + hdr->e_phoff);
	uint64_t address = 0;
	for( int i=0; i < hdr->e_phnum; i++,phdr++) {
		if (phdr->p_type == PT_LOAD) {
			uint64_t sz = phdr->p_filesz;
			int prot = elf_pf_to_prot(phdr->p_flags) | PROT_W;
			uint64_t base = PAGE_DOWN( phdr->p_vaddr );
			uint64_t load_size = PAGE_UP(phdr->p_memsz);	
			uint64_t src_offset = phdr->p_offset + (uint64_t)data;

			address = (uint64_t) sys_mmap_user(&p->t, p->head_region,(void*)base, load_size, prot, MAP_FIXED, 0, 0);
			asm("isb; dsb ish;");
			if (address == MAP_FAILED || address == 0)
				return -ENOMEM;
			//copy the data to memory
			copy_to_user((void*) PAGE_DOWN(phdr->p_vaddr), (uint64_t*)src_offset,  sz);
			uint64_t pa = (uint64_t) get_physical_address((uint64_t)address);
			page_table_map_user(&p->t, (uint64_t)address, pa, elf_pf_to_prot(phdr->p_flags));/*rx only*/
		}
	}
	//stack
	//p->stack_base = (void*) 0x00007ffffffff000ull;
	p->stack_base = (void*) 0x00007ff000ull;
	p->stack_size = 0x4000;

	if ( !sys_mmap_user(&p->t,p->head_region, (void*)( p->stack_base - p->stack_size), p->stack_size, PROT_RW, MAP_FIXED, 0, 0) )
		return -ENOMEM;
	return 0;
}

int check_and_get_count( char *const arg[])
{

	int count=0;
	if (arg == 0) return -EINVAL;

	for (int i=0;;i++) {
		if ( arg[i] == 0 ) break;
		else count++;
	}
	return count;
}

int create_new_process(process_t *p, char *const argv[], char *const envp[])
{

	int arg_count = check_and_get_count(argv);	
	int env_count = check_and_get_count(envp);	
	int total_count = 0;

	if (arg_count <= 0) return -EINVAL;

	for(int i=0; i< arg_count; i++) {
		total_count += sizeof(void *) + strlen(argv[i]) + 1;
	}

	for(int i=0; i< env_count; i++) {
		total_count += sizeof(void *) + strlen(envp[i]) + 1;
	}

	if (env_count == 0) {
		total_count += sizeof(void*);
	}

	total_count += (2 *sizeof(void*));

	//align to 16  
	uint64_t space = (total_count + 15) & ~(0x0F);

	p->ctx.sp = p->stack_base - space; 

	char **ptr = (char**) p->ctx.sp;
	char *ptr_str = (char*) &ptr[arg_count + env_count + 2];


	int index = 0;
	for (;index < arg_count; index++) {
		ptr[index] = (char*) ptr_str;
		strcpy(ptr_str, argv[index]);
		ptr_str = (char*)&(ptr_str[strlen(ptr_str) + 1]);
	}
	ptr[index] = 0;
	index++;

	for (int i=0; i < env_count; i++) {
		ptr[index] = (char*) ptr_str;
		strcpy(ptr_str, envp[i]);
		ptr_str = (char*)&(ptr_str[strlen(ptr_str) + 1]);
		index += 1;
	}
	ptr[index] = 0;
	
	p->ctx.sp = p->ctx.sp - 8;
	*((int*)p->ctx.sp) = arg_count;

	p->ctx.x[0] = arg_count;
	p->ctx.x[1] = &ptr[0];
	p->ctx.x[2] = &ptr[arg_count + 1];
	p->ctx.elr_el1 = p->entry;
	p->cmd_line = &ptr[0];

	return 0;	
}

int sys_execve(const char *path, char *const argv[], char *const envp[])
{
	process_t *p = 0;
	p = (process_t *) kcalloc(sizeof(process_t), 1);

	if (p == 0 ) return -ENOMEM;
	memset(p, 0, sizeof(process_t));
	p->head_region = kcalloc(sizeof(mem_region_t), 1);
	if (!p->head_region) {
		kfree(p);
		return -ENOMEM;
	}

	uart_send_byte_array((char*)"\ncreating translation tables\n",29);
	create_user_translation_table(&p->t);
	uart_send_byte_array((char*)"\ndone creating translation tables\n",34);

//allocate a stack
//	p->stack_base = sys_mmap_user(&p->t,p->head_region, (void*)((uint64_t)0xff000), 0x2000, PROT_RW, MAP_FIXED, -1, 0);
//	if (p->stack_base != (void*)((uint64_t)0xff000) ) crash();
//end allocate a stack

	char message[]= "opening path: ";
	uart_send_byte_array(message, strlen(message));
	uart_send_byte_array(path, strlen(path));
	int fd = 1;// open(path,O_RDONLY,0);
	char v = (char)fd + '0';

	uart_send_byte_array((char*)"\nreturn open fd is:   ",20);
	uart_send_byte_array(&v,1);

	if (fd < 0) return fd;

	asm("msr ttbr0_el1, %0; isb; dsb ish;"::"r"(p->t.PGD));	
	/*new page table for process*/

	uart_send_byte_array((char*)"\nloading executable \n",20);
	int ret = load_executable(fd, p, (void*)user_binary,8192);
	uart_send_byte_array((char*)"\ndone loading, closing fd\n",26);
//	sys_close(fd);
	uart_send_byte_array((char*)"\ncreating new process\n",21);

	if (ret < 0 ) return ret;

	ret = create_new_process(p, argv, envp); 

	uart_send_byte_array((char*)"\ncreating proc done \n",20);
	if (ret < 0) return ret;


	p->state = NONE;
	uart_send_byte_array((char*)"\nadding new proc done \n",23);
	add_new_process(p);
	uart_send_byte_array((char*)"\ndone + new proc done \n",23);
	insert_fd(p->pid, 0, 0);
	insert_fd(p->pid, 1, 1);
	insert_fd(p->pid, 2, 2);

	p->state = READY;
	uart_send_byte_array("\nnew process created going to scheduler\n",39);
	return p->pid;	
}



/*

vma allocation

KERNEL_VA_BASE --> &_text_end ==> rx <kernel_text>
_data_start ---> _&data_end ==> rw
_bitmap_ns <bitmap_start,bitmap_end>
_bitmap_s <bitmap_start,bitmap_end>

*/
