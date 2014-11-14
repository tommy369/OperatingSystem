CSCI 402 KERNEL ASSIGNMENT - 1
README

(AM SECTION)

procs-README
-------------
TEAM MEMBERS – 
    ANUJ GUPTA             (anuj@usc.edu)                                                                                                           
	JARIN SHAH             (jarinnis@usc.edu)					                         
	SAYALI SAKHALKAR       (sakhalka@usc.edu) 
	SNEHA VIJAYAN          (snehavij@usc.edu)
	
SCORE SPLIT – EQUAL SHARE SPLIT (25% to each team member)

JUSTFICATION –
We divided the whole development of Kernel Assignment 1 among our team by making 2 sub-teams who collaborated whenever code dependencies were spotted in a sub-team’s tasks.
Sub-Team1 – Sayali Sakhalkar and Jarin Shah                                                                                                         
Sub-Team2 – Anuj Gupta and Sneha Vijayan

Tasks performed by Sub-Team1 – wrote code of kmain.c, kthread.c and some functions of proc.c.
Tasks performed by Sub-Team2 – wrote code of kmutex.c, sched.c and some functions of proc.c

Testing was done collaboratively by all the 4 team members. 

RUNNING WEENIX
--------------

1.  Compile Weenix:

    $ make

2.  Invoke Weenix:

    $ ./weenix -n
	
    or, to run Weenix under gdb, run:

    $ ./weenix -n -d gdb   

ASSUMPTIONS
-----------
	/* UNKA   a) Kernel Thread must run to execution or block on cancellable sleep. */
	b) Proc_Kill call Kthread_Cancel. 
	c) Not implementing any of the MTP (Multi-threaded functionality).


TEST CASES
----------
/*    UNKA 
	1) Implicit Kthread exit via return
	2) Calling proc_kill_all on a list of threads in a KT_SLEEP_CANCELLABLE STATE
	3) Calling proc_kill_all on a list of threads in a KT_RUN.
	4) Deadlock scenario with proc_kill_all
	5) Producer-Consumer problem.
	6) Reparenting after do_exit()

   unka         */
   
EXPLANATION OF FUNCTIONS
------------------------
I) KMAIN:
1) kmain.c/bootstrap():
	Initializes the page table template 
	Making current process as idle process and checks whether idle process has been created successfuly
	Creates the idle thread and checks whether thread has been created successfuly 
	Activates the context
2) kmain.c/initproc_create():
	Init process created 
	Checks whether init process is created successfully and checks whether the pointer to init process is right
	Creates init thread and checks whether it is created successfully
3) kmain.c/initproc_run():
	It invokes the function testproc in which all test cases are written
	All the test cases are run one by one by invoking the function
4) kmain.c/idleproc_run():
	Invoke the process creating init process
	Initialises all the processes and its threads
	Enables interrupts and runs initproc()
	Waits for one of the child process to exit
	
II) PROC/proc.c:

1. proc_create():
	pulls a proc_t off the slab and initializes it
	
2. proc_cleanup():
	called after in kthread_exit() call path after all threads exit.
	does all in process cleanup. kthread_destory left for outside.
	
3. proc_kill():
	calls kthread_cancel on all threads of a proc, or calls do_exit if
	proc is the current proc
	
4. proc_kill_all():
	any kernel thread can call this to shutdown the system cleanly.  All
	procs except 0,1,2 are canceled here.
	
5. proc_thread_exited():
	mostly for MTP, this would check if the exited thread was the last
	thread in a process, but since we did not implement MTP, this just
	calls proc_cleanup	
6. do_waitpid():
	waits on a child pid and does kthread_destroy on all its threads 
	to reclaim that space.
	
7. do_exit():
	called by any thread of a process to kill all process threads and
	exit the process. Again, since MTP was not implemented this
	just behaves as kthread_exit()


III) PROC/kthread.c:

1. kthread_create():
	/*pulls kthread_t off slab and initializes it*/
 	Checks whether the associated process is not NULL]
	Creates new thread and assigns its state, stack and process
	Initialises the links of the thread
	Inserting the new thread in the tail of the currently created thread
	
	
2. kthread_cancel(): 
	/*used to kill a thread thats not the current thread. Very powerfull
	and not protected against process to process hate crimes*/
	If the thread to be cancelled is the current thread then it exits kthread
	Otherwise set the attribute of the thread(cancelled) as 1
	Set the return value of the kthread
	If the state of the thread is KT_SLEEP_CANCELLABLE then call sched_wakeup_on

3. kthread_exit():
	/*implitly called via return 0, this is normal thread exit and 
	cleanup call path*/
	Check if wait queue is empty
	Checks if the process associated with the current thread is the current process
	
4. kthread_init()
	/*Allocates memory using slab_allocator to thread*/
	Creates slab allocator and cheks if it is NULL

IV) PROC/sched.c:

1. sched_sleep_on():
	give up processor to wait on a q. Cant be pulled off the q to be
	canceled.

2. sched_cancellable_sleep_on():
	break point type sleep so cancellable into or out of wait q

3. sched_wakeup_on():
	take head of q and put on runq

4. sched_broadcast_on():
	put whole q on runq

5. sched_cancel():
	used to cancel a thread which is on a waitq

6. sched_switch():
	this is the heart of the scheduler, which exists in every threads
	context.  if there are no threads to run, it waits in a blocking
	wait for interupts. IPL_LOW before to catch all interupts, and return 
	IPL_HIGH after wards to proctect the runq from interrupt context
	collisions.

7. sched_make_runnable():
	sets state the KT_RUN and puts on runq



V) PROC/kmutex.c:

1. kmutex_init():
	Sets the initial holder of mutex to Null and initializes mutex queue.
2. kmutex_lock():
	If there is no holder of mutex, the function gives it to the current thread. 
	Otherwise, it makes the current thread goto sleep and wait for the queue. 
	On waking up, it gives the mutex to the queue and makes it leave the function.
3. kmutex_lock_cancellable():
	It does the same function as above, however the sleep is sleep_cancellable.
4. kmutex-unlock():
	It makes the next thread on the mutex queue as the new holder of the mutex.


REFERENCES
-----------
Producer-Consumer: http://cis.poly.edu/