#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "lib/string.h"
#include "lib/syscall-nr.h"

static void syscall_handler (struct intr_frame *);
void halt (void);
void exit (int);
pid_t exec (const char *);
int wait (pid_t);
bool create (const char *, unsigned);
bool remove (const char *);
int open(const char *);
int filesize (int);
int read (int, void *, unsigned);
int write (int, const void *, unsigned);
void seek (int, unsigned);
unsigned tell (int);
void close (int);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  void *esp = f->esp;
  switch (&esp)
  {
  case SYS_HALT:
    halt ();
    break;
  case SYS_EXIT:
    esp += 4;
    int status = &esp;
    exit (status);
    break;
  case SYS_EXEC:
    esp += 4;
    const char *cmd = esp;
    exec (cmd);
    break;
  case SYS_WAIT:
    esp += 4;
    pid_t pid = &esp;
    wait (pid);
    break;
  case SYS_CREATE:
    esp += 4;
    const char *file = esp;
    esp += 4;
    unsigned size = &esp;
    create (file, size);
    break;
  case SYS_REMOVE:
    esp += 4;
    const char *file = esp;
    remove (file);
    break;
  case SYS_OPEN:
    esp += 4;
    const char *file = esp;
    open (file);
    break;
  case SYS_FILESIZE:
    esp += 4;
    int fd = &esp;
    filesize (fd);
    break;
  case SYS_READ:
    esp += 4;
    int fd = &esp;
    esp += 4;
    void *buffer = &esp;
    esp += 4;
    unsigned size = &esp;
    read (fd, buffer, size);
    break;
  case SYS_WRITE: 
    esp += 4;
    int fd = &esp;
    esp += 4;
    void *buffer = &esp;
    esp += 4;
    unsigned size = &esp;
    write (fd, buffer, size);
    break;
  case SYS_SEEK:
    esp += 4;
    int fd = &esp;
    esp += 4;
    unsigned position = &esp;
    seek (fd, position);
    break;
  case SYS_TELL:
    esp += 4;
    int fd = &esp;
    tell (fd);
    break;
  case SYS_CLOSE:
    esp += 4;
    int fd = &esp;
    close (fd);
    break;
  }
}

void
halt (void)
{
  shutdown_power_off();
}

void
exit (int status)
{
  process_exit ();
}

pid_t
exec (const char *cmd_line)
{
  char *save_ptr;
  return process_execute (strtok_r (cmd_line, " ", save_ptr));
}

int
wait (pid_t pid)
{
  return process_wait (pid);
}

bool
create (const char *file, unsigned size)
{

}

bool
remove (const char *file)
{

}

int
open (const char *file)
{

}

int
filesize (int fd)
{

}

int
read (int fd, void *buffer, unsigned size)
{

}

int
write (int fd, const void *buffer, unsigned size)
{

}

void
seek (int fd, unsigned position)
{

}

unsigned 
tell (int fd)
{

}

void
close (int fd)
{

}
