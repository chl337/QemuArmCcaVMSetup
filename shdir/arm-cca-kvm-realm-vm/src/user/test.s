.text
.equ SYS_Write, 64;
.equ SYS_Exit,  93;
.global message
.global _start
.type _start, %function
_start:
	mov x0, 0;
	adr x1, message;
	mov x2, 16; 
	mov x8, SYS_Write;
	svc 0;
	mov x0, 42;
	mov x8, SYS_Exit;
	svc 0;
	b .;
message:
.ascii "hello from user\n"
