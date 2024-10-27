#ifndef _SYSCALL_H
#define _SYSCALL_H
#include "common.h"

int open(const char *pathname, int flags, unsigned int mode);
int64_t read(int fd, void *buf, size_t count);
int64_t write(int fd, const void *buf, size_t count);
int64_t lseek(int fd, int64_t offset, int whence);
int close(int);
int sys_execve(const char *path, char *const argv[], char *const envp[]);
void kernel_system_call_entry_handler(context_t *ctx);

#define SYS_OPEN	56
#define SYS_READ	63
#define SYS_WRITE	64
#define SYS_LSEEK	61
#define SYS_CLOSE	57
#define SYS_EXIT	93

#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002

int sys_open(const char *pathname, int flags, unsigned int mode);
int64_t sys_write(int fd, const void *buf, size_t count);
int64_t sys_lseek(int fd, int64_t offset, int whence);
int64_t sys_read(int fd, void *buf, size_t count);
int sys_close(int fd);
#endif
