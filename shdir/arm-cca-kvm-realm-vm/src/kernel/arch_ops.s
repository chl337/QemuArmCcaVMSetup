/*
typedef struct context {
	uint64_t spsr_el1;
	uint64_t elr_el1;
	uint64_t esr_el1;
	uint64_t zero;
	uint64_t x[32];
	uint64_t far;
	uint64_t sp;
} context_t;
*/

.global _switch_to_user; 
.type _switch_to_user, %function;

.macro restore_state
	mov x1, x0;
	.irp n,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30
		ldr x\n, [x0], #8;
	.endr
.endm

_switch_to_user:
	ldr x1, [x0], #8;
	mrs x1, spsr_el1;

	ldr x1, [x0], #8;
	msr elr_el1, x1; 

	str x0, [sp, -8]!;//store start of x

	add x0, x0, #32;//x2
	restore_state

	ldr x1, [x0, #16];
	msr sp_el0, x1; 
	
	ldr x0, [sp], 8;
	add x0, x0, #16;

	ldp x0, x1, [x0];
	eret;



