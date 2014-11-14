#include "globals.h"
#include "errno.h"

#include "main/interrupt.h"

#include "proc/sched.h"
#include "proc/kthread.h"

#include "util/init.h"
#include "util/debug.h"

static ktqueue_t kt_runq;

static __attribute__((unused)) void
sched_init(void)
{
        sched_queue_init(&kt_runq);
}
init_func(sched_init);



/*** PRIVATE KTQUEUE MANIPULATION FUNCTIONS ***/
/**
 * Enqueues a thread onto a queue.
 *
 * @param q the queue to enqueue the thread onto
 * @param thr the thread to enqueue onto the queue
 */
static void
ktqueue_enqueue(ktqueue_t *q, kthread_t *thr)
{

		dbg(DBG_MM, "ktqueue_enqueue start....\n");

		/*dbg(DBG_MM, "Proc name = %s Thread = %p Present Queue = %p Dest. Queue = %p\n",thr->kt_proc->p_comm, thr, thr->kt_wchan, q);*/
		KASSERT(!thr->kt_wchan);
		dbg(DBG_MM, "The thread is not blocked on any queue\n");
		/*dbg(DBG_MM,"link next = %p prev = %p\n",(&thr->kt_qlink)->l_next, (&thr->kt_qlink)->l_prev);*/
        list_insert_head(&q->tq_list, &thr->kt_qlink);

        thr->kt_wchan = q;
        q->tq_size++;
		/*dbg(DBG_MM, "After Enqueueu: Present Queue = %p Dest. Queue = %p Size(R) = %d\n", thr->kt_wchan, q, q->tq_size);*/
		dbg(DBG_MM, "ktqueue_enqueue end....\n");
}

/**
 * Dequeues a thread from the queue.
 *
 * @param q the queue to dequeue a thread from
 * @return the thread dequeued from the queue
 */
static kthread_t *
ktqueue_dequeue(ktqueue_t *q)
{
		dbg(DBG_MM, "ktqueue_dequeue start....\n");
        kthread_t *thr;
        list_link_t *link;
        /*dbg(DBG_MM,"Dequeue Present Queue = %p Queue Size = %d\n", q, q->tq_size);*/
        if (list_empty(&q->tq_list))
                return NULL;

        link = q->tq_list.l_prev;
        thr = list_item(link, kthread_t, kt_qlink);
        list_remove(link);
        thr->kt_wchan = NULL;

        q->tq_size--;
        /*dbg(DBG_MM,"After Dequeue: Proc = %s thread = %p Queue Size = %d\n", thr->kt_proc->p_comm, thr, q->tq_size);*/
        dbg(DBG_MM, "ktqueue_dequeue end....\n");
        return thr;
}

/**
 * Removes a given thread from a queue.
 *
 * @param q the queue to remove the thread from
 * @param thr the thread to remove from the queue
 */
static void
ktqueue_remove(ktqueue_t *q, kthread_t *thr)
{
		dbg(DBG_MM, "ktqueue_remove start....\n");
        KASSERT(thr->kt_qlink.l_next && thr->kt_qlink.l_prev);
        /*dbg(DBG_MM,"Before Remove: Queue Size = %d\n", kt_runq.tq_size);*/
        list_remove(&thr->kt_qlink);
        thr->kt_wchan = NULL;
        q->tq_size--;
        /*dbg(DBG_MM,"After Remove: Queue Size = %d\n", kt_runq.tq_size);*/
        dbg(DBG_MM, "ktqueue_remove end....\n");
}

/*** PUBLIC KTQUEUE MANIPULATION FUNCTIONS ***/
void
sched_queue_init(ktqueue_t *q)
{
		dbg(DBG_MM, "sched_queue_init start...\n");
		list_init(&q->tq_list);
        q->tq_size = 0;
        dbg(DBG_MM, "sched_queue_init end...\n");
}

int
sched_queue_empty(ktqueue_t *q)
{
        return list_empty(&q->tq_list);
}

/*
 * Updates the thread's state and enqueues it on the given
 * queue. Returns when the thread has been woken up with wakeup_on or
 * broadcast_on.
 *
 * Use the private queue manipulation functions above.
 */
void
sched_sleep_on(ktqueue_t *q)
{
        /*NOT_YET_IMPLEMENTED("PROCS: sched_sleep_on");*/
		dbg(DBG_MM, "sched_sleep_on_start....\n");
	    curthr->kt_state = KT_SLEEP;
	    /*dbg(DBG_MM, "Sleep: Enqueue in Queue = %p\n", q);*/
	    ktqueue_enqueue(q, curthr);
	    sched_switch();
	    dbg(DBG_MM, "sched_sleep_on end....\n");
}

/*
 * Similar to sleep on, but the sleep can be cancelled.
 *
 * Don't forget to check the kt_cancelled flag at the correct times.
 *
 * Use the private queue manipulation functions above.
 */
int
sched_cancellable_sleep_on(ktqueue_t *q)
{
        /*NOT_YET_IMPLEMENTED("PROCS: sched_cancellable_sleep_on");*/
		dbg(DBG_MM, "Entering cancellable sleep on function ..... \n");
		if(curthr->kt_cancelled == 1)
		{
			return -EINTR;
		}
		curthr->kt_state = KT_SLEEP_CANCELLABLE;
		/*dbg(DBG_MM, "Cancellable Sleep: Enqueue in Queue = %p\n", q);*/
		ktqueue_enqueue(q, curthr);
		sched_switch();
		dbg(DBG_MM, "Exit cancellable sleep on function ..... \n");
		if(curthr->kt_cancelled == 1)
		{
			return -EINTR;
		}
        return 0;
}

kthread_t *
sched_wakeup_on(ktqueue_t *q)
{
        /*NOT_YET_IMPLEMENTED("PROCS: sched_wakeup_on");*/
		dbg(DBG_MM, "Entering sched_wakeup_on function ..... \n");
		if(sched_queue_empty(q)) return NULL;
		else
		{
			kthread_t *ret;
			ret = ktqueue_dequeue(q);
			KASSERT(ret->kt_state == KT_SLEEP || ret->kt_state == KT_SLEEP_CANCELLABLE);
			dbg(DBG_MM, "GRADING1 4.a The thread is in the correct state : Sleep State\n");
			sched_make_runnable(ret);
			return ret;
		}
		dbg(DBG_MM, "Exiting sched_wakeup_on function ..... \n");
}

void
sched_broadcast_on(ktqueue_t *q)
{
        /*NOT_YET_IMPLEMENTED("PROCS: sched_broadcast_on");*/
	dbg(DBG_MM, "Entering sched_broadcast\n");
	while(!sched_queue_empty(q))
	{
		sched_wakeup_on(q);
	}
	dbg(DBG_MM, "Exiting sched_broadcast\n");
}

/*
 * If the thread's sleep is cancellable, we set the kt_cancelled
 * flag and remove it from the queue. Otherwise, we just set the
 * kt_cancelled flag and leave the thread on the queue.
 *
 * Remember, unless the
 *
 *
 *  thread is in the KT_NO_STATE or KT_EXITED
 * state, it should be on some queue. Otherwise, it will never be run
 * again.
 */
void
sched_cancel(struct kthread *kthr)
{
        /*NOT_YET_IMPLEMENTED("PROCS: sched_cancel");*/
		dbg(DBG_MM, "Entering sched_cancel\n");
		kthr->kt_cancelled = 1;
		if(kthr->kt_state == KT_SLEEP_CANCELLABLE)
		{
			ktqueue_remove(kthr->kt_wchan,kthr);
			sched_make_runnable(kthr);
		}
		dbg(DBG_MM, "Exiting sched_cancel\n");
}

/*
 * In this function, you will be modifying the run queue, which can
 * also be modified from an interrupt context. In order for thread
 * contexts and interrupt contexts to play nicely, you need to mask
 * all interrupts before reading or modifying the run queue and
 * re-enable interrupts when you are done. This is analagous to
 * locking a mutex before modifying a data structure shared between
 * threads. Masking interrupts is accomplished by setting the IPL to
 * high.
 *
 * Once you have masked interrupts, you need to remove a thread from
 * the run queue and switch into its context from the currently
 * executing context.
 *
 * If there are no threads on the run queue (assuming you do not have
 * any bugs), then all kernel threads are waiting for an interrupt
 * (for example, when reading from a block device, a kernel thread
 * will wait while the block device seeks). You will need to re-enable
 * interrupts and wait for one to occur in the hopes that a thread
 * gets put on the run queue from the interrupt context.
 *
 * The proper way to do this is with the intr_wait call. See
 * interrupt.h for more details on intr_wait.
 *
 * Note: When waiting for an interrupt, don't forget to modify the
 * IPL. If the IPL of the currently executing thread masks the
 * interrupt you are waiting for, the interrupt will never happen, and
 * your run queue will remain empty. This is very subtle, but
 * _EXTREMELY_ important.
 *
 * Note: Don't forget to set curproc and curthr. When sched_switch
 * returns, a different thread should be executing than the thread
 * which was executing when sched_switch was called.
 *
 * Note: The IPL is process specific.
 */
void
sched_switch(void)
{
       	   /* NOT_YET_IMPLEMENTED("PROCS: sched_switch");*/
			dbg(DBG_MM, "Switch entered........\n");
			uint8_t r = apic_getipl();
			apic_setipl(IPL_HIGH);
			kthread_t *t = ktqueue_dequeue(&kt_runq);
			kthread_t *oldThr = curthr;
			/*dbg(DBG_MM, "Current Proc = %s.....\n", curproc->p_comm);*/
			while(t == NULL)
			{
				intr_disable();
				apic_setipl(IPL_LOW);
				/*dbg(DBG_MM, "Before wait: Run Queue size = %d\n", kt_runq.tq_size);*/
				intr_wait();
				/*dbg(DBG_MM, "After wait: Run Queue size = %d\n", kt_runq.tq_size);*/
				apic_setipl(IPL_HIGH);
				t = ktqueue_dequeue(&kt_runq);
			}

			/*if(curthr == t)
			{
				apic_setipl(r);
				return;
			}*/

			/*dbg(DBG_MM, "Run queue size = %d\n", kt_runq.tq_size);
			dbg(DBG_MM, "Bef cs: current thread = %p, next thread = %p\n", curthr, t);*/
			curthr = t;
			curproc = t->kt_proc;
			apic_setipl(r);
			context_switch(&oldThr->kt_ctx, &t->kt_ctx);
			dbg(DBG_MM, "Exiting sched_switch.... \n");
}

/*
 * Since we are modifying the run queue, we _MUST_ set the IPL to high
 * so that no interrupts happen at an inopportune moment.

 * Remember to restore the original IPL before you return from this
 * function. Otherwise, we will not get any interrupts after returning
 * from this function.
 *
 * Using intr_disable/intr_enable would be equally as effective as
 * modifying the IPL in this case. However, in some cases, we may want
 * more fine grained control, making modifying the IPL more
 * suitable. We modify the IPL here for consistency.
 */
void
sched_make_runnable(kthread_t *thr)
{
        /*NOT_YET_IMPLEMENTED("PROCS: sched_make_runnable");*/
		dbg(DBG_MM, "Entering sched_make_runnable ... \n");
		KASSERT(&kt_runq != thr->kt_wchan);
		dbg(DBG_MM, "GRADING1 4.b The thread is not blocked");
		uint8_t r = apic_getipl();
		apic_setipl(IPL_HIGH);
		thr->kt_state = KT_RUN;
		/*dbg(DBG_MM, "Runnable: Enqueue thread = %p Run queue = %p\n",thr, &kt_runq);*/
		ktqueue_enqueue(&kt_runq, thr);
		apic_setipl(r);
		dbg(DBG_MM, "Exiting sched_make_runnable..... \n");
}
