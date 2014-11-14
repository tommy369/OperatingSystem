#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void
proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        /*dbg(DBG_MM, "lkgsdl\n");*/
        while (1)
        {
failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
                {
                	/*dbg(DBG_MM, "lkgsdl\n");*/
                        if(p->p_pid == pid)
                        {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid)
                                {
                                        return -1;
                                }
                                else
                                {
                                	/*dbg(DBG_MM, "lkgsdl\n");*/
                                	goto failed;
                                }
                        }
                } list_iterate_end();
                /*dbg(DBG_MM, "lkgsdl\n");*/
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_create");*/

    	dbg(DBG_MM, "Starting process creation.....\n");
        proc_t *p = (proc_t *)(slab_obj_alloc(proc_allocator));

        strncpy(p->p_comm, name, PROC_NAME_LEN);
        /*dbg(DBG_MM, "Assigned process name.....%s\n", p->p_comm);*/
        p->p_pid = _proc_getid();
        KASSERT(PID_IDLE != p->p_pid || list_empty(&_proc_list));
        dbg(DBG_MM, "GRADING1 2.a Process Id is PID_IDLE if it is the first process, else process id is correctly assigned to it\n");
        KASSERT(PID_INIT != p->p_pid || PID_IDLE == curproc->p_pid);
        dbg(DBG_MM, "GRADING1 2.a Process Id is PID_INIT if created from idle process\n");

        p->p_state = PROC_RUNNING;
        /*dbg(DBG_MM, "Assigned process state.....\n");*/
        p->p_status = 0;

        /*Initializing List structs*/
        list_init(&p->p_children);
        list_init(&p->p_threads);
        list_link_init(&p->p_list_link);
        list_link_init(&p->p_child_link);

        p->p_pagedir = pt_create_pagedir();

        /*Initializes wait queue*/
        sched_queue_init(&p->p_wait);

        /*Adds process on the proc list*/
        list_insert_tail(&_proc_list, &p->p_list_link);

        if(p->p_pid == 0)
        {
        	p->p_pproc = p;

        }
        else if(p->p_pid == 1)
        {
        	proc_initproc = p;
        	p->p_pproc = curproc;
        	list_insert_tail(&curproc->p_children, &p->p_child_link);
        }
        else
        {
        	p->p_pproc = curproc;
        	list_insert_tail(&curproc->p_children, &p->p_child_link);
        }

        /*VFS*/
        /*if(p->p_pid == 2)
        {
        	p->p_cwd = NULL;
        }
        else if(p->p_pid != 0 && p->p_pid != 1)
        {

        	p->p_cwd = p->p_pproc->p_cwd;
        	if(p->p_cwd) vref(p->p_cwd);
        }*/


        int i = 0;
        for(i = 0; i < NFILES; i++)
        {
        	p->p_files[i] = NULL;
        }

        if(curproc) p->p_cwd = curproc->p_cwd;
        else p->p_cwd = vfs_root_vn;

        if(p->p_cwd && p->p_pid != 2)
        	vref(p->p_cwd);

        p->p_vmmap = vmmap_create();
        p->p_vmmap->vmm_proc = p;

        return p;
		/*
        return NULL;*/
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void
proc_cleanup(int status)
{
	/*NOT_YET_IMPLEMENTED("PROCS: proc_cleanup");*/
	KASSERT(NULL != proc_initproc);
	dbg(DBG_MM, "GRADING1 2.b Init process is present and proc_initproc is pointint to it\n");
	KASSERT(1 <= curproc->p_pid);
	dbg(DBG_MM, "GRADING1 2.b Current Process is not idle\n");
	KASSERT(NULL != curproc->p_pproc);
	dbg(DBG_MM, "GRADING1 2.b Current process has a parent and it is not null\n");
	curproc->p_state = PROC_DEAD;
	/*curproc->p_status = status;*/
	if(curproc->p_pproc->p_wait.tq_size != 0)
		sched_wakeup_on(&curproc->p_pproc->p_wait);
	proc_t *child;

	int i;
	for(i = 0; i < NFILES; i++)
	{
		if(curproc->p_files[i] != NULL && curproc->p_files[i]->f_refcount > 0)
		{
			/*curproc->p_files[i]->f_refcount = 1;*/
			do_close(i);
		}
	}

	if(curproc->p_pid != 2)
	{
		vput(curproc->p_cwd);
	}

	vmmap_destroy(curproc->p_vmmap);

	if(curproc != proc_initproc)
	{
		list_iterate_begin(&curproc->p_children, child, proc_t, p_child_link)
		{
			child->p_pproc = proc_initproc;
			dbg(DBG_MM, "Reparenting....................................\n");
			/*However, the init process should not
			  have any children at the time it exits).*/
			list_remove(&child->p_child_link);
			list_insert_tail(&proc_initproc->p_children, &child->p_child_link);
		}list_iterate_end();
	}
	list_remove(&curproc->p_list_link);
	sched_switch();
	KASSERT(NULL != curproc->p_pproc);
	dbg(DBG_MM, "GRADING1 2.b Current process has a parent and it is not null\n");
	dbg(DBG_MM, "proc_cleanup exits...\n");
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_kill");*/
		dbg(DBG_MM, "Enters proc_kill......\n");
		if(p == curproc)
			do_exit(status);
		else
		{
			kthread_t *t1;
			list_iterate_begin(&(p->p_threads),t1,kthread_t,kt_plink)
			{
				kthread_cancel(t1,t1->kt_retval);
				/*if(curthr != t1) kthread_destroy(t1);*/
			}list_iterate_end();
		}
		dbg(DBG_MM, "Exits proc_kill......\n");

}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
       /* NOT_YET_IMPLEMENTED("PROCS: proc_kill_all");*/
	dbg(DBG_MM, "Enters proc_kill_all......\n");
		proc_t *p1;
		list_iterate_begin(&_proc_list,p1,proc_t,p_list_link)
		{
			if(p1 !=curproc )
			{
				if(p1->p_pproc->p_pid!= PID_IDLE)
				{
					if(p1->p_pid != PID_IDLE)
					{
						if(p1->p_state!=PROC_DEAD)
						{
							proc_kill(p1, 1);
						}
					}
				}
			}
		}list_iterate_end();
		dbg(DBG_MM, "Exits proc_kill_all......\n");
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p->p_pid == pid) {
                        return p;
                }
        } list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{
        /*NOT_YET_IMPLEMENTED("PROCS: proc_thread_exited");*/
	dbg(DBG_MM, "proc_thread_exited entered...\n");
	proc_cleanup((int)retval);
	dbg(DBG_MM, "proc_thread_exited exited.....\n");
}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t
do_waitpid(pid_t pid, int options, int *status)
{
    /*NOT_YET_IMPLEMENTED("PROCS: do_waitpid");*/
	dbg(DBG_MM, "do_waitpid entered....\n");
	proc_t *ch;
	if(list_empty(&curproc->p_children))
	{
		return -ECHILD;
	}
	/*proc_t *ch;*/

	if(pid == -1)
	{
		do
		{
			list_iterate_begin(&curproc->p_children, ch, proc_t, p_child_link)
			{
				KASSERT(NULL != ch);
				dbg(DBG_MM, "GRADING1 2.c Child process is not null\n");
				if(ch->p_state == PROC_DEAD)
				{
					KASSERT(-1 == pid || ch->p_pid == pid);
					dbg(DBG_MM, "GRADING1 2.c Valid process that is to be exited is found (Process Name = %s)\n", ch->p_comm);
					kthread_t *f;
					list_iterate_begin(&ch->p_threads, f, kthread_t, kt_plink)
					{
						if(f->kt_state == KT_EXITED)
						{
							KASSERT(KT_EXITED == f->kt_state);
							dbg(DBG_MM, "GRADING1 2.c Thread to be destroyed has a valid status (KT_EXITED)\n");
							kthread_destroy(f);
						}
					}list_iterate_end();

					list_remove(&ch->p_child_link);

					if(status) *status = ch->p_status;
					int ret = ch->p_pid;

					KASSERT(NULL != ch->p_pagedir);
					dbg(DBG_MM, "GRADING1 2.c Process has a page directory\n");
					pt_destroy_pagedir(ch->p_pagedir);

					slab_obj_free(proc_allocator, ch);
					return ret;
				}
			}
			list_iterate_end();
			sched_sleep_on(&curproc->p_wait);
		}while(1);
	}
	if(pid > 0)
	{
		list_iterate_begin(&curproc->p_children, ch, proc_t, p_child_link)
		{
			KASSERT(NULL != ch);
			dbg(DBG_MM, "GRADING1 2.c Child Process is not NULL\n");
			if(pid == ch->p_pid)
			{
				KASSERT(-1 == pid || ch->p_pid == pid);
				dbg(DBG_MM, "GRADING1 2.c Valid process that is to be exited is found(Process Name = %s)\n", ch->p_comm);
				do
				{
					/*dbg(DBG_MM, "ch->p_state = %d\n", ch->p_state);*/
					if(ch->p_state != PROC_DEAD)
					{
						/*dbg(DBG_MM, "Not dead\n");*/
						sched_sleep_on(&curproc->p_wait);
					}
					else
					{
						/*dbg(DBG_MM, "Dead\n");*/
						kthread_t *f;
						list_iterate_begin(&ch->p_threads, f, kthread_t, kt_plink)
						{
							if(f->kt_state == KT_EXITED)
							{
								KASSERT(KT_EXITED == f->kt_state);
								dbg(DBG_MM, "GRADING1 2.c Thread to be destroyed has a valid status (KT_EXITED)\n");
								kthread_destroy(f);
							}
						}
						list_iterate_end();
						list_remove(&ch->p_child_link);
						/*list_remove(&ch->p_list_link);*/
						*status = ch->p_status;
						/*dbg(DBG_MM, "Status = %d\n", *status);*/
						int ret = ch->p_pid;
						KASSERT(NULL != ch->p_pagedir);
						dbg(DBG_MM, "GRADING1 2.c Process has a page directory\n");
						pt_destroy_pagedir(ch->p_pagedir);

						slab_obj_free(proc_allocator, ch);
						return ret;
					}
				}while(1);
			}
		}list_iterate_end();
		return -ECHILD;
	}
	dbg(DBG_MM, "do_waitpid exited...\n");
    return 0;
}

/*
 * Cancel all threads, join with them, and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
        /*NOT_YET_IMPLEMENTED("PROCS: do_exit");*/
		dbg(DBG_MM, "do_exit entered (%d)...\n", status);
        kthread_t *th;
        curproc->p_status = status;
        list_iterate_begin(&curproc->p_threads, th, kthread_t,kt_plink)
        {
        	if(th != curthr) kthread_cancel(th, th->kt_retval);
        	/*kthread_join(th,&(th->kt_retval));*/
        }list_iterate_end();
        kthread_exit((void *)status);
        dbg(DBG_MM, "do_exit exited...\n");
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
		dbg(DBG_MM, "proc_info entered...\n");
        const proc_t *p = (proc_t *) arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc) {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        } else {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
                ++count;
        } list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children)) {
                iprintf(&buf, &size, "children:     -\n");
        } else {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        } list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd) {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        } else {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif
        dbg(DBG_MM, "....\n");
        dbg(DBG_MM, "proc_info exited...\n");
        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
		dbg(DBG_MM, "proc_list_info...\n");
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                char parent[64];
                if (NULL != p->p_pproc) {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                } else {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd) {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                } else {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        } list_iterate_end();
        dbg(DBG_MM, "proc_list_info exited...\n");
        return size;
}
