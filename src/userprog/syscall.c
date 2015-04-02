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
#include "threads/synch.h"

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
mapid_t mmap (int, void *);
void munmap (mapid_t);
struct lock io_lock;

void
syscall_init (void) 
{
  lock_init (&io_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
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
  mapid_t mapping;
  if (!is_kernel_vaddr(esp += 1)) moreargs = false;
  switch ((int) *call)
  {
  case SYS_HALT:
    halt ();
    break;
  case SYS_EXIT:
    if (moreargs)
    {
       status = (int) *esp;
       exit(status);
    }
    else exit(-1);
    break;
  case SYS_EXEC:
    if (moreargs) cmd = (const char *) *esp;
    else 
    {
      printf ("No name to execute executable file.");
      exit (-1);
    }
    f->eax = exec (cmd);
    break;
  case SYS_WAIT:
    pid = (pid_t) *esp;
    f->eax = wait (pid);
    break;
  case SYS_CREATE:
    file = (const char *) *esp;
    esp += 1;
    size = (unsigned) *esp;
    f->eax = create (file, size);
    break;
  case SYS_REMOVE:
    file = (const char *) *esp;
    f->eax = remove (file);
    break;
  case SYS_OPEN:
    file = (const char *) *esp;
    f->eax = open (file);
    break;
  case SYS_FILESIZE:
    fd = (int) *esp;
    f->eax = filesize (fd);
    break;
  case SYS_READ:
    fd = (int) *esp;
    esp += 1;
    buffer = (void *) *esp;
    esp += 1;
    size = (unsigned) *esp;
    f->eax = read (fd, buffer, size);
    break;
  case SYS_WRITE: 
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
    fd = (int) *esp;
    esp += 1;
    position = (unsigned) *esp;
    seek (fd, position);
    break;
  case SYS_TELL:
    fd = (int) *esp;
    f->eax = tell (fd);
    break;
  case SYS_CLOSE:
    fd = (int) *esp;
    close (fd);
    break;
  case SYS_MMAP:
    fd = (int) *esp;
    esp += 1;
    void *addr = (void *) *esp;
    f->eax = mmap (fd, addr);
    break;
  case SYS_MUNMAP:
    mapping = (mapid_t) *esp;
    munmap (mapping);
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
  if(temp == NULL || temp == -1) return -1;
  lock_acquire (&io_lock);
  pid_t pid = process_execute (cmd_line);
  lock_release (&io_lock);
  return pid;
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
  lock_acquire (&io_lock);
  bool ret = filesys_create (file, size);
  lock_release (&io_lock);
  return ret;
}

bool
remove (const char *file)
{
  lock_acquire (&io_lock);
  bool ret = filesys_remove (file);
  lock_release (&io_lock);
  return ret;
}

int
open (const char *file)
{
  if(file == NULL)
    return -1;
  if(file >= PHYS_BASE || !pagedir_is_mapped(thread_current()->pagedir, file))
    exit(-1);
  if(file[0] == '\0') return -1;
  int index = 2;
  while(index < sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0]))
  {
    if(thread_current()->fds[index] == NULL)
    {
      lock_acquire (&io_lock);
      thread_current()->fds[index] = filesys_open(file);
      lock_release (&io_lock);
      if (thread_current()->fds[index] == NULL) return -1;
      return index;
    }
    index++;
  }
  return -1;
}

int
filesize (int fd)
{
  if(fd == NULL || fd < 2 ||
     fd > sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0])) exit(-1);
  return file_length(thread_current()->fds[fd]);
}

int
read (int fd, void *buffer, unsigned size)
{
  if(fd == NULL || fd < 0 || fd == 1 ||
     fd > sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0]))
    exit(-1);
  if(buffer == NULL || buffer >= PHYS_BASE
     || !pagedir_is_mapped(thread_current()->pagedir, buffer)) exit(-1);
  if(fd == 0)
  {
    strlcpy(buffer, input_getc() , size);
  }else
  {
    lock_acquire (&io_lock);
    int ret = file_read(thread_current()->fds[fd], buffer, size);
    lock_release (&io_lock);
    return ret;
  }
}

int
write (int fd, const void *buffer, unsigned size)
{
  if(fd == NULL || fd < 1 ||
     fd > sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0]))
    exit(-1);
  if(buffer == NULL || buffer >= PHYS_BASE
     || !pagedir_is_mapped(thread_current()->pagedir, buffer)) exit(-1);
  if(fd == 1)
  {
    putbuf(buffer, size);
    return size;
  }else if(fd < 2 || fd > sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0]))
  {
    exit(-1);
  }else
  {
    lock_acquire (&io_lock);
    int ret = file_write(thread_current()->fds[fd], buffer, size);
    lock_release (&io_lock);
    return ret;
  }
}

void
seek (int fd, unsigned position)
{
  if(fd == NULL || fd < 2 ||
     fd > sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0]))
    exit(-1);
  lock_acquire (&io_lock);
  file_seek(thread_current()->fds[fd], position);
  lock_release (&io_lock);
}

unsigned 
tell (int fd)
{
  if(fd == NULL || fd < 2 ||
     fd > sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0]))
    exit(-1);
  lock_acquire (&io_lock);
  unsigned ret = file_tell(thread_current()->fds[fd]);
  lock_release (&io_lock);
  return ret;
}

void
close (int fd)
{
  if (fd == NULL || fd < 2 ||
      fd > sizeof(thread_current()->fds)/sizeof(thread_current()->fds[0]))
    exit(-1);
  lock_acquire (&io_lock);
  file_close(thread_current()->fds[fd]);
  thread_current()->fds[fd] = NULL;
  lock_release (&io_lock);
}

mapid_t
mmap (int fd, void *addr)
{
  return NULL;
}

void
munmap (mapid_t mapping)
{
  
}
