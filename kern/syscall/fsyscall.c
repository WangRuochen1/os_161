#include <types.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <thread.h>
#include <current.h>
#include <filetable.h>
#include <synch.h>
#include <vfs.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <proc.h>
#include <uio.h>
#include <kern/errno.h>
#include <vnode.h>
#include <syscall.h>
#include <copyinout.h>
#include <kern/seek.h>



//open file return file descripter
int open(const char *filename, int flags,int32_t* retval){

    struct vnode* ret_vn;

    //check if the file name is valid
    if(filename == NULL){
      return EFAULT;
    }
 //kprintf("flags = %d\n",flags);

    //check if the flag is valid//&& flags != O_WRONLY && flags !=O_CREAT && flags !=O_EXCL && flags !=O_TRUNC && flags !=O_APPEND
    if(flags != (O_RDWR|O_CREAT|O_TRUNC)&& flags !=21 && flags !=O_RDONLY && flags != O_WRONLY && flags !=O_CREAT && flags !=O_EXCL && flags !=O_TRUNC && flags !=O_APPEND){
        return EINVAL;
    }

/////////////
// get the path copy user in to kerel and get kernel dest
    char * dest = kmalloc(__PATH_MAX);
    size_t  actural;

    int result = copyinstr((const_userptr_t)filename,dest, __PATH_MAX,&actural);
    if(result){
        kfree(dest);
        return result;
    }
    //this actually open the file

//use path to open
//get ret_vn here
   int result_op = vfs_open(dest,flags,0,&ret_vn);

    if(result_op){
        kfree(dest);
       // kfree(actural);
        return result_op;
    }

   // int result_new;
    struct file_table_entry *entry = NULL;

        //add entry of file table
        entry = new_entry(curproc->fdTable,flags,ret_vn,retval);
        //kprintf("%d\n",*retval);
        //increase reference count
        if(entry == NULL){
            return ENOMEM;
        }
        increase_refcnt(entry);



      //change the file handle value to result new
      // *retval = result_new;

    //if flag = append
    if(flags == O_APPEND){
      //use vop_stat
      struct stat * stat_temp;
      stat_temp = kmalloc(sizeof(struct stat));
      VOP_STAT(entry->vn,stat_temp);

      lock_acquire(curproc->fdTable->fd_table_lk);
      entry->offset = stat_temp->st_size;
      lock_release(curproc->fdTable->fd_table_lk);

      kfree(stat_temp);
    }

    kfree(dest);
   // kfree(actural);
    return 0;

}


//lseek
int lseek (int fd, off_t pos, int whence,int32_t * retval1, int32_t * retval2){
   //get the entry and change offset depend on whence
   //int error;
  // int errno;

   //check fd
   if(check_fd_valid(curproc->fdTable,fd)==false ){
       return EBADF;
   }

   //fd refers to an object which does not support seeking.
    //bool (*vop_isseekable)(struct vnode *object);
    lock_acquire(curproc->fdTable->fd_table_lk);
    struct file_table_entry *temp_entry = curproc->fdTable->ft[fd];
    lock_release(curproc->fdTable->fd_table_lk);

   lock_acquire(temp_entry->table_lk);
   if(!VOP_ISSEEKABLE(temp_entry->vn)){
      lock_release(temp_entry->table_lk);
      return ESPIPE;
   }
    lock_release(temp_entry->table_lk);

   struct file_table_entry *entry = NULL;
    lock_acquire(curproc->fdTable->fd_table_lk);
    entry = curproc->fdTable->ft[fd];
    lock_release(curproc->fdTable->fd_table_lk);

   if(whence == SEEK_SET){
       //first get entry
       //then change offset
       lock_acquire(entry -> table_lk);
       if(pos < 0){
           lock_release(entry -> table_lk);
           return EINVAL;
       }
       entry->offset = pos;
       lock_release(entry -> table_lk);

   }else if(whence == SEEK_CUR){
       lock_acquire(entry -> table_lk);
       if(entry->offset + pos < 0){
           lock_release(entry -> table_lk);
           return EINVAL;
       }
       entry->offset += pos;
       lock_release(entry -> table_lk);

   }else if(whence == SEEK_END){
       struct stat * stat_temp = kmalloc(sizeof(struct stat));
       lock_acquire(entry -> table_lk);
       VOP_STAT(entry->vn,stat_temp);
       if(stat_temp->st_size + pos < 0){
           lock_release(entry -> table_lk);
           return EINVAL;
       }
       entry->offset = stat_temp->st_size + pos;
       lock_release(entry -> table_lk);
       kfree(stat_temp);
   }else{
       return EINVAL;
   }
   //finally return value
   lock_acquire(entry -> table_lk);
   off_t pt = entry->offset;
    lock_release(entry -> table_lk);
   *retval1 = pt >> 32; // higher 32 bits
   *retval2 = pt & 0xFFFFFFFF; // lower 32 bits
   return 0;
}

int close(int fd)
{
    struct fd_table *fdTable = curproc->fdTable;

    if(!check_fd_valid(fdTable,fd)){ //if not valid or not used
        return EBADF;
    }
    fd_free(fdTable,fd);
   // kprintf("close\n");
    return 0;
}


int read(int fd, const void *buf,size_t buflen, int *retval){
    struct fd_table *fd_table = curproc->fdTable;
    struct uio u;
    struct iovec iov;


    if(check_fd_valid(fd_table,fd) == false){ //if not valid or not used
        return EBADF;
    }
    lock_acquire(fd_table->fd_table_lk);
    struct file_table_entry *ft_entry = fd_table->ft[fd];
    lock_release(fd_table->fd_table_lk);

    lock_acquire(ft_entry->table_lk);
    if(ft_entry->status == O_WRONLY){
        lock_release(ft_entry->table_lk);
        return EBADF;
    }

    if(buf == NULL) return EFAULT;
    //set up a uio with the buffer
    uio_userinit(&iov, &u, buf, buflen,ft_entry->offset, UIO_READ);
   // KASSERT(u);



    int val = VOP_READ(ft_entry->vn, &u);
    if(val){
        lock_release(ft_entry->table_lk);
        return val;
    }
    *retval = buflen-u.uio_resid;
    ft_entry->offset += (buflen-u.uio_resid);
    lock_release(ft_entry->table_lk);

    return 0;
}

int write(int fd, const void *buf,size_t buflen, int *retval){
    struct fd_table *fd_table = curproc->fdTable;
    struct uio u;
    struct iovec iov;


    if(check_fd_valid(fd_table,fd) == false){ //if not valid or not used
        return EBADF;
    }
    lock_acquire(fd_table->fd_table_lk);
    struct file_table_entry * ft_entry = fd_table->ft[fd];
    lock_release(fd_table->fd_table_lk);

    lock_acquire(ft_entry->table_lk);
    if(ft_entry->status == O_RDONLY){
        lock_release(ft_entry->table_lk);
        return EBADF;
    }
    if(buf == NULL) return EFAULT;
    //set up a uio with the buffer
   // KASSERT(u);
    uio_userinit(&iov, &u, buf, buflen,ft_entry->offset, UIO_WRITE);


    int val = VOP_WRITE(ft_entry->vn, &u);
    if(val){
        lock_release(ft_entry->table_lk);
        return val;
    }
    *retval = buflen-u.uio_resid;
    ft_entry->offset += (buflen-u.uio_resid);
    lock_release(ft_entry->table_lk);

    return 0;
}

int dup2(int oldfd, int newfd, int *retval){
    if(oldfd<0 || oldfd>=__OPEN_MAX || newfd<0 || newfd>=__OPEN_MAX) return EBADF;
    struct fd_table *fd_table = curproc->fdTable;
    struct file_table_entry *ft_entry;

    if(!check_fd_valid(fd_table,oldfd)){

        return EBADF;
    }
    ft_entry=fd_table->ft[oldfd];
    if(!check_fd_valid(fd_table,newfd)){
        fd_free(fd_table,newfd);
    }
    lock_acquire(fd_table->fd_table_lk);
    fd_table->ft[newfd] = ft_entry;
    lock_acquire(ft_entry->table_lk);
    increase_refcnt(ft_entry);
    lock_release(ft_entry->table_lk);
    lock_release(fd_table->fd_table_lk);
    *retval = newfd;
    return 0;
}


int
chdir(const char *pathname){
    if(pathname == NULL){
        return EFAULT;
    }
    char * dest = kmalloc(__PATH_MAX);
    size_t actural;
    int result = copyinstr((const_userptr_t)pathname,dest,__PATH_MAX,&actural);
    if(result){
        kfree(dest);
        return result ;
    }
    int result1 = vfs_chdir(dest);
    if(result1){
        kfree(dest);
        return result1;
    }
   kfree(dest);
    return 0;
}

int __getcwd(char *buf, size_t buflen, int *retval){
    struct iovec iov;
    struct uio u;
    iov.iov_ubase = (userptr_t)buf;
    iov.iov_len = buflen;
    u.uio_iov = &iov;
    u.uio_iovcnt = 1;
    u.uio_offset = 0;
    u.uio_resid = buflen;
    u.uio_segflg = UIO_USERSPACE;
    u.uio_rw = UIO_READ;
    u.uio_space = curproc->p_addrspace;
    int ret = vfs_getcwd(&u);
    if(ret) return ret;
    *retval = buflen-u.uio_resid;
    return 0;
}
