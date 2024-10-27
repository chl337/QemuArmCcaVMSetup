#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include "syscall.h"
#include "hypercall.h"
#include "vma.h"
#include "heap.h"
#include "kernel.h"
#include "uart.h"

int open(const char *pathname, int flags, unsigned int mode)
{
	int val;


	uint64_t data = (uint64_t)get_physical_address((uint64_t)pathname);
	
	data = data & ~get_shared_bit();
	val = (int) hvc_call(HCALL_OPEN, data, flags, mode, 0, 0, 0);
	return val;
}

int64_t read(int fd, void *buf, size_t count)
{
	if (!buf) return -EINVAL;
	
	uint64_t addr = (uint64_t)get_physical_address((uint64_t)buf);
	addr = addr & ~get_shared_bit();
	if(addr == (uint64_t)-1) 
		return -EINVAL;
	
	return (int64_t) hvc_call(HCALL_READ, fd, addr, count, 0, 0, 0);
}

int64_t write(int fd, const void *buf, size_t count)
{
	if (!buf) return -EINVAL;
	uart_send_byte_array("\nexecuting sys  write\n\n",25);
	
	uint64_t addr = (uint64_t)get_physical_address((uint64_t)buf);
	addr = addr & ~get_shared_bit();
	if(addr == (uint64_t)-1) 
		return -EINVAL;
	
	return (int64_t) hvc_call(HCALL_WRITE, fd, addr, count, 0, 0, 0);

}

int64_t lseek(int fd, int64_t offset, int whence)
{
	return (int64_t) hvc_call(HCALL_LSEEK, fd, 0, 0, 0, 0, 0);
}

int close(int fd)
{
	return (int) hvc_call(HCALL_CLOSE, fd, 0, 0, 0, 0, 0);
}

int exit(int no)
{
	uart_send_byte_array("\nexecuting sys  exit   \n",25);
	return (int) hvc_call(HCALL_EXIT, no, 0, 0, 0, 0, 0);
}

int sys_open(const char *pathname, int flags, unsigned int mode)
{
	//access_ok
	char *new_path;
	int len;
	int ret;

	if (!pathname) return -EINVAL;

	len = strlen(pathname);

	if (len == 0) return -EINVAL;
	new_path = smalloc(len + 1);	

	if (!new_path) return -ENOMEM;	

	memcpy(new_path, pathname, len);

	ret = open(new_path, flags, mode);
	if (ret <  0) goto finish_sys_open;
	
	//get_current
finish_sys_open:
	sfree(new_path);
	return ret;
}

int64_t sys_read(int fd, void *buf, size_t count)
{
	int pid = get_current_process_pid();
	int host_fd = get_host_fd(pid, fd);	

	if (host_fd < 0) return -ENOENT;
	
	void *new_buffer = smalloc(count);
	if (new_buffer == 0) return -ENOMEM;
	
	size_t found = read(fd, new_buffer, count);

	if (found < 0) goto finish_sys_read;
	copy_to_user(buf, new_buffer, found);

finish_sys_read:
	sfree(new_buffer);
	return found;
}

int64_t sys_write(int fd, const void *buf, size_t count)
{
	int pid = get_current_process_pid();
	int host_fd = get_host_fd(pid, fd);	

	uart_send_byte_array("\ncheck host fd\n",14);
	if (host_fd < 0) return -ENOENT;
	
	void *new_buffer = (void*)smalloc(count);
	if (new_buffer == 0) return -ENOMEM;
	//access ok read <count>	
	uart_send_byte_array("\ncopy from user\n",16);
	size_t transferred = copy_from_user(new_buffer, buf, count);
	uart_send_byte_array("\ndone copying from user\n",24);
	if (transferred < 0) goto finish_sys_read;
	transferred = write(fd, new_buffer, count);

finish_sys_read:
	sfree(new_buffer);
	return transferred;
}

int64_t sys_lseek(int fd, int64_t offset, int whence)
{
	int pid = get_current_process_pid();
	int host_fd = get_host_fd(pid, fd);	
	
	if (host_fd < 0) return -ENOENT;

	return lseek(host_fd, offset, whence);	
}

int sys_close(int fd)
{
	int pid = get_current_process_pid();
	int host_fd = get_host_fd(pid, fd);	
	
	if (host_fd < 0) return -ENOENT;
	close(host_fd);
	
	delete_fd (pid, fd, host_fd);
	return 0;
}

int sys_exit(int value)
{
	exit(value);
	return 0;
}

uint64_t syscall(uint64_t nr, uint64_t arg0, uint64_t arg1, uint64_t arg2,
		 uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
	uint64_t ret;
	switch(nr) {
	case SYS_OPEN:
		ret = sys_open((const char *)arg0, (int)arg1, arg2);
		break;	
	case SYS_READ:
		ret = sys_read((int)arg0, (void*)arg1, arg2);
		break;	
	case SYS_WRITE:
		ret = sys_write((int)arg0, (const void *)arg1, arg2);
		break;	
	case SYS_LSEEK:
		ret = sys_lseek((int)arg0, (int64_t)arg1, (int)arg2);
		break;	
	case SYS_CLOSE:
		ret = sys_close((int)arg0);
		break;	
	case SYS_EXIT:
		ret = sys_exit((int)arg0);
		break;
	default:
		return -ENOSYS;
	}
	return ret;
}      

void kernel_system_call_entry_handler(context_t *ctx)
{
	
	uart_send_byte_array("\nexecuting systemcall\n",21);
	
	ctx->x[0] = (uint64_t) syscall(ctx->x[8], 
			ctx->x[0], ctx->x[1], ctx->x[2],
			ctx->x[3], ctx->x[4], ctx->x[5]);
}
