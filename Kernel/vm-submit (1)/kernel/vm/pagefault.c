#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"
#include "api/access.h"

/*
 * yooooooooooo This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page (don't forget
 * about shadow objects, especially copy-on-write magic!). Make
 * sure that if the user writes to the page it will be handled
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{
        /*NOT_YET_IMPLEMENTED("VM: handle_pagefault");*/
	uint32_t vfn = ADDR_TO_PN(vaddr);
	vmarea_t *temp = vmmap_lookup(curproc->p_vmmap, vfn);
	if(!temp)
	{
		proc_kill(curproc, EFAULT);
		return;
	}
	uint32_t p = 0;
	if((cause & FAULT_RESERVED) == FAULT_RESERVED) p = p | PROT_NONE;
	if((cause & FAULT_WRITE) == FAULT_WRITE) p = p | PROT_WRITE;
	if((cause & FAULT_EXEC) == FAULT_EXEC) p = p | PROT_EXEC;
	if((cause & FAULT_PRESENT) == FAULT_PRESENT) p = p | PROT_READ;

	int ret = addr_perm(curproc, (const void *)vaddr, p);
	if(ret == 0)
	{
		proc_kill(curproc, EFAULT);
		return;
	}
	pframe_t *pf = NULL;
	int pf_ret;
	if(temp->vma_obj->mmo_shadowed)
	{
		int forwrite;
		if((cause & FAULT_WRITE) == FAULT_WRITE) forwrite = 1;
		else forwrite = 0;
		pf_ret = temp->vma_obj->mmo_ops->lookuppage(temp->vma_obj, vfn - temp->vma_start + temp->vma_off, forwrite, &pf);
		/*pf_ret = pframe_get(temp->vma_obj->mmo_shadowed, vfn - temp->vma_start + temp->vma_off, &pf);*/

	}
	else
	{
		pf_ret = pframe_get(temp->vma_obj, vfn - temp->vma_start + temp->vma_off, &pf);

	}
	if(pf_ret < 0)
	{
		curproc->p_status = pf_ret;
		return;
	}
	uint32_t pdflags = PD_PRESENT | PD_USER | PD_WRITE;
	uint32_t ptflags = PT_PRESENT | PT_USER;
	if((cause & FAULT_WRITE) == FAULT_WRITE) ptflags |= PT_WRITE;

	pt_map(curproc->p_pagedir,(uintptr_t)PAGE_ALIGN_DOWN(vaddr),(uintptr_t)PAGE_ALIGN_DOWN(pt_virt_to_phys((uintptr_t)pf->pf_addr)), pdflags, ptflags);
}
