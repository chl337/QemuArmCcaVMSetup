#ifndef _HYPERCALL_H
#define _HYPERCALL_H

#include <stdint.h>

#define HVC_BASE	0x8000	//16 bit immediate
#define HCALL_OPEN	(HVC_BASE | 1)
#define HCALL_READ	(HVC_BASE | 2)
#define HCALL_WRITE	(HVC_BASE | 3)
#define HCALL_LSEEK	(HVC_BASE | 4)
#define HCALL_CLOSE	(HVC_BASE | 5)
#define HCALL_EXIT	(HVC_BASE | 6)

#define HVC_OPEN	(HVC_BASE | 1)
#define HVC_READ	(HVC_BASE | 2)
#define HVC_WRITE	(HVC_BASE | 3)
#define HVC_LSEEK	(HVC_BASE | 4)
#define HVC_CLOSE	(HVC_BASE | 5)
#define HVC_EXIT	(HVC_BASE | 6)

#define HVC_SYSCALL_HV 0
#define HVC_SYSCALL_IO 1

void hvc_call_exit_fatal(uint64_t reason);

uint64_t hvc_call(uint64_t hvc_code,
	uint64_t arg0, uint64_t arg1, uint64_t arg2,
	uint64_t arg3, uint64_t arg4, uint64_t arg5);

int get_hypercall_mode();
void set_hypercall_mode(int new_mode);

#endif
