/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <cdefs.h> /* for __DEAD */
#include <kern/limits.h>
#include <addrspace.h>
struct trapframe; /* from <machine/trapframe.h> */
extern struct semaphore * fork_sem;
//int arg_pos[4000]; // store argument position in kernel buff
//char argv[__ARG_MAX]; // store argument in kernel buff
/*
 * The system call dispatcher.
 */

void syscall(struct trapframe *tf);

/*
 * Support functions.
 */

/* Helper for fork(). You write this. */
void enter_forked_process(struct trapframe *tf);

/* Enter user mode. Does not return. */
__DEAD void enter_new_process(int argc, userptr_t argv, userptr_t env,
		       vaddr_t stackptr, vaddr_t entrypoint);


/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys___time(userptr_t user_seconds, userptr_t user_nanoseconds);
//function implement in asst4
int open(const char *filename, int flags, int32_t* retval);
int close(int fd);
int read(int fd, const void *buf,size_t buflen, int *retval);
int write(int fd, const void *buf,size_t buflen, int *retval);
int lseek (int fd, off_t pos, int whence,int32_t * retval1, int32_t * retval2);
int dup2(int oldfd, int newfd, int *retval);
int chdir(const char *pathname);
int __getcwd(char *buf, size_t buflen, int *retval);
int fork(struct trapframe * parent_tf, int32_t *retval);
void child_entry(void* data1, unsigned long data2);
int getpid(int32_t *retval);
int _exit(int32_t exitcode);
int waitpid(pid_t pid, int32_t * status, int32_t options, int32_t * retval);
//int execv(const char *program, char **args);
//int copy_arg(char ** args,int * pos,int * num_arg);
//int copy_out_arg(vaddr_t* ptr,int * pos,int* num_arg,struct addrspace *as);
//void change_child_status(void);
#endif /* _SYSCALL_H_ */
