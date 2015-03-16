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
  int **esp = f->esp;
  int **call = esp;
  if(esp == NULL || is_kernel_vaddr(esp) ||
     !pagedir_is_mapped(thread_current()->pagedir, esp))
  {
    exit (-1);
  }
  const char *file;
  int fd;
  int status;
  unsigned size;
  unsigned position;
  const char *cmd;
  void *buffer;
  pid_t pid;
  bool moreargs = true;
  if (!is_kernel_vaddr(esp += 1)) moreargs = false;
  switch ((int) *call)
  {
  case SYS_HALT:
    printf ("HALT CALLED\n");
    halt ();
    break;
  case SYS_EXIT:
    printf ("EXIT CALLED\n");
    if (moreargs) status = (int) *esp;
    else status = 0;
    exit (status);
    break;
  case SYS_EXEC:
    printf ("EXEC CALLED\n");
    if (moreargs) cmd = (const char *) *esp;
    else 
    {
      printf ("No name to execute executable file.");
      exit (-1);
    }
    exec (cmd);
    break;
  case SYS_WAIT:
    printf ("WAIT CALLED\n");
    pid = (pid_t) *esp;
    wait (pid);
    break;
  case SYS_CREATE:
    printf ("CREATE CALLED\n");
    file = (const char *) *esp;
    esp += 1;
    size = (unsigned) *esp;
    create (file, size);
    break;
  case SYS_REMOVE:
    printf ("REMOVE CALLED\n");
    file = (const char *) *esp;
    remove (file);
    break;
  case SYS_OPEN:
    printf ("OPEN CALLED\n");
    file = (const char *) *esp;
    open (file);
    break;
  case SYS_FILESIZE:
    printf ("FILESIZE CALLED\n");
    fd = (int) *esp;
    filesize (fd);
    break;
  case SYS_READ:
    printf ("READ CALLED\n");
    fd = (int) *esp;
    esp += 1;
    buffer = (void *) *esp;
    esp += 1;
    size = (unsigned) *esp;
    read (fd, buffer, size);
    break;
  case SYS_WRITE: 
    printf ("WRITE CALLED\n");
    if (moreargs) fd = (int) *esp;
    else
    {
      printf ("Writing to console!\n");
      fd = 1;
      write (fd, NULL, NULL);
      break;
    }
    esp += 1;
    buffer = (void *) *esp;
    esp += 1;
    size = (int) *esp;
    write (fd, buffer, size);
    break;
  case SYS_SEEK:
    printf ("SEEK CALLED\n");
    fd = (int) *esp;
    esp += 1;
    position = (unsigned) *esp;
    seek (fd, position);
    break;
  case SYS_TELL:
    printf ("TELL CALLED\n");
    fd = (int) *esp;
    tell (fd);
    break;
  case SYS_CLOSE:
    printf ("CLOSE CALLED\n");
    fd = (int) *esp;
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
  //return process_execute (strtok_r (cmd_line, " ", save_ptr));
  return process_execute (cmd_line);
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

