//
// Created by 王小狗 on 2019-10-13.
//
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
#include <synch.h>



struct fd_table*
fd_table_init(void){

    struct fd_table* fdTable = kmalloc(sizeof(struct fd_table));
    if(fdTable == NULL){
        return NULL;
    }
    //initialize all value to NULL
    for(int i = 0; i < 32; i++){
        fdTable->ft[i] = NULL;
    }
    //initialize the lock
    const char *name1 = "fd_table_lock";
    fdTable->fd_table_lk =lock_create(name1);
    // fdTable->current_index = 0;
    return fdTable;
}

struct file_table_entry *
entry_init(int flags){
    struct file_table_entry *entry = kmalloc(sizeof(struct file_table_entry));
    const char *name1 = "fd_table_lock";
    entry->table_lk = lock_create(name1);
    struct vnode *vn = NULL;
    entry->vn = vn;
    off_t offset = 0;
    entry->offset = offset;
    entry->status = flags;
    entry->ref_cnt = 0;
    return entry;

}

//init the file table entry to stdio
//init connect the stdio with the file describer table
void
connect_std(struct fd_table *fdTable, struct file_table_entry *entry0,struct file_table_entry *entry1,struct file_table_entry *entry2){
    struct vnode *stdin;
    struct vnode *stdout;
    struct vnode *stderr;
    const char *cons = "con:";

    vfs_open(kstrdup(cons),O_RDONLY,0,&stdin);
    vfs_open(kstrdup(cons),O_WRONLY,0,&stdout);
    vfs_open(kstrdup(cons),O_WRONLY,0,&stderr);
    //create entry
    entry0 = entry_init(O_RDONLY);
    entry1 = entry_init(O_WRONLY);
    entry2 = entry_init(O_WRONLY);

    lock_acquire(entry0->table_lk);
    entry0->vn = stdin;
    lock_release(entry0->table_lk);

    lock_acquire(entry1->table_lk);
    entry1->vn = stdout;
    lock_release(entry1->table_lk);

    lock_acquire(entry2->table_lk);
    entry2->vn = stderr;
    lock_release(entry2->table_lk);

//connect them together
    fdTable->ft[0] = entry0;
    fdTable->ft[1] = entry1;
    fdTable->ft[2] = entry2;
//fbTable->current_index = 3;

//increase reference count
    increase_refcnt(entry0);
    increase_refcnt(entry1);
    increase_refcnt(entry2);
}

struct file_table_entry*
new_entry(struct fd_table* fdTable, int flags,struct vnode* ret_vn,int32_t * retval){

    /*find an empty space*/
    struct file_table_entry* entry = NULL;
    int pos = 0;
    for(int i = 3; i < 32; i++){
        if(fdTable->ft[i] == NULL){
            pos = i;
            *retval = i;
            break;
        }
    }
    if(pos == 0){
        return NULL;
    }
    //init a new entry
     entry = entry_init(flags);
    if(entry == NULL){
        return NULL;
    }

    // change vn to ret_vn
    lock_acquire(fdTable->fd_table_lk);

    fdTable->ft[pos] = entry;

    lock_acquire(fdTable->ft[pos]->table_lk);
    fdTable->ft[pos]->vn = ret_vn;
    lock_release(fdTable->ft[pos]->table_lk);

    lock_release(fdTable->fd_table_lk);
    return entry;
}

void
increase_refcnt(struct file_table_entry *entry){
    entry->ref_cnt++;
}

int check_fd_valid(struct fd_table* fdTable, int fd){

    lock_acquire(fdTable->fd_table_lk);
    if(fdTable->ft[fd] == NULL || fd < 0 ||fd >=32){
        lock_release(fdTable->fd_table_lk);
        return false;
    }
    lock_release(fdTable->fd_table_lk);
    return true;
}

//ft functions
void fd_free(struct fd_table *fd_table, int fd){
    KASSERT(!(fd<0|| fd>=__OPEN_MAX));
    KASSERT(fd_table!=NULL);
    if(fd_table->ft[fd]==NULL) return;
    struct file_table_entry *ft_entry = fd_table->ft[fd];
    decrement_refcount(ft_entry);
    fd_table->ft[fd]=NULL;
}
void decrement_refcount(struct file_table_entry *ft_entry){
    KASSERT(ft_entry!=NULL);
    lock_acquire(ft_entry->table_lk);

        ft_entry->ref_cnt --;
        if(ft_entry->ref_cnt == 0) {
            //delete a file_table_entry
            vfs_close(ft_entry->vn);
            lock_release(ft_entry->table_lk);
            lock_destroy(ft_entry->table_lk);
            kfree(ft_entry);
            ft_entry = NULL;
            return;
        }

   //
   //
    lock_release(ft_entry->table_lk);
}

void fd_table_destroy(struct fd_table* fdTable){
    lock_destroy(fdTable->fd_table_lk);
    kfree(fdTable);
    fdTable = NULL;
}

int file_table_copy(struct fd_table* old_table, struct fd_table* new_table){

    //link the file table with the all file table entry
   for(int i = 0; i <__OPEN_MAX; i++){
       if(old_table->ft[i] == NULL){
           continue;
       }
      new_table->ft[i] = old_table->ft[i]; // assign file pointer to new process
      increase_refcnt(old_table->ft[i]); // increase reference cnt
   }
   return 0;
}