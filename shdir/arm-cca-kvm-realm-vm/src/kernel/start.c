#include <stdint.h>
#include "uart.h"
#include "elf.h"
#include "linker_defines.h"
#include "rsi.h"
/*definitions of elf header*/
extern char *_binary_start;
//force this into .data section by initializing
//after symbol resolution, we will initialize memory
//regions to their init value. .data remains the 
//same but .bss will be initialized to zero before we
//start calling other initialization routines
//we want early kernel relocations to persist beyond 
//the .bss reinitialization
static uint64_t _is_realm;
static uint64_t _shared_bit;
int is_realm()
{
	return _is_realm;
}
uint64_t get_stack_base();

struct RelocStore {
	uint64_t address;
	uint64_t value;
};
extern struct RelocStore global_relocations[MAX_RELOCATIONS];
 __attribute__((always_inline)) __attribute__((naked)) void relocate_kernel(uint64_t base)
{
	
	volatile register Elf64_Sym *reloc_symbol_table  = 0;
	volatile register Elf64_Shdr *sections_table 	 = 0;
	volatile register Elf64_Shdr *rela_section 	 = 0;
	volatile register Elf64_Rela *rela_table 	 = 0; 

	sections_table = (Elf64_Shdr*) (((Elf64_Ehdr*)base)->e_shoff + base);


	for (Elf64_Half i=0; i < ((Elf64_Ehdr*)base)->e_shnum; i++) {

		if( sections_table[i].sh_type == ELF64_SHT_RELA ) {
			rela_table = (Elf64_Rela*) (base  + sections_table[i].sh_offset); 
			reloc_symbol_table = (Elf64_Sym*) (sections_table[ sections_table[i].sh_link ].sh_offset + base);
			rela_section = &sections_table[i];
			break;
		}
	}
	/*start doing relocations*/
	
	for ( Elf64_Half i=0; i < (rela_section->sh_size / rela_section->sh_entsize) ; i++ ) {
		switch( ELF64_R_TYPE(rela_table[i].r_info) ) {
		case RAARCH64_ABS64: 
		case RAARCH64_GLOB_DATA: 
		case RAARCH64_JUMP_SLOT: {
			/*address = base + r_offset*/
			/*value = base + symbol_value + r_addend*/
			*((uint64_t*)(base + rela_table[i].r_offset)) = \
					base + reloc_symbol_table[ ELF64_R_SYM(rela_table[i].r_info) ].st_value + rela_table[i].r_addend;

			break;
			}
		default:
			break;
		}
	} 
	//create a memory barrier here

	asm volatile("" ::: "memory");

	for ( Elf64_Half i=0; i < MAX_RELOCATIONS; i++ ) {
		global_relocations[i].address = 0;
		global_relocations[i].value = 0; 
	}
	
	for ( Elf64_Half i=0; i < (rela_section->sh_size / rela_section->sh_entsize) ; i++ ) {
		global_relocations[i].address =  ((uint64_t)(base + rela_table[i].r_offset));
		global_relocations[i].value = *((uint64_t*)(base + rela_table[i].r_offset)) - base + KERNEL_VA_BASE; 
	}

}

char message[] = "hell kernel\n";
char kernel_init_msg[] = "done  kinit\n";

uint64_t shift_to_va ()
{
	uint64_t stack_offset = get_stack_base() - ((uint64_t)&_binary_start & ~0x0FFF);

	for (int i=0; i < MAX_RELOCATIONS; i++) {
		if (global_relocations[i].address == 0) break;
		*((uint64_t*) global_relocations[i].address) = global_relocations[i].value;
	}
	return (KERNEL_VA_BASE + stack_offset);
}

uint64_t get_shared_bit()
{
	return _shared_bit;
}

uint64_t _set_shared_bit()
{
	_shared_bit = (0x1 << 32);
//	uint64_t base = get_stack_base();	
//	uint64_t ipa_width = 0;
//	base = base - 0x1000;
//	
//	if(rsi_realm_config(base) != RSI_ERROR_INPUT) {
//		ipa_width = *((uint64_t*)base);
//	}
//	if (ipa_width > 0 ) {
//		_shared_bit = (0x1 << ipa_width);
//		return _shared_bit;
//	}
//	return 0;
}

#define SUPPORTS_REALM (0x3UL << 56)
void check_init_features()
{
	uint64_t register id asm("x0") __attribute__((unused)) = 0;
	asm(" mrs x0, ID_AA64PFR0_EL1");
	//if (SUPPORTS_REALM & id) {
	if (1) {
		_is_realm = 1;
		//_set_shared_bit();
		_shared_bit = 0x1UL << 32;
	} else {
		_is_realm = 0;
		_shared_bit = 0;
	}
}


void _start()
{
	register uint64_t *base asm("x0") = (uint64_t*) 0x42000000;
	relocate_kernel((uint64_t)base);

	asm volatile  ("\
	.equ UART_PA_BASE, 0x9000000; \
	.equ KERNEL_VA_START, 0xFFFFF00000000000; \
	.equ KERNEL_PA_BASE,  0x42000000; \
	.extern kernel_init; \
	.extern get_stack_base; \
	.extern enable_mmu; \
	.extern uart_init; \
	.extern uart_send_byte_array; \
	.extern shift_to_va; \
	.extern kernel_main; \
	.extern get_new_address; \
	.extern check_init_features; \
	.global message; \
	.global kernel_init_msg; \
	mrs  x1, ID_AA64MMFR0_EL1; \
	and  x1, x1, #0x0F; \
	mrs  x0, CurrentEL; \
	bl get_stack_base; \
	mov sp, x0; \
	stp xzr,xzr,[sp,#-32]!; \
	mov x0,sp;\
	mov fp,x0;\
	bl check_init_features; \
	bl get_uart_base; \
	bl uart_init; \
	ldr x0, =message; \
	mov x1, #12; \
	bl uart_send_byte_array;\
	bl kernel_init; \
	ldr x0, =kernel_init_msg; \
	mov x1, #12; \
	bl uart_send_byte_array;\
	bl enable_mmu;\
	adr x0, sync_main; \
	bl get_new_address; \
	stp x0, x0, [sp, #-16]!; \
	bl shift_to_va; \
	mrs x0, sctlr_el1; \
	mov x1, #0x1; \
	orr x0, x0, x1; \
	msr sctlr_el1, x0; \
	isb; \
	dsb ish; \
	bl get_stack_base; \
	ldp x3, x4, [sp],#16; \
	mov sp, x0; \
	br x3; \
sync_main:bl kernel_main;\
end:\
	b end;");

	 __builtin_unreachable();
}
