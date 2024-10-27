# 0 "init.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "init.S"

.extern kernel_init
.extern get_stack_base
.section ".text"
.macro save_gp_regs
 stp x0, x1, [sp, #0x0]
 stp x2, x3, [sp, #0x10]
 stp x4, x5, [sp, #0x20]
 stp x6, x7, [sp, #0x30]
 stp x8, x9, [sp, #0x40]
 stp x10, x11, [sp, #0x50]
 stp x12, x13, [sp, #0x60]
 stp x14, x15, [sp, #0x70]
 stp x16, x17, [sp, #0x80]
 stp x18, x19, [sp, #0x90]
 stp x20, x21, [sp, #0xa0]
 stp x22, x23, [sp, #0xb0]
 stp x24, x25, [sp, #0xc0]
 stp x26, x27, [sp, #0xd0]
 stp x28, x29, [sp, #0xe0]

 stp x30, xzr, [sp, #0xf0]
.endm

.macro restore_gp_regs
 ldp x30, xzr, [sp, #0xf0]
 ldp x28, x29, [sp, #0xe0]
 ldp x26, x27, [sp, #0xd0]
 ldp x24, x25, [sp, #0xc0]
 ldp x22, x23, [sp, #0xb0]
 ldp x20, x21, [sp, #0xa0]
 ldp x18, x19, [sp, #0x90]
 ldp x16, x17, [sp, #0x80]
 ldp x14, x15, [sp, #0x70]
 ldp x12, x13, [sp, #0x60]
 ldp x10, x11, [sp, #0x50]
 ldp x8, x9, [sp, #0x40]
 ldp x6, x7, [sp, #0x30]
 ldp x4, x5, [sp, #0x20]
 ldp x2, x3, [sp, #0x10]
 ldp x0, x1, [sp, #0x0]
.endm

#.macro init_stack
# adrp x0,_kernel_stack;
# add x0, x0, #0x10000;
# mov sp, x0;
# stp xzr,xzr,[sp,#-16]!;
# mov x0,sp;
# mov fp,x0;
#.endm


.global _start
.type _start, %function

_start:
 mrs x1, ID_AA64MMFR0_EL1;
 and x1, x1, #0x0F;
 mrs x0, CurrentEL;
 cmp x0, #0b0100;
# beq in_el1;

# adr x0, in_el1;
# msr ELR_EL3, x0;
# mov x0, #0x09;
# msr SPSR_EL2, x0;

# mov x1, #0b01111000000;
# mov x2, #0b0000001001;
# orr x0, x1, x2;
# msr spsr_el2, x0;

# adr x1, address_stack;
# ldr x1, [x1];
# msr sp_el1, x1;

# eret;
in_el1:

 #init_stack;
 bl get_stack_base;
 mov sp, x0;
 stp xzr,xzr,[sp,#-16]!;
 mov x0,sp;
 mov fp,x0;
 mov x0, 0x4000000 ; #64MB
 mov x1, 0x400000 ; #4MB
 bl kernel_init;


 mov x0, #0;
 mov sp, x0;
 mov fp, x0;
 mov lr, x0;

##setup jump address in x2
# adr x2,address_binary_start;
# ldr x2,[x2];
# adr x3, address_vma_done;
# ldr x3, [x3];
# sub x2, x3, x2;
# ldr x3,=KERNEL_VA_BASE;
# orr x2, x2, x3;
##end setup
 mrs x0,sctlr_el1;
 mov x1, #0x1;
 orr x0, x0, x1;
 msr sctlr_el1, x0;
 adr x2, .;
 add x2,x2, #16;
 br x2;
#_vma_done:
# #jump to using vma
# isb;
# dsb ish;
# init_stack;
# bl kernel_main
# b .;
#.section ".data"

#address_binary_start:
# .quad =_binary_start;
#address_ram_end:
# .quad =_ram_end;
#address_stack_base:
# .quad =_stack_base;
#address_sstack:
# .quad =_sstack;
#address_vma_done:
# .quad =_vma_done;
#address_stack:
# .quad =_stack;





#setup stack
