// Typical exception vector table code.
.global vector_table
.section .text
.type vector_table, %function

.extern sync_handler
.extern serro_handler
.extern fiq_handler
.extern irq_handler
.extern _switch_to_user 


.macro push_pair Xn0, Xn1
	stp \Xn0,\Xn1, [sp,-16]!;
.endm

.macro pop_pair Xn0, Xn1
	ldp \Xn0,\Xn1, [sp, 16]!;
.endm
// struct _state {
//	uint64_t spsr_el1;
//	uint64_t elr_el1;
//	uint64_t esr_el1;
//	uint64_t pad;
//	uint64_t x[32];
//}
.macro el1_excep_save_state
	push_pair x30,x30;
	push_pair x28,x29;
	push_pair x26,x27;
	push_pair x24,x25;
	push_pair x22,x23;
	push_pair x20,x21;
	push_pair x18,x19;
	push_pair x16,x17;
	push_pair x14,x15;
	push_pair x12,x13;
	push_pair x10,x11;
	push_pair x8,x9;
	push_pair x6,x7;
	push_pair x4,x5;
	push_pair x2,x3;
	push_pair x0,x1;
				/*ESR_EL1, PAD*/
	mrs x0, esr_el1; 
	push_pair x0,xzr;
				/*SPSR_EL1, ELR_EL1*/
	mrs x1, elr_el1;
	mrs x0, spsr_el1;
	push_pair x0,x1;
.endm

.macro el1_excep_restore_state
	/*undo msr*/
	pop_pair x0,x1; //esr_el1,xzr
	pop_pair x0,x1; //spsr_el1,elr_el1
	/*end msr*/
	pop_pair x0,x1;
	pop_pair x2,x3;
	pop_pair x4,x5;
	pop_pair x6,x7;
	pop_pair x8,x9;
	pop_pair x10,x11;
	pop_pair x12,x13;
	pop_pair x14,x15;
	pop_pair x16,x17;
	pop_pair x18,x19;
	pop_pair x20,x21;
	pop_pair x22,x23;
	pop_pair x24,x25;
	pop_pair x26,x27;
	pop_pair x28,x29;
	pop_pair x30,x30;
.endm

.balign 0x1000
vector_table:
.type curr_el_sp0_sync, %function
curr_el_sp0_sync:        // The exception handler for a synchronous 
el1_excep_save_state; // exception from the current EL using SP0.
mov x0, sp;
bl sync_handler;
b _switch_to_user 

.balign 0x80
.section .text
.type curr_el_sp0_irq, %function
curr_el_sp0_irq:         // The exception handler for an IRQ exception
b .;                     // from the current EL using SP0.

.balign 0x80
.section .text
.type curr_el_sp0_fiq, %function
curr_el_sp0_fiq:         // The exception handler for an FIQ exception
b .;                     // from the current EL using SP0.

.balign 0x80
.section .text
.type curr_el_sp0_serror, %function
curr_el_sp0_serror:      // The exception handler for a System Error 
b .;                     // exception from the current EL using SP0.

.balign 0x80
// The exception handler for a synchrous
// exception from the current EL using the
// current SP.
.section .text
.type curr_el_spx_sync, %function
curr_el_spx_sync:
el1_excep_save_state;
mov x0, sp;
bl sync_handler;
b exception_restore_half;

.balign 0x80
//IRQ exception_handler with current SP
.section .text
.type curr_el_spx_irq, %function
curr_el_spx_irq:
el1_excep_save_state;
mov x0, sp;
bl irq_handler;
b exception_restore_half;

//FIQ exception_handler with current SP
.balign 0x80
.section .text
.type curr_el_spx_fiq, %function
curr_el_spx_fiq:
el1_excep_save_state;
mov x0, sp;
bl fiq_handler;
b exception_restore_half;

//System Error handler
.balign 0x80
.section .text
.type curr_el_spx_serror, %function
curr_el_spx_serror:
el1_excep_save_state;
mov x0, sp;
bl serror_handler;
b exception_restore_half;

.balign 0x80
.section .text
.type lower_el_aarch64_sync, %function
lower_el_aarch64_sync:   // The exception handler for a synchronous 
el1_excep_save_state; // exception from a lower EL (AArch64).
mov x0, sp;
bl sync_handler;
b _switch_to_user;


.balign 0x80
.section .text
.type lower_el_aarch64_irq, %function
lower_el_aarch64_irq:    // The exception handler for an IRQ from a lower EL
b .;                     // (AArch64).

.balign 0x80
.section .text
.type lower_el_aarch64_fiq, %function
lower_el_aarch64_fiq:    // The exception handler for an FIQ from a lower EL
b .;                     // (AArch64).

.balign 0x80
.section .text
.type lower_el_aarch64_serror, %function
lower_el_aarch64_serror:  // The exception handler for a System Error 
b .;                      // exception from a lower EL(AArch64).

.balign 0x80
.section .text
.type lower_el_aarch32_sync, %function
lower_el_aarch32_sync:   // The exception handler for a synchronous 
b .;                       // exception from a lower EL(AArch32).

.balign 0x80
.section .text
.type lower_el_aarch32_irq, %function
lower_el_aarch32_irq:    // The exception handler for an IRQ exception 
b .;                     // from a lower EL (AArch32).

.balign 0x80
.section .text
.type lower_el_aarch32_fiq, %function
lower_el_aarch32_fiq:    // The exception handler for an FIQ exception from 
b .;                     // a lower EL (AArch32).

.balign 0x80
.section .text
.type lower_el_aarch32_serror, %function
lower_el_aarch32_serror: // The exception handler for a System Error
b .;                     // exception from a lower EL(AArch32).

.section .text
.type exception_restore_half, %function
exception_restore_half:
el1_excep_restore_state;
eret;

