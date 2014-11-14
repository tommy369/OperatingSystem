#include "config.h"
#include "globals.h"

#include "errno.h"

#include "util/init.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"

#include "mm/slab.h"
#include "mm/page.h"

kthread_t *curthr; /* global */
static slab_allocator_t *kthread_allocator = NULL;

#ifdef __MTP__
/* Stuff for the reaper daemon, which cleans up dead detached threads */
static proc_t *reapd = NULL;
static kthread_t *reapd_thr = NULL;
static ktqueue_t reapd_waitq;
static list_t kthread_reapd_deadlist; /* Threads to be cleaned */

static void *kthread_reapd_run(int arg1, void *arg2);
#endif

void
kthread_init()
{
		dbg(DBG_MM, "kthread_init start.....\n");
		kthread_allocator = slab_allocator_create("kthread", sizeof(kthread_t));
        KASSERT(NULL != kthread_allocator);
        dbg(DBG_MM, "kthread_init ended...\n");
}

/**
 * Allocates a new kernel stack.
 *
 * @return a newly allocated stack, or NULL if there is not enough
 * memory available
 */
static char *
alloc_stack(void)
{
        /* extra page for "magic" data */
		dbg(DBG_MM, "alloc_stack start.....\n");
		char *kstack;
        int npages = 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT);
        kstack = (char *)page_alloc_n(npages);
        dbg(DBG_MM, "alloc_stack end....\n");
        return kstack;
}

/**
 * Frees a stack allocated with alloc_stack.
 *
 * @param stack the stack to free
 */
static void
free_stack(char *stack)
{
		dbg(DBG_MM, "free_stack entered...\n");
        page_free_n(stack, 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT));
        dbg(DBG_MM, "free stack exited...\n");
}

/*
 * Allocate a new stack with the alloc_stack function. The size of the
 * stack is DEFAULT_STACK_SIZE.
 *
 * Don't forget to initialize the thread context with the
 * context_setup function. The context should have the same pagetable
 * pointer as the process.
 */
kthread_t *
kthread_create(struct proc *p, kthread_func_t func, long arg1, void *arg2)
{
		/*NOT_YET_IMPLEMENTED("PROCS: kthread_create");*/
		dbg(DBG_MM, "Creating kthread for process : %s\n",p->p_comm);
		KASSERT(NULL != p);
		dbg(DBG_MM, "GRADING1 3.a Process associated with the thread is not NULL\n");
		kthread_t *newThread = (kthread_t *)(slab_obj_alloc(kthread_allocator));

		newThread->kt_kstack = alloc_stack();

		newThread->kt_proc = p;
		newThread->kt_state = KT_RUN;

		/*Initializing the links of the thread*/
		list_link_init(&newThread->kt_plink);
		list_link_init(&newThread->kt_qlink);

        list_insert_tail(&p->p_threads, &newThread->kt_plink);

		/*context_t context_newThread;*/
		context_setup(&newThread->kt_ctx, func, arg1, arg2, (void *)newThread->kt_kstack, DEFAULT_STACK_SIZE, p->p_pagedir);

		newThread->kt_cancelled = 0;
		newThread->kt_errno = 0;
		newThread->kt_retval = 0;
		newThread->kt_wchan = NULL;
		/*newThread->kt_ctx = context_newThread;*/
		dbg(DBG_MM, "Kthread created successfully for process : %s......\n", p->p_comm);
		return newThread;
        /*return NULL;*/
}

void
kthread_destroy(kthread_t *t)
{
		dbg(DBG_MM, "kthread_destroy start....\n");
		KASSERT(t && t->kt_kstack);
        free_stack(t->kt_kstack);
        if (list_link_is_linked(&t->kt_plink))
                list_remove(&t->kt_plink);

        slab_obj_free(kthread_allocator, t);
        dbg(DBG_MM, "kthread_destroy end....\n");
}

/*
 * If the thread to be cancelled is the current thread, this is
 * equivalent to calling kthread_exit. Otherwise, the thread is
 * sleeping and we need to set the cancelled and retval fields of the
 * thread.
 *
 * If the thread's sleep is cancellable, cancelling the thread should
 * wake it up from sleep.
 *
 * If the thread's sleep is not cancellable, we do nothing else here.
 */
void
kthread_cancel(kthread_t *kthr, void *retval)
/*Not sure about unlinking the thread*/
{
	/*NOT_YET_IMPLEMENTED("PROCS: kthread_cancel");*/
	dbg(DBG_MM, "kthread_cancel start....\n");
	KASSERT(NULL != kthr);
	dbg(DBG_MM, "GRADING1 3.b The thread to be cancelled is valid and not NULL\n");
	if(kthr == curthr)
	{
		kthread_exit(kthr->kt_retval);
	}
	else
	{
		kthr->kt_retval = retval;
		/*if(kthr->kt_state == KT_SLEEP_CANCELLABLE)
		{
			sched_wakeup_on(kthr->kt_wchan);
		}*/
		sched_cancel(kthr);
	}
	dbg(DBG_MM, "kthread_cancel end....\n");
}

/*
 * You need to set the thread's retval field, set its state to
 * KT_EXITED, and alert the current process that a thread is exiting
 * via proc_thread_exited.
 *
 * It may seem unneccessary to push the work of cleaning up the thread
 * over to the process. However, if you implement MTP, a thread
 * exiting does not necessarily mean that the process needs to be
 * cleaned up.
 */
void
kthread_exit(void *retval)
{
     /*NOT_YET_IMPLEMENTED("PROCS: kthread_exit");*/
	dbg(DBG_MM, "kthread_exit start...\n");
	KASSERT(!curthr->kt_wchan);
	dbg(DBG_MM, "GRADING1 3.c The current thread is not blocked on any queue\n");
	KASSERT(!curthr->kt_qlink.l_next && !curthr->kt_qlink.l_prev);
	dbg(DBG_MM, "GRADING1 3.c The current thread has no link in ktqueue\n");
	/*dbg(DBG_MM, "Current thread's Process = %s Cur Proc = %s\n", curthr->kt_proc->p_comm, curproc->p_comm);*/
	KASSERT(curthr->kt_proc == curproc);
	dbg(DBG_MM, "GRADING1 3.c Process associated with current thread is the current process\n");

	curthr->kt_retval = retval;
	curthr->kt_state = KT_EXITED;
	/*dbg(DBG_MM, "Size of %s's Waiting Queue = %d\n", curthr->kt_proc->p_pproc->p_comm, curthr->kt_proc->p_pproc->p_wait.tq_size);*/
	proc_thread_exited(curthr->kt_retval);
	dbg(DBG_MM, "kthread_exit exit....\n");
}

/*
 * The new thread will need its own context and stack. Think carefully
 * about which fields should be copied and which fields should be
 * freshly initialized.
 *
 * You do not need to worry about this until VM.
 */
kthread_t *
kthread_clone(kthread_t *thr)
{

			dbg(DBG_MM, "kthread_clone start....\n");
			KASSERT(KT_RUN == thr->kt_state);
			dbg(DBG_PRINT,"(GRADING3A 8.a) State of thr is KT_RUN\n");

			/* New thread created */
			kthread_t* child_thread = slab_obj_alloc(kthread_allocator);
			/* Allocating thread its context
			 * Here thr  is parent thread and child_thread is the child one in which we need to copy context of parent thread
			 * context_t consists of all the pointers to be restored....esp, ebp..
			 * */
			/*memcpy(&child_thread->kt_ctx, &thr->kt_ctx,sizeof(context_t));*/
			/* ALlocating the stack*/
			child_thread->kt_kstack = alloc_stack();

			/*context_setup();*/

				/* If the stack is not empty then setting the value of the stack in the context of the thread*/
				/*child_thread->kt_ctx.c_kstack = child_thread->kt_kstack;*/
				/*Setting the stack size in the context of the child thread as that of parent thread stack size prior to copying*/
				child_thread->kt_ctx.c_kstacksz = thr->kt_ctx.c_kstacksz;
				child_thread->kt_ctx.c_kstack = (uintptr_t)child_thread->kt_kstack;
				/* After allocating the size ...and copying the structure , now actual copying using memcopy */
				/*memcpy((void *)child_thread->kt_ctx.c_kstack,(void *)thr->kt_ctx.c_kstack,child_thread->kt_ctx.c_kstacksz);*/
				/* Copying fields from the parent thread to child thread */
				child_thread->kt_cancelled = thr->kt_cancelled;
				child_thread->kt_errno = thr->kt_errno;
				child_thread->kt_retval = thr->kt_retval;
				child_thread->kt_state = thr->kt_state;
				child_thread->kt_wchan = thr->kt_wchan;

				/* Now child thread list cannot be copied from the parent thread but needs to be initialised*/
				list_link_init(&child_thread->kt_plink);
				list_link_init(&child_thread->kt_qlink);

				if(child_thread->kt_wchan)
				{
					list_insert_head(&child_thread->kt_wchan->tq_list, &child_thread->kt_qlink);
					child_thread->kt_wchan->tq_size++;
				}
			KASSERT(KT_RUN == child_thread->kt_state);
			dbg(DBG_PRINT,"(GRADING3A 8.a) State of child_thread is KT_RUN\n\n");
			dbg(DBG_MM, "kthread_clone end....\n");

	        return child_thread;
}

/*
 * The following functions will be useful if you choose to implement
 * multiple kernel threads per process. This is strongly discouraged
 * unless your weenix is perfect.
 */
#ifdef __MTP__
int
kthread_detach(kthread_t *kthr)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_detach");
        return 0;
}

int
kthread_join(kthread_t *kthr, void **retval)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_join");
        return 0;
}

/* ------------
 * ------------------------------------------------------ */
/* -------------------------- REAPER DAEMON ------------------------- */
/* ------------------------------------------------------------------ */
static __attribute__((unused)) void
kthread_reapd_init()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_init");
}
init_func(kthread_reapd_init);
init_depends(sched_init);

void
kthread_reapd_shutdown()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_shutdown");
}
static void *
kthread_reapd_run(int arg1, void *arg2)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_run");
        return (void *) 0;
}
#endif
