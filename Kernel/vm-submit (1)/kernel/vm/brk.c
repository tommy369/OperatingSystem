#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should NOT fail as the man page says. Instead,
 * "return" the current break. We use this to implement sbrk(0) without writing
 * a separate syscall. Look in user/libc/syscall.c if you're curious.
 *
 * Also, despite the statement on the manpage, you MUST support combined use
 * of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{



		if(!addr)
		{
			 *ret=curproc->p_brk;
			 return 0;
	    }

		if((uint32_t)addr < (uint32_t)curproc->p_start_brk || (uint32_t)addr > USER_MEM_HIGH)
		{
					return -ENOMEM;
		}

	  uint32_t brkstrt = ADDR_TO_PN(curproc->p_start_brk);

	  uint32_t end = ADDR_TO_PN(addr);

	  	if(!(vmmap_lookup(curproc->p_vmmap,brkstrt)))
	    {
			  vmarea_t *newvma;
			  size_t len =  (size_t)PN_TO_ADDR(end - brkstrt);
			  int prot = PROT_READ | PROT_WRITE;
			  int flags = MAP_PRIVATE | MAP_ANON;
			  int fd = -1;
			  int rv = do_mmap(PN_TO_ADDR(brkstrt), len, prot, flags, fd,0, (void**)&newvma);
			  if(rv < 0)
			  {
				  return rv;
			  }

			  goto label;
	    }

		 uint32_t start = ADDR_TO_PN(curproc->p_brk);

		  if(end == start)
			goto label;

		  if(end > start)
		  {

			  vmarea_t* vma = vmmap_lookup(curproc->p_vmmap, start);
			  if(vmmap_is_range_empty(curproc->p_vmmap, vma->vma_end, end + 1 - vma->vma_end))
			  {
				  vma->vma_end = end + 1;
				  goto label;
			  }
			  else return -ENOMEM;
		  }
		  else
		  {
			  vmmap_remove(curproc->p_vmmap,end + 1,start - end);
			  goto label;
		  }
		  label:
		  curproc->p_brk=addr;
		  *ret = addr;
		  return 0;

}
