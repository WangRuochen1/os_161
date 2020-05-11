#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <types.h>
#include <array.h>
#include <synch.h>
#include <proc.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <thread.h>
#include <current.h>
#include <filetable.h>
#include <vfs.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <uio.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <vnode.h>
#include <syscall.h>
//file table
struct file_table_entry {
    struct lock*table_lk;
    struct vnode *vn;
    int status;
    off_t offset;
    int ref_cnt;
};

//file describer table
struct fd_table{
    struct file_table_entry *ft[__OPEN_MAX];
    struct lock*fd_table_lk;
   // int current_index;

};

//init the file describer table
struct fd_table * fd_table_init(void);
//init the file table entry
struct file_table_entry* entry_init(int flags);
//init the first 3 entry of file describer table to stdio
void connect_std(struct fd_table *fdTable,struct file_table_entry *entry0,struct file_table_entry *entry1, struct file_table_entry *entry2);
//add an new entry to file table
struct file_table_entry* new_entry(struct fd_table* fdTable, int flags,struct vnode* ret_vn,int32_t * retval);
//increase reference cnt
void increase_refcnt(struct file_table_entry *entry);
//check if our fd is valid
int check_fd_valid(struct fd_table* fbTable, int fd);
//free the fd
void fd_free(struct fd_table *fd_table, int fd);
//decrease ref count
void decrement_refcount(struct file_table_entry *ft_entry);
//destroy file descriptor table
void fd_table_destroy(struct fd_table* ftTable);

int file_table_copy(struct fd_table* old_table, struct fd_table* new_table);

#endif