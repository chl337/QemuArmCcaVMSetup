#include "kernel.h"
#include "heap.h"
#include "errno.h"
#include "process.h"
//returns new fd
int append_fd(int pid, int host_fd)
{
	fd_table_t *ptr = kernel_struct.fd_head;
	int new_kfd = -1;
	
	for(; ptr; ptr=ptr->next) {
		if (ptr->pid == pid) {
			if ((new_kfd + 1) < ptr->k_fd) {
				new_kfd += 1;
				break;
			}
			new_kfd = ptr->k_fd;
		}
	}
	if (new_kfd == -1) {
		insert_fd(pid, host_fd, 0);
		return 0;
	}
	insert_fd(pid, host_fd, new_kfd);
	return new_kfd;
}

int insert_fd(int pid, int host_fd, int k_fd)
{
	fd_table_t *ptr = kernel_struct.fd_head;
	fd_table_t *new = 0;
	fd_table_t *target = 0;
	int pid_exists = 0;

	if (ptr == 0) {
		kernel_struct.fd_head = (fd_table_t*) kcalloc(sizeof(fd_table_t),1);
		if (kernel_struct.fd_head == 0) return -ENOMEM;
		kernel_struct.fd_head->pid = pid;
		kernel_struct.fd_head->host_fd = host_fd;
		kernel_struct.fd_head->k_fd = 0;
		return 0;
	}
	while(ptr) {
		if (ptr->pid == pid) {
			pid_exists = 1;
			if(ptr->host_fd == host_fd && ptr->k_fd == k_fd)
				return -EEXIST;
			if (ptr->k_fd > k_fd) {
				new = ptr;
				break;
			}
		}
		if(ptr->next) 
			ptr = ptr->next;
		else 
			break;
	}

	if(pid_exists) {
		ptr = kernel_struct.fd_head;
		for (; ptr->next && new; ptr=ptr->next) {
			if(ptr->next == new)
				break;
		}
	}

	if (ptr) {
		target  = kcalloc(sizeof(fd_table_t),1);
		if (target == 0) return -ENOMEM;
		target->pid = pid;
		target->host_fd = host_fd;
		target->k_fd = k_fd;
		ptr->next = target;
		target->next = new;
	}
	return 0;	
}

int get_host_fd(int pid, int k_fd) 
{
	fd_table_t *ptr = kernel_struct.fd_head;
	
	
	while(ptr) {
		if (ptr->pid == pid && ptr->k_fd == k_fd) 
			return ptr->host_fd;
		ptr = ptr->next;
	}
	return -ENOENT;
}

int delete_fd(int pid, int host_fd, int k_fd)
{
	fd_table_t *ptr = kernel_struct.fd_head;

	while(ptr) {
		if (ptr->pid == pid && ptr->host_fd == host_fd && ptr->k_fd == k_fd) 
			break;
		if(ptr->next) 
			ptr = ptr->next;
	}
	
	if (!ptr) return -ENOENT;	

	fd_table_t *prev = kernel_struct.fd_head;
	if (ptr == prev) {
		kernel_struct.fd_head = ptr->next;
		kfree(ptr);
		return 0;
	}
	while(prev->next != ptr) {
		prev = prev->next;
	}

	prev->next = ptr->next;
	kfree(ptr);
	return 0;
}
