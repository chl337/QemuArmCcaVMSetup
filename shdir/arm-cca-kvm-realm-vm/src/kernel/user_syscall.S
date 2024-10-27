.equ SYS_read  ,0x3
.equ SYS_write ,0x4
.equ SYS_open  ,0x5
.equ SYS_close ,0x6

.type _start, %function
.global _start
_start:
hvc;
//save initial start
mov  x8,x0;
mov  x9,x1;
mov x10,x2;
mov x11,x3;
mov x12,x4;
mov x13,x5;
mov x14,x6;
mov x15,x7;
/*initialize with syscall values*/
mov x0, SYS_read;
mov x1, 0;
mov x2, 0;
mov x3, 0;
mov x4, 0;
mov x5, 0;
mov x6, 0;
mov x7, 0;
str x0, [x8]
ldr x0, [x8]
wfi
b .;

