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
#include "threads/palloc.h"

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
  //printf ("system call!\n");
  int **sp = f->esp;
  int **call = sp;
  if(sp == NULL || is_kernel_vaddr(sp) ||
     !pagedir_is_mapped(thread_current()->pagedir, sp))
  {
    exit (-1);
  }
  int **esp = pagedir_get_page (thread_current()->pagedir, sp);
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
    //printf ("HALT CALLED\n");
    halt ();
    break;
  case SYS_EXIT:
    //printf ("EXIT CALLED\n");
    if (moreargs)
    {
       status = (int) *esp;
       exit(status);
    }
    else exit(-1);
    //exit (status);
    break;
  case SYS_EXEC:
    //printf ("EXEC CALLED\n");
    if (moreargs) cmd = (const char *) *esp;
    else 
    {
      printf ("No name to execute executable file.");
      exit (-1);
    }
    f->eax = exec (cmd);
    break;
  case SYS_WAIT:
    //printf ("WAIT CALLED\n");
    pid = (pid_t) *esp;
    f->eax = wait (pid);
    break;
  case SYS_CREATE:
    //printf ("CREATE CALLED\n");
    file = (const char *) *esp;
    esp += 1;
    size = (unsigned) *esp;
    f->eax = create (file, size);
    break;
  case SYS_REMOVE:
    //printf ("REMOVE CALLED\n");
    file = (const char *) *esp;
    f->eax = remove (file);
    break;
  case SYS_OPEN:
    //printf ("OPEN CALLED\n");
    file = (const char *) *esp;
    f->eax = open (file);
    break;
  case SYS_FILESIZE:
    //printf ("FILESIZE CALLED\n");
    fd = (int) *esp;
    f->eax = filesize (fd);
    break;
  case SYS_READ:
    //printf ("READ CALLED\n");
    fd = (int) *esp;
    esp += 1;
    buffer = (void *) *esp;
    esp += 1;
    size = (unsigned) *esp;
    f->eax = read (fd, buffer, size);
    break;
  case SYS_WRITE: 
    //printf ("WRITE CALLED\n");
    if (moreargs) fd = (int) *esp;
    else
    {
      printf ("Writing to console!\n");
      fd = 1;
      f->eax = write (fd, NULL, NULL);
      break;
    }
    esp += 1;
    buffer = (void *) *esp;
    esp += 1;
    size = (int) *esp;
    f->eax = write (fd, buffer, size);
    break;
  case SYS_SEEK:
    //printf ("SEEK CALLED\n");
    fd = (int) *esp;
    esp += 1;
    position = (unsigned) *esp;
    seek (fd, position);
    break;
  case SYS_TELL:
    //printf ("TELL CALLED\n");
    fd = (int) *esp;
    f->eax = tell (fd);
    break;
  case SYS_CLOSE:
    //printf ("CLOSE CALLED\n");
    fd = (int) *esp;
    close (fd);
    break;
  }
  //printf("END OF SYSTEM CALL\n");
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
  thread_current ()->exit_status = status;
  thread_exit ();
}

pid_t
exec (const char *cmd_line)
{
  if(cmd_line == NULL || cmd_line >= PHYS_BASE
     || !pagedir_is_mapped(thread_current ()->pagedir, cmd_line)
     || cmd_line[0] == '\0') exit(-1);
  char *save_ptr;
  char * cmd_copy = palloc_get_page (0);
  if (cmd_copy == NULL) return TID_ERROR;
  strlcpy (cmd_copy, cmd_line, PGSIZE);
  int temp = filesys_open(strtok_r (cmd_copy, " ", &save_ptr));
  //int temp = open(cmd_line);
  if(temp == NULL || temp == -1) return -1;
//  close(temp);
  return process_execute (cmd_line);
}

int
wait (pid_t pid)
{
  if(pid == NULL || pid >= PHYS_BASE
     || pagedir_is_mapped(thread_current()->pagedir, pid)) exit(-1);
  return process_wait (pid);
}

bool
create (const char *file, unsigned size)
{
  if(file == NULL || file >= PHYS_BASE
     || !pagedir_is_mapped(thread_current()->pagedir, file))
    exit(-1);
  if(file[0] == '\0')
    return 0;

  if(strlen(file)>14) return 0;
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
  if(file == NULL)
    return -1;
  if(file >= PHYS_BASE || !pagedir_is_mapped(thread_current()->pagedir, file))
    exit(-1);
  if(file[0] == '\0')
    return -1;

  //printf ("fds size: %d\n", sizeof(fds)/sizeof(fds[0]));
  int index = 2;
  while(index < sizeof(fds)/sizeof(fds[0]))
  {
    if(fds[index] == NULL)
    {
      fds[index] = filesys_open(file);
      if (fds[index] == NULL) return -1;
      return index;
    }
    index++;
  }
  return -1;
}

int
filesize (int fd)
{
  if(fd == NULL || fd < 2 || fd > sizeof(fds)/sizeof(fds[0])) exit(-1);
  return file_length(fds[fd]);
}

int
read (int fd, void *buffer, unsigned size)
{
  if(fd == NULL || fd < 0 || fd == 1 || fd > sizeof(fds)/sizeof(fds[0])) exit(-1);
  if(buffer == NULL || buffer >= PHYS_BASE
     || !pagedir_is_mapped(thread_current()->pagedir, buffer)) exit(-1);
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
  if(fd == NULL || fd < 1 || fd > sizeof(fds)/sizeof(fds[0])) exit(-1);
  if(buffer == NULL || buffer >= PHYS_BASE
     || !pagedir_is_mapped(thread_current()->pagedir, buffer)) exit(-1);
  if(fd == 1)
  {
    putbuf(buffer, size);
    return size;
  }else if(fd < 2 || fd > sizeof(fds)/sizeof(fds[0]))
  {
    exit(-1);
  }else
  {
    return file_write(fds[fd], buffer, size);
  }
}

void
seek (int fd, unsigned position)
{
  if(fd == NULL || fd < 2 || fd > sizeof(fds)/sizeof(fds[0])) exit(-1);
  file_seek(fds[fd], position);
}

unsigned 
tell (int fd)
{
  if(fd == NULL || fd < 2 || fd > sizeof(fds)/sizeof(fds[0])) exit(-1);
  return file_tell(fds[fd]);
}

void
close (int fd)
{
  if(fd == NULL || fd < 2 || fd > sizeof(fds)/sizeof(fds[0])) exit(-1);
  file_close(fds[fd]);
  fds[fd] = NULL;
}

