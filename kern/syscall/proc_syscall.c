#include<types.h>
#include<proc.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <thread.h>
#include <current.h>
#include <filetable.h>
#include <synch.h>
#include <kern/limits.h>
#include <syscall.h>
#include <array.h>
#include <spinlock.h>
#include <addrspace.h>
#include <filetable.h>
#include <mips/trapframe.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <kern/wait.h>
#include <copyinout.h>
//struct semaphore * fork_sem;
int
fork(struct trapframe * parent_tf, int32_t *retval){

    struct proc *child_proc;
    int result =proc_fork(&child_proc);

   //fork_sem  = sem_create("fork",0);


    //copy file table: not really copy
    result= file_table_copy(curproc->fdTable, child_proc->fdTable);
    if(result){
        return result;
    }
//    kprintf("current_proc pid:%d\n",curproc->pid);
//    kprintf("child_proc pid:%d\n",child_proc->pid);
    //int len = array_num(curproc->child_array);
   // kprintf("child array length: %d\n",len);
   // kprintf("\n");

    //set up trapframe
    struct trapframe *child_tf;
    child_tf = kmalloc(sizeof(struct trapframe));
    if(child_tf == NULL){
        return ENOMEM;
    }
   // *child_tf = *parent_tf;
   memcpy((void*)child_tf,(const void*)parent_tf, sizeof(struct trapframe));
    child_tf->tf_v0 = 0;
    child_tf->tf_a3 = 0;
    child_tf->tf_epc += 4;


    int ret = thread_fork("child thread", child_proc, child_entry, child_tf, (unsigned long)child_proc->p_addrspace);
    if (ret) {
        return ret;
    }

   // P(fork_sem);
    *retval = child_proc->pid;
    return 0;
}

void child_entry(void* data1, unsigned long data2){
    (void)data2;
    //copy child trapframe from heap to stack (check same stack in mips_usermode)

    struct trapframe tf = *(struct trapframe *)data1;
    memcpy(&tf, (const void *) data1, sizeof(struct trapframe));
    kfree((struct trapframe *) data1);
    as_activate();
   // V(fork_sem);
    mips_usermode(&tf);
}

int
getpid(int32_t *retval){
    lock_acquire(pid_struct->pid_lk);
    *retval = curproc->pid;
    lock_release(pid_struct->pid_lk);
    return 0;
}

int
_exit(int32_t exitcode){

  pid_t pid= curproc->pid;

  lock_acquire(pid_struct -> pid_lk);
  pid_struct ->exit_array[pid] =_MKWAIT_EXIT(exitcode);
  cv_broadcast(pid_struct->pid_cv, pid_struct->pid_lk);
  lock_release(pid_struct->pid_lk);

  //thread_exit();
   thread_exit1(curproc);
return 0;
}



int
waitpid(pid_t pid, int32_t * status, int32_t options,int32_t * retval){
 //   kprintf("wait_pid\n");
    int result = 0;
    int exitcode = 0;
    int child_index = -1;

    //check if the pid parameter is the child of curproc
//    int length = array_num(curproc->child_array);
    int child_check = 0;

    for(int i = 0; i < 20; i++){
        if(curproc->pid_child[i] == pid){
            child_check = 1;
            child_index = i;
            break;
        }
    }
        if(child_check == 0){
        return ECHILD;
    }

    //check if parameter pid is myself
    if(curproc->pid == pid){
        return ECHILD;
    }
    //check if the pid is valid
    if(pid < __PID_MIN || pid > __PID_MAX){
        return ESRCH;
    }
    //check is the pid value has a valid process
    if(pid_struct->pid_array[pid] == NULL){
        return ESRCH;
    }
    if(options != 0){
        return EINVAL;
    }
    //check if status is
    if(status == NULL){
        return 0;
    }
    //check the pointer align
    if((int)status % 4 != 0){
        return EFAULT;
    }



    lock_acquire(pid_struct->pid_lk);
    int check = pid_struct->exit_array[pid];
   // struct proc* this_proc = pid_struct -> pid_array[pid];
   //-1 the exit array does not modified which means the process does not exit
    if(pid_struct->exit_array[pid] != -1){
        exitcode = pid_struct->exit_array[pid];
           result = copyout(&exitcode,(userptr_t)status, sizeof(int32_t));
           if(result) {
               lock_release(pid_struct->pid_lk);
               return result;
           }
    }else{
        while(check == -1) {
            cv_wait(pid_struct->pid_cv, pid_struct->pid_lk);
            check = pid_struct->exit_array[pid];
        }
        exitcode = pid_struct->exit_array[pid];
            result = copyout(&exitcode,(userptr_t)status, sizeof(int32_t));
            if(result){
                lock_release(pid_struct->pid_lk);
                return result;
            }
    }
    pid_struct->pid_array[pid] = NULL;
    pid_struct->exit_array[pid] = -1;
   // array_remove(curproc->child_array,child_index);
    curproc->pid_child[child_index] = -1;

    lock_release(pid_struct->pid_lk);
    *retval = pid;

    return 0;
}





