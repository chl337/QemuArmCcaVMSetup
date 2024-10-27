#include "hypercall.h"
#include "errno.h"
#include "heap.h"
#include "uart.h"

static int mode = HVC_SYSCALL_IO;
extern uint64_t get_physical_address(uint64_t);
extern uint64_t get_shared_bit();
void set_hypercall_mode(int new_mode) 
{
	mode = 0x11 & new_mode; 
}

int get_hypercall_mode() 
{
	return mode;
}

#define hvc_hypercall_macro(name, hvc_code) \
uint64_t hypercall_ ##name (uint64_t arg0, uint64_t arg1, uint64_t arg2, \
				uint64_t arg3, uint64_t arg4, uint64_t arg5)  \
{\
	register uint64_t x0 asm("x0");\
	asm ( "hvc %c[HV_CODE];":: [HV_CODE]"i" ( hvc_code ) : "memory");\
	return x0;\
}\


hvc_hypercall_macro( open,  HVC_OPEN)
hvc_hypercall_macro( read,  HVC_READ)
hvc_hypercall_macro( write, HVC_WRITE)
hvc_hypercall_macro( lseek, HVC_LSEEK)
hvc_hypercall_macro( close, HVC_CLOSE)
hvc_hypercall_macro( exit,  HVC_EXIT)

static char io_hc[] = "\ngoing to send address for syscall\n";
uint64_t io_hypercall(  uint64_t hvc_code, uint64_t arg0, uint64_t arg1, uint64_t arg2,
		uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
	uint64_t* io_call = (uint64_t*)scalloc((sizeof(uint64_t)), 7);
	if (io_call < 0 ) return (uint64_t) (-ENOMEM);

	io_call[0] = hvc_code;
	io_call[1] = arg0;
	io_call[2] = arg1;
	io_call[3] = arg2;
	io_call[4] = arg3;
	io_call[5] = arg4;
	io_call[6] = arg5;
	
	uint64_t address = (uint64_t)get_physical_address((uint64_t)io_call);
	address = address & ~get_shared_bit();
	uart_send_byte_array(io_hc,strlen(io_hc));
	uart_send_byte_array((char*)&address,8);
	uart_syscall_write(address);
	/*syscall handling happens in this function after sending byte 8*/

	/*this is a synchronous hypercall that */
	uint64_t ret_val = ((uint64_t*)io_call)[0];
	uart_send_byte_array((char*)"\nreturning from open: doing sfree \n",34);
	sfree(io_call);
	uart_send_byte_array((char*)"\nreturning from open hypercall\n",30);
	return ret_val;
}

uint64_t hvc_hypercall(  uint64_t hvc_code, uint64_t arg0, uint64_t arg1, uint64_t arg2,
		uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
	switch(hvc_code) {
		case HVC_OPEN:
			return hypercall_open(arg0, arg1, arg1, arg3, arg4, arg5);
		case HVC_READ:
			return hypercall_read(arg0, arg1, arg1, arg3, arg4, arg5);
		case HVC_WRITE:
			return hypercall_write(arg0, arg1, arg1, arg3, arg4, arg5);
		case HVC_LSEEK:
			return hypercall_lseek(arg0, arg1, arg1, arg3, arg4, arg5);
		case HVC_CLOSE:
			return hypercall_close(arg0, arg1, arg1, arg3, arg4, arg5);
		case HVC_EXIT:
			return hypercall_exit(arg0, arg1, arg1, arg3, arg4, arg5);
		default:
			break;
	}
	return (uint64_t) -EINVAL;
}

uint64_t hvc_call(  uint64_t hvc_code, uint64_t arg0, uint64_t arg1, uint64_t arg2,
		uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
	if (mode == HVC_SYSCALL_HV) 
		return hvc_hypercall(hvc_code, arg0, arg1, arg2, arg3, arg4, arg5);

	return io_hypercall(hvc_code, arg0, arg1, arg2, arg3, arg4, arg5);
}

void hvc_call_exit(uint64_t reason)
{
	hvc_call(HVC_EXIT,reason,0,0,0,0,0);
}
void hvc_call_exit_fatal(uint64_t reason)
{
	hvc_call(HVC_EXIT,reason,0,0,0,0,0);
}
