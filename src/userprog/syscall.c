#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void* address_validate(const void*);
struct file_process* list_search(struct list* files, int fd);

struct file_process {
	struct file* ptr;
	int fd;
	struct list_elem elem;
};

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


void syscall_halt();
void syscall_exit();
int syscall_exec(int *p);
int syscall_wait(int *p);
int syscall_open_helper(struct file* fptr);
int syscall_read_helper(int *p);
int syscall_write_helper(int *p);

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp;

  address_validate(p);

  int system_call = * p;
	switch (system_call)
	{
		case SYS_HALT:
		syscall_halt();
		break;

		case SYS_EXIT:
		syscall_exit(p);
		break;

		case SYS_EXEC:
		f->eax=syscall_exec(p);
		break;

		case SYS_WAIT:
		f->eax = syscall_wait(p);
		break;

		case SYS_CREATE:
		address_validate(p+2);
		address_validate(*(p+1));
		acquire_filesys_lock();
		f->eax = filesys_create(*(p+1),*(p+2));
		release_filesys_lock();
		break;

		case SYS_REMOVE:
		address_validate(p+1);
		address_validate(*(p+1));
		acquire_filesys_lock();
		if(filesys_remove(*(p+1))!=NULL)
			f->eax = true;
		else
			f->eax = false;
		release_filesys_lock();
		break;

		case SYS_OPEN:
		address_validate(p+1);
		address_validate(*(p+1));

		acquire_filesys_lock();
		struct file* fptr = filesys_open (*(p+1));
		release_filesys_lock();
		f->eax = syscall_open_helper(fptr);
		break;

		case SYS_FILESIZE:
		address_validate(p+1);
		acquire_filesys_lock();
		f->eax = file_length (list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
		break;

		case SYS_READ:

		address_validate(p+3);
		address_validate(*(p+2));
		address_validate(p+1);
		if(*(p+1)!=0)
		{
			struct file_process* fptr = list_search(&thread_current()->files, *(p+1));
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				acquire_filesys_lock();
				f->eax = file_read (fptr->ptr, *(p+2), *(p+3));
				release_filesys_lock();
			}
		}
		else
		{
			f->eax = syscall_read_helper(p);
		}		
		break;

		case SYS_WRITE:
		address_validate(p+3);
		address_validate(*(p+2));
		address_validate(p+1);
		if(*(p+1)!=1)
		{
			struct file_process* fptr = list_search(&thread_current()->files, *(p+1));
			if(fptr==NULL)
				f->eax=-1;
			else
			{
				acquire_filesys_lock();
				f->eax = file_write (fptr->ptr, *(p+2), *(p+3));
				release_filesys_lock();
			}	
		}
		else
		{
			f->eax = syscall_write_helper(p);
		}
		break;

		case SYS_SEEK:
		address_validate(p+3);
		address_validate(p+2);
		address_validate(p+1);
		acquire_filesys_lock();
		file_seek(list_search(&thread_current()->files, *(p+1))->ptr,*(p+2));
		release_filesys_lock();
		break;

		case SYS_TELL:
		address_validate(p+1);
		acquire_filesys_lock();
		f->eax = file_tell(list_search(&thread_current()->files, *(p+1))->ptr);
		release_filesys_lock();
		break;

		case SYS_CLOSE:
		address_validate(p+1);
		acquire_filesys_lock();
		close_file(*(p+1));
		f->eax = -1;
		release_filesys_lock();
		break;


		default:
		printf("Default %d\n",*p);
	}
}


void syscall_halt(){
	shutdown_power_off();
}


void syscall_exit(int *p){
	address_validate(p+1);
	exit_process(*(p+1));
}

int syscall_exec(int *p){
	address_validate(p+1);
	address_validate(*(p+1));
	return exec_process(*(p+1));
}

int syscall_wait(int *p){
	address_validate(p+1);
	return process_wait(*(p+1));
}

int syscall_open_helper(struct file* fptr){
	if(fptr!=NULL)
		{
			struct file_process *pfile = malloc(sizeof(*pfile));
			pfile->ptr = fptr;
			pfile->fd = thread_current()->fd_count;
			thread_current()->fd_count++;
			list_push_back (&thread_current()->files, &pfile->elem);
			return pfile->fd;
		}
			
	return -1;
}

int syscall_read_helper(int *p){
	int i;
	uint8_t* buffer = *(p+2);
	for(i=0;i<*(p+3);i++)
		buffer[i] = input_getc();
	return *(p+3);
}


int syscall_write_helper(int *p){
	putbuf(*(p+2),*(p+3));
	return *(p+3);
}


void* address_validate(const void *vaddr)
{
	if (!is_user_vaddr(vaddr))
	{
		exit_process(-1);
		return 0;
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		exit_process(-1);
		return 0;
	}
	return ptr;
}


struct file_process* list_search(struct list* files, int fd)
{

	struct list_elem *e;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct file_process *f = list_entry (e, struct file_process, elem);
          if(f->fd == fd)
          	return f;
        }
   return NULL;
}

void close_file(int fd)
{

	struct thread *t = thread_current();
    struct list_elem *next;
    struct list_elem *e = list_begin(&t->files);
  
    for (;e != list_end(&t->files); e = next)
    {
    	next = list_next(e);
    	struct file_process *process_file_ptr = list_entry (e, struct file_process, elem);
    	if (fd == process_file_ptr->fd || fd == -1)
    	{
			file_close(process_file_ptr->ptr);
			list_remove(&process_file_ptr->elem);
			free(process_file_ptr);
			if (fd != -1)
			{
				return;
			}
    }
  }
}

void close_all_files(struct list* files)
{

	struct list_elem *e;

	while(!list_empty(files))
	{
		e = list_pop_front(files);

		struct file_process *f = list_entry (e, struct file_process, elem);
          
	      	file_close(f->ptr);
	      	list_remove(e);
	      	free(f);
	}
}


int exec_process(char *file_name)
{
	acquire_filesys_lock();
	char * fn_cp = malloc (strlen(file_name)+1);
	strlcpy(fn_cp, file_name, strlen(file_name)+1);
	
	char * save_ptr;
	fn_cp = strtok_r(fn_cp," ",&save_ptr);

	struct file* f = filesys_open (fn_cp);

	if(f==NULL)
	{
		release_filesys_lock();
		return -1;
	}
	else
	{
		file_close(f);
		release_filesys_lock();
		return process_execute(file_name);
	}
}

void exit_process(int status)
{
	struct list_elem *e;

      for (e = list_begin (&thread_current()->parent->process_child); e != list_end (&thread_current()->parent->process_child);
           e = list_next (e))
        {
          struct child *f = list_entry (e, struct child, elem);
          if(f->tid == thread_current()->tid)
          {
          	f->used = true;
          	f->exit_error = status;
          }
        }


	thread_current()->exit_error = status;

	if(thread_current()->parent->wait_on_tid == thread_current()->tid)
		sema_up(&thread_current()->parent->process_child_lock);

	thread_exit();
}
