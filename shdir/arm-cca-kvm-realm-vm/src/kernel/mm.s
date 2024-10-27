//add the following before start and after .text, if any
.equ PSCI_AFFINITY_INFO	, 0xC4000004;
.equ PSCI_CPU_OFF	, 0x84000002;
.equ PSCI_CPU_ON	, 0xC4000003;
.equ PSCI_CPU_SUSPEND	, 0xC4000001;
.equ PSCI_FEATURES	, 0x8400000A;	
.equ PSCI_SYSTEM_OFF	, 0x84000008;
.equ PSCI_SYSTEM_RESET	, 0x84000009;
.equ PSCI_VERSION	, 0x84000000;

.macro do_psci cmd:req, arg0:req, arg1:req, arg2:req, arg3:req
	movl  x0, \cmd;
	movl  x1, \arg1;
	movl  x2, \arg2;
	movl  x3, \arg3;
	smc 0;
.endm

// load a 64-bit immediate using mov
.macro movq Xn, imm
    movz    \Xn,  \imm & 0xFFFF
    movk    \Xn, (\imm >> 16) & 0xFFFF, lsl 16
    movk    \Xn, (\imm >> 32) & 0xFFFF, lsl 32
    movk    \Xn, (\imm >> 48) & 0xFFFF, lsl 48
.endm

// load a 32-bit immediate using MOV
.macro movl Wn, imm
    movz    \Wn,  \imm & 0xFFFF
    movk    \Wn, (\imm >> 16) & 0xFFFF, lsl 16
.endm

.text 
.global _start
.global hello_msg 
_start: 
mov x4, 0x1;
lsl x4, x4, 32;
mov x1, 0x9000000;
orr x5, x1, x4;
mov x7, #0x301;
strh w7,[x5,0x30]; 

mrs x17,ID_AA64PFR0_EL1;
mov x18, 64;

print_id:
mov x19, 0xf;
sub x18, x18, 4;

lsl x19, x19, x18;
and x19, x17, x19;
lsr x19, x19, x18;
add x19, x19, 0x30;
strb w19, [x5];

cmp x18, 0;
bne print_id;

mov w4,#0x0a;
strb w4, [x5];
mov w4,#0x0d;
strb w4, [x5];

mov x16, 10;
loop:
	mov w4,#0x68;
	strb w4, [x5];
	mov w4,#0x65;
	strb w4, [x5];
	mov w4,#0x6c;
	strb w4, [x5];
	mov w4,#0x6c;
	strb w4, [x5];
	mov w4,#0x6f;
	strb w4, [x5];
	mov w4,#0x0a;
	strb w4, [x5];
	mov w4,#0x0d;
	strb w4, [x5];

mov x0, 0x1;
lsl x0, x0, 32;
movz x5, 0x0000;
movk x5, 0x900,lsl 16;
orr x1, x5, x0;
mov x5, x1;
mov x7, #0x301;
strh w7,[x5,0x30]; 
	mov w4,#0x77;
	strb w4, [x5];
	mov w4,#0x6F;
	strb w4, [x5];
	mov w4,#0x72;
	strb w4, [x5];
	mov w4,#0x6C;
	strb w4, [x5];
	mov w4,#0x64;
	strb w4, [x5];
	mov w4,#0x0a;
	strb w4, [x5];
	mov w4,#0x0d;
	strb w4, [x5];
	
	subs x16, x16, 1;	
	bne loop
shutdown_now:
	do_psci PSCI_SYSTEM_OFF, 0 , 0, 0,0;

