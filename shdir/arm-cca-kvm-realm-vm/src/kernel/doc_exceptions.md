# Exception Handling in Aarch64 (Arm 64 bit)
##**Exceptions**
Aarch64 architecture considers both standard *x86* interrupts and exceptions as just exceptions.

1. Exception producing instruction	
	- HVC
	- SMC
	- SVC
2. Synchronous and Asynchronous exceptions
Synchronous is
	a. is precise
	b. occurs due to/attempted execution of instructions
	c. return address has relationship with exception causing instruction

Asynchronous == (not synchronous)

## List of synchronous exceptions:
 1. Software Step exception.
 2. PC Alignment Fault exception.
 3. Instruction Abort exception.
 4. Breakpoint exception.
 5. Address Matching Vector Catch exception.
 6. Illegal Execution state exception.
 7. Software Breakpoint exception.
 8. Branch Target exception.
 9. An Instruction Abort exception.
10. A Data Abort exception.
11. A PC alignment fault exception.
12. A Watchpoint exception
13. A Translation fault on a stage 2 translation
14. A Access flag fault on a stage 2 translation
15. A stage 2 Address Size fault
16. A fault on stage 2 translation of an address access in a stage 1 TTW
17. A Granule Protection Fault (GPF) on access for a stage 2 translation table
##note
1. 9-12 cause the virtual address to be written to *FAR\_ELx*.
2. GPC protection, exception is captured in *FAR\_EL3*.
3. Intermediate physical address is written to *HPFAR\_EL2* for 13-17 
4. For GPC exception, a PA that characterizes the exception is captured in *MFAR\_EL3*.

## Exception vectors 
Area contains code that executes when exception is taken code that corresponds to exception category.
An exceptions is one of the following:
1. Synchronous exception
2. SError
3. IRQ
4. FIQ
Above are categorised based on the exception level that it occured in, the stack pointer to be used
and state of register file.

##Exception Entry
When an exception occurs, the following happens:
1. PSTATE before exception stored in *SPSR\_ELx*.
2. **Return Address** is stored in *ELR\_ELx*.
3. **Exception Syndrome** is stored in *ESR\_ELx* (synchronous exception and SError interrupts). 
4. Execution starts from exception vector at target Exception Level.

## Contents of PSTATE after taking Exception
a. PSTATE.EL --> *set to target execution*
b. PSTATE.{D,A,I,F,SP,TCO} set to 1
c. PSTATE.SSBS == SCTLR\_ELx.DSSBS
d. PSTATE.{IL,nRw,UAO} are set to 0
e. PSTATE.BTYPE == 0b00
f. PSTATE.SS is set according to self-hosted debug
g. PSTATE.PAN is set to 1 if:
	a. Target exception is EL1 and SCTLR\_EL1.SPAN is 0
	b. Exception from El0, target EL2, Secure EL2 on, HCR_EL2.{TGE,E2H} == {1,1} and SCTLR_EL2.SPAN == 0.
	c. PSTATE.ALLINT == inverse( SCTLR.ELx.SPINTMASK )


## PSTATE register
```c
struct *SPSR\_EL1* {
/*AArch64 Exception level and stack pointer*/
/*0b0000 - EL0t*/
/*0b0100 - EL1t*/
/*0b0101 - EL2t*/
	uint64_t M:4;
/*Execution state. set to 0b0 == AArch64 execution state*/
	uint64_t M:1;
	uint64_t Reserved:1;
/*FIQ interrupt mask.*/
	uint64_t F:1;
/*IRQ interrupt mask.*/
	uint64_t I:1;
/*SError interrupt mask*/
	uint64_t A:1;
/*Debug exception mask*/
	uint64_t D:1;
/*FEAT-BTI needed. branch type indicator*/
	uint64_t BPTYPE:2;
/*FEAT-SSBS needed. speculative store bypass*/
	uint64_t SSBS:1;
/*FEAT-NMI needed. */
	uint64_t ALLINT:1;
	uint64_t reserved:6;
/*Illegal Execution state*/
	uint64_t IL::1;
/*Software step*/
	uint64_t SS:1;
/*FEAT-PAN: privileged access never*/
	uint64_t PAN:1;
/*FEAT UAO: user access override*/
	uint64_t UAO:1;
/*FEAT-DTI data independent timing*/
	uint64_t DIT:1;
/*FEAT-MTE Tag check override */
	uint64_t TCO:1;
/*Overflow flag*/
	uint64_t V:1;
/*carry condition flag*/
	uint64_t C:1;
/*Zero condition flag*/
	uint64_t Z:1;
/*negative condition flag*/
	uint64_t N:1;
	uint64_t reserved:32;
};
```
## Registers to check
1. *ESR\_ELx* exception syndrome register
2. *ELR\_ELx* return address register
3. *SPSR\_ELx* processor state before exception
