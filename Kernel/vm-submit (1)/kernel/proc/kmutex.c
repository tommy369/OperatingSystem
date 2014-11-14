#include "globals.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/kthread.h"
#include "proc/kmutex.h"

/*
 * IMPORTANT: Mutexes can _NEVER_ be locked or unlocked from an
 * interrupt context. Mutexes are _ONLY_ lock or unlocked from a
 * thread context.
 */

void
kmutex_init(kmutex_t *mtx)
{
	dbg(DBG_MM, "Initialize mutex\n");
	sched_queue_init(&mtx->km_waitq);
	mtx->km_holder = NULL;
	/*NOT_YET_IMPLEMENTED("PROCS: kmutex_init");*/
}

/*
 * This should block the current thread (by sleeping on the mutex's
 * wait queue) if the mutex is already taken.
 *
 * No thread should ever try to lock a mutex it already has locked.
 */
void
kmutex_lock(kmutex_t *mtx)
{
	KASSERT(curthr && (curthr != mtx->km_holder));
    dbg(DBG_MM, "GRADING1 5.a Current Thread is not currently holding the mutex\n");
    if(mtx->km_holder == NULL)
    {
    	mtx->km_holder = curthr;
    	/*dbg(DBG_MM, "Mut: %s\n", mtx->km_holder->kt_proc->p_comm);*/
    }
    if(curthr != mtx->km_holder && mtx->km_holder != NULL)
    {
    	/*dbg(DBG_MM, "%s\n", mtx->km_holder->kt_proc->p_comm);*/
    	sched_sleep_on(&mtx->km_waitq);
    	/*ktqueue_enqueue(&mtx->km_waitq, curthr);*/
    }
    /* NOT_YET_IMPLEMENTED("PROCS: kmutex_lock");    */
}

/*
 * This should do the same as kmutex_lock, but use a cancellable sleep
 * instead.
 */
int
kmutex_lock_cancellable(kmutex_t *mtx)
{
	KASSERT(curthr && (curthr != mtx->km_holder));
    dbg(DBG_MM, "GRADING1 5.b Current Thread is not currently holding the mutex\n");
    if(curthr != mtx->km_holder && mtx->km_holder != NULL)
    {
        int test = sched_cancellable_sleep_on(&mtx->km_waitq);
        if(test == -EINTR && curthr != mtx->km_holder)
        {
        	return -EINTR;
        }
    }
    else
    {
    	mtx->km_holder = curthr;
    	return 0;
    }
    /* AG: both given by BC ->   NOT_YET_IMPLEMENTED("PROCS: kmutex_lock_cancellable");*/
    return 0;
}

/*
 * If there are any threads waiting to take a lock on the mutex, one
 * should be woken up and given the lock.
 *
 * Note: This should _NOT_ be a blocking operation!
 *
 * Note: Don't forget to add the new owner of the mutex back to the
 * run queue.
 *
 * Note: Make sure that the thread on the head of the mutex's wait
 * queue becomes the new owner of the mutex.
 *
 * @param mtx the mutex to unlock
 */
void
kmutex_unlock(kmutex_t *mtx)
{
  KASSERT(curthr && (curthr == mtx->km_holder));
  dbg(DBG_MM, "GRADING1 5.c Current Thread is the mutex holder and now it wants to release the mutex\n");
  kthread_t *thdeq_kmwaitq = NULL;
  thdeq_kmwaitq = sched_wakeup_on(&mtx->km_waitq);
  if(thdeq_kmwaitq != NULL)
  {
	  mtx->km_holder = thdeq_kmwaitq;
	  /*sched_make_runnable(thdeq_kmwaitq);*/
  }
  else
  {
	  mtx->km_holder = NULL;
  }
  KASSERT(curthr != mtx->km_holder);
  dbg(DBG_MM, "GRADING1 5.c Current Thread no more holds the mutex\n");
  /*      NOT_YET_IMPLEMENTED("PROCS: kmutex_unlock");    */
}
