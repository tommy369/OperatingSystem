#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
  /* Pointer argument and dummy return address, and userland dummy return
   * address */
  uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
  *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
  memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
  return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
	KASSERT(regs != NULL);
	dbg(DBG_PRINT,"(GRADING3A 7.a) Register values are not NULL\n");
	KASSERT(curproc != NULL);
	dbg(DBG_PRINT,"(GRADING3A 7.a) Current Process is not NULL\n");
	KASSERT(curproc->p_state == PROC_RUNNING);
	dbg(DBG_PRINT,"(GRADING3A 7.a) The state of current process is PROC_RUNNING\n");
	proc_t* p1 = proc_create("fork child");

	KASSERT(p1->p_state == PROC_RUNNING);
  	dbg(DBG_PRINT,"(GRADING3A 7.a) The state of new process is PROC_RUNNING\n");
  	KASSERT(p1->p_pagedir != NULL);
  	dbg(DBG_PRINT,"(GRADING3A 7.a) Page directory of the new process is not NULL\n");
  	p1->p_vmmap = vmmap_clone(curproc->p_vmmap);

  vmarea_t* child;
  list_link_t *par_link  = p1->p_pproc->p_vmmap->vmm_list.l_next;
  list_iterate_begin(&p1->p_vmmap->vmm_list, child, vmarea_t, vma_plink)
    {
      vmarea_t* parent = list_item(par_link, vmarea_t,vma_plink);
      if(parent->vma_flags & MAP_SHARED)
      {
    	  /*Increasing refcount of underlying objects*/
		  child->vma_obj = parent->vma_obj;
		  child->vma_obj->mmo_ops->ref(child->vma_obj);

		  /*inserting into list of parent so that later when parent destroyed corresponding child can be flushed out*/
		  list_insert_tail(&(parent->vma_obj->mmo_un.mmo_vmas),&child->vma_olink);
      }
      else
      {
    	  /* Creating shadow object for parent old proces i.e parent process and copying entirely in the shadow copy*/
		  mmobj_t* ps = shadow_create();
		  ps->mmo_shadowed = parent->vma_obj;

		  mmobj_t* cs = shadow_create();
		  cs->mmo_shadowed = parent->vma_obj;

		  ps->mmo_un.mmo_bottom_obj = parent->vma_obj->mmo_un.mmo_bottom_obj;
		  cs->mmo_un.mmo_bottom_obj = parent->vma_obj->mmo_un.mmo_bottom_obj;

		  /*inserting into list of shadow object that corresponds to parent so that later when parent destroyed corresponding child can be flushed out*/
		  list_insert_tail(&cs->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas, &child->vma_olink);

		  parent->vma_obj->mmo_ops->ref(parent->vma_obj);

		  child->vma_obj = cs;
		  parent->vma_obj = ps;

		  pt_unmap_range(p1->p_pproc->p_pagedir, (uintptr_t)PN_TO_ADDR(parent->vma_start), (uintptr_t)PN_TO_ADDR(parent->vma_end));

      }
      par_link = par_link->l_next;
			
    }list_iterate_end();
	 tlb_flush_all();
	 kthread_t* par;
	 list_iterate_begin( &p1->p_pproc->p_threads, par, kthread_t, kt_plink)
	 {
		  kthread_t* child_thr = kthread_clone(par);
		  KASSERT(child_thr->kt_kstack != NULL);
		  dbg(DBG_PRINT,"(GRADING3A 7.a) the stack of newthr is not NULL\n");

		  child_thr->kt_proc = p1;
		  list_insert_tail(&p1->p_threads, &child_thr->kt_plink);
		  child_thr->kt_ctx.c_pdptr = p1->p_pagedir;
		  child_thr->kt_ctx.c_eip = (uint32_t)userland_entry;
		  regs->r_eax = 0;
		  child_thr->kt_ctx.c_esp = fork_setup_stack(regs, child_thr->kt_kstack);
		if(par == curthr)
		{
		  sched_make_runnable(child_thr);
		}
	}list_iterate_end();
		
	  int i;
	  for(i = 0; i < NFILES; i++)
	  {
		  p1->p_files[i]=p1->p_pproc->p_files[i];
		  if(p1->p_pproc->p_files[i]) fref(p1->p_pproc->p_files[i]);
	  }
	  p1->p_cwd=p1->p_pproc->p_cwd;
	  p1->p_brk=p1->p_pproc->p_brk;
	  p1->p_start_brk=p1->p_pproc->p_start_brk;

	  return p1->p_pid;

}
