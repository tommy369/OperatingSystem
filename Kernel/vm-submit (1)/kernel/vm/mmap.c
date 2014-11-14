#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/stat.h"


#include "vm/vmmap.h"
#include "vm/mmap.h"


/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int do_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off,
		void **ret)
{

	/*if ((uint32_t)addr < USER_MEM_LOW || (uint32_t)addr > USER_MEM_HIGH)
			return -EINVAL;*/

		uint32_t npages = ADDR_TO_PN(len);

		if (!(PAGE_ALIGNED(len)))
		{
			len = (uint32_t)PN_TO_ADDR(npages+1);
		}

		uint32_t endaddr = (uint32_t) addr + len;
		if (flags & MAP_FIXED)
		{
			if(!PAGE_ALIGNED(addr))
	    	{
				return -EINVAL;
	    	}
			if ((uint32_t)addr < USER_MEM_LOW || (uint32_t)addr > USER_MEM_HIGH || endaddr > USER_MEM_HIGH)
			{
				return -EINVAL;
			}
		}
		else
		{
			int findrange_ret = 0;
			if (!ADDR_TO_PN(len))
				return -EINVAL;

			findrange_ret = vmmap_find_range(curproc->p_vmmap, ADDR_TO_PN(len),VMMAP_DIR_LOHI);
			if (findrange_ret < 0)
				return -EINVAL;
			addr = PN_TO_ADDR(findrange_ret);
		}

		if (flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_FIXED | MAP_ANON))
				return -ENOTSUP;

		if (len <= 0)
			return -EINVAL;

		if (!(flags & MAP_SHARED) && !(flags & MAP_PRIVATE))
			return -EINVAL;

		if ((flags & MAP_PRIVATE) && (flags & MAP_SHARED))
			return -EINVAL;

		if (!PAGE_ALIGNED(off))
			return -EINVAL;

		if(fd < 0 && fd > MAX_FILES)
		{
			if (!(flags & MAP_ANON))
				return -EBADF;
		}

		/*Obtaining the vnode_t w.r.t the given file descriptor */
		file_t *sfte;
		if (flags & MAP_ANON)
			sfte = NULL;
		else
			sfte = fget(fd);

		if(sfte)
		{
			if(!sfte->f_vnode->vn_ops->mmap)
			{
				fput(sfte);
				return -ENODEV;
			}

			if ((prot & PROT_WRITE) && (sfte->f_mode & FMODE_APPEND))
				goto label;

			if((flags & MAP_PRIVATE) && !(sfte->f_mode & FMODE_READ))
			{
				goto label;
			}

			if ((flags & MAP_SHARED) && (prot & PROT_WRITE) && (!(sfte->f_mode & FMODE_READ) || !(sfte->f_mode & FMODE_WRITE)))
				goto label;

			if (!(S_ISREG(sfte->f_vnode->vn_mode) || (S_ISCHR(sfte->f_vnode->vn_mode))))
			{
				goto label;
			}
		}
		if (sfte == NULL && !(flags & MAP_ANON))
		{
			fput(sfte);
			return -EBADF;
		}

		/*getting the "lopage" value - the page number from where the mapping is going to start */
		uint32_t lopage = ADDR_TO_PN(addr);

		/*size_t len is length in bytes. Converting it to number of pages. Not sure if length is to be manipulated this way. */
		/*uint32_t npages = len / PAGE_SIZE;           (Not sure....... DOUBLE CHECK !! )*/

		/*vmarea_t **new -> going to point to the newly mapped vmarea */
		vmarea_t *new;



		int vmmap_ret = 0;
		/*
		 * 	int out = vmmap_map(curproc->p_vmmap, vn, ADDR_TO_PN(addr), ADDR_TO_PN(len),
				prot, flags, off, VMMAP_DIR_LOHI, &vma);
		 */

		if(flags & MAP_ANON)
		{
			vmmap_ret = vmmap_map(curproc->p_vmmap, NULL, lopage, ADDR_TO_PN(len),
					prot, flags, off, VMMAP_DIR_LOHI, &new);
		}
		else
		{
			vmmap_ret = vmmap_map(curproc->p_vmmap, sfte->f_vnode, lopage, ADDR_TO_PN(len),
					prot, flags, off, VMMAP_DIR_LOHI, &new);
		}

		*ret = PN_TO_ADDR(new->vma_start);

		if (!(flags & MAP_ANON))
			fput(sfte);

		tlb_flush_range((uintptr_t)addr, len);
		/*flushing the TLB */

		KASSERT(curproc->p_pagedir != NULL);
		dbg(DBG_PRINT, "GRADING3A 2.a - Page Directory of current process still exists");
		return vmmap_ret;
		/*NOT_YET_IMPLEMENTED("VM: do_mmap");
		 return -1;*/
		label:
		     fput(sfte);
		     return -EACCES;
}

/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int do_munmap(void *addr, size_t len)
{
	/* checking if addr is a multiple of page size */
		uint32_t lopage = ADDR_TO_PN(addr);
		uint32_t npages = ADDR_TO_PN(len);

		uint32_t endaddr = (uint32_t)addr + (uintptr_t)len;

		if(!PAGE_ALIGNED(addr))
		{
			return -EINVAL;
		}

		if (!(PAGE_ALIGNED(len)))
		{
			len = (uint32_t) PN_TO_ADDR(npages + 1);
		}

		if ((uintptr_t)addr < USER_MEM_LOW || (uintptr_t)addr > USER_MEM_HIGH || endaddr > USER_MEM_HIGH)
			return -EINVAL;
		/*Not sure if this needs to be done */
		if (len <= 0)
			return -EINVAL;
		/* not sure */
		/*getting the "lopage" value - the page number from where the unmapping is going to start */

		int vmmap_ret = vmmap_remove(curproc->p_vmmap, lopage, npages);
		/*tlb_flush_range((uintptr_t)addr, len);*/
		tlb_flush_all();

		KASSERT(curproc->p_pagedir != NULL);
		dbg(DBG_PRINT, "GRADING3A 2.b - Page directory of current process still exists");
		return 0;}
