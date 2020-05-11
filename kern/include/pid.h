#ifndef _PID_H_
#define _PID_H_

#include <types.h>
#include <array.h>
#include <synch.h>
#include <proc.h>
#include <kern/limits.h>
struct pid_struct{
    struct lock*pid_lk;
    struct proc * pid_array[__PID_MAX];
    struct cv* pid_cv;
    int exit_array[__PID_MAX];
};
#endif