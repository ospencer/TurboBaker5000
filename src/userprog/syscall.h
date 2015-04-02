#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;
typedef int mapid_t;

void syscall_init (void);
void exit (int exit_code); 


#endif /* userprog/syscall.h */
