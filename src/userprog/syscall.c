#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "lib/string.h"
#include "lib/syscall-nr.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

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
struct file *fds[128];

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  uint32_t *esp = f->esp;
  if(*esp == NULL || is_kernel_vaddr(esp) ||
     !pagedir_is_mapped(thread_current()->pagedir, esp))
  {
    exit (-1);
  }
  const char *file;
  int fd;
  int status;
  unsigned size;
  const char *cmd;
  void *buffer;
  switch (*esp)
  {
  case SYS_HALT:
    halt ();
    break;
  case SYS_EXIT:
    esp += 4;
    status = &esp;
    exit (status);
    break;
  case SYS_EXEC:
    esp += 4;
    cmd = esp;
    exec (cmd);
    break;
  case SYS_WAIT:
    esp += 4;
    pid_t pid = &esp;
    wait (pid);
    break;
  case SYS_CREATE:
    esp += 4;
    file = esp;
    esp += 4;
    size = &esp;
    create (file, size);
    break;
  case SYS_REMOVE:
    esp += 4;
    file = esp;
    remove (file);
    break;
  case SYS_OPEN:
    esp += 4;
    file = esp;
    open (file);
    break;
  case SYS_FILESIZE:
    esp += 4;
    fd = &esp;
    filesize (fd);
    break;
  case SYS_READ:
    esp += 4;
    fd = &esp;
    esp += 4;
    buffer = &esp;
    esp += 4;
    size = &esp;
    read (fd, buffer, size);
    break;
  case SYS_WRITE: 
    esp += 4;
    fd = &esp;
    esp += 4;
    buffer = &esp;
    esp += 4;
    size = &esp;
    write (fd, buffer, size);
    break;
  case SYS_SEEK:
    esp += 4;
    fd = &esp;
    esp += 4;
    unsigned position = &esp;
    seek (fd, position);
    break;
  case SYS_TELL:
    esp += 4;
    fd = &esp;
    tell (fd);
    break;
  case SYS_CLOSE:
    esp += 4;
    fd = &esp;
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
  printf("%s: exit(%d)\n", thread_name(), status);
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
  return filesys_create (file, size);
}

bool
remove (const char *file)
{
  return filesys_remove (file);
}

int
open (const char *file)
{
  int index = 2;
  while(index < sizeof(fds))
  {
    if(fds[index] == NULL)
    {
      fds[index] = filesys_open(file);
      return index;
    }
    index++;
  }
  return -1;
}

int
filesize (int fd)
{
  return file_length(fds[fd]);
}

int
read (int fd, void *buffer, unsigned size)
{
  if(fd == 0)
  {
    strlcpy(buffer, input_getc() , size);
  }else
  {
    return file_read(fds[fd], buffer, size);
  }
}

int
write (int fd, const void *buffer, unsigned size)
{
  if(fd == 1)
  {
    putbuf(buffer, size);
    return size;
  }else
  {
    return file_write(fds[fd], buffer, size);
  }
}

void
seek (int fd, unsigned position)
{
  file_seek(fds[fd], position);
}

unsigned 
tell (int fd)
{
  return file_tell(fds[fd]);
}

void
close (int fd)
{
  file_close(fds[fd]);
  fds[fd] = NULL;
}

