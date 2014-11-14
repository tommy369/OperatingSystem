#include "types.h"
#include "globals.h"
#include "kernel.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/cpuid.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/tty/virtterm.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "errno.h"

#ifdef __KERNEL__

#include "kernel.h"
#include "globals.h"
#include "errno.h"
#include "config.h"
#include "limits.h"

#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "fs/dirent.h"
#include "fs/vfs_syscall.h"
#include "fs/stat.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/mman.h"
#include "mm/kmalloc.h"

#include "test/usertest.h"

#endif

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

extern void *testproc(int, void*);
extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void normal_test();
extern int vfstest_main(int, char**);
extern void *renametest();

static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);
static void       hard_shutdown(void);

static context_t bootstrap_context;

static int gdb_wait = GDBWAIT;
/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
#ifdef __DRIVERS__

        int do_foo(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
            dbg(DBG_INIT, "(GRADING): do_foo() is invoked, argc = %d, argv = 0x%08x\n",
                    argc, (unsigned int)argv);
            return 0;
        }

    #endif /* __DRIVERS__ */

void
kmain()
{
        GDB_CALL_HOOK(boot);

        dbg_init();
        dbgq(DBG_CORE, "Kernel binary:\n");
        dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
        dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
        dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

        page_init();

        pt_init();
        slab_init();
        pframe_init();

        acpi_init();
        apic_init();
        intr_init();

        gdt_init();

        /* initialize slab allocators */
#ifdef __VM__
        anon_init();
        shadow_init();
#endif
        vmmap_init();
        proc_init();
        kthread_init();

#ifdef __DRIVERS__
        bytedev_init();
        blockdev_init();
#endif

        void *bstack = page_alloc();
        pagedir_t *bpdir = pt_get();
        KASSERT(NULL != bstack && "Ran out of memory while booting.");
	/* This little loop gives gdb a place to synch up with weenix.  In the
	 * past the weenix command started qemu was started with -S which
	 * allowed gdb to connect and start before the boot loader ran, but
	 * since then a bug has appeared where breakpoints fail if gdb connects
	 * before the boot loader runs.  See
	 *
	 * https://bugs.launchpad.net/qemu/+bug/526653
	 *
	 * This loop (along with an additional command in init.gdb setting
	 * gdb_wait to 0) sticks weenix at a known place so gdb can join a
	 * running weenix, set gdb_wait to zero  and catch the breakpoint in
	 * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
	 *
	 * DANGER: if GDBWAIT != 0, and gdb isn't run, this loop will never
	 * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
	 * you expect.
	 */
      	while (gdb_wait) ;
        context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
        context_make_active(&bootstrap_context);

        panic("\nReturned to kmain()!!!\n");
}

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
bootstrap(int arg1, void *arg2)
{
        /* necessary to finalize page table information */
	    dbg(DBG_MM, "Bootstrap entered...\n");
        pt_template_init();
        /*NOT_YET_IMPLEMENTED("PROCS: bootstrap");*/
        dbg(DBG_MM, "Creating idle process.....\n");
		proc_t *idle = proc_create("idle");
		curproc = idle;
		KASSERT(NULL != curproc);
		dbg(DBG_MM, "GRADING1 1.a Idle process has been created successfully\n");
		KASSERT(PID_IDLE == curproc->p_pid);
		dbg(DBG_MM, "GRADING1 1.a Idle process has been assigned correct process id (pid = 0)\n");

		/*dbg(DBG_MM,"Creating a new thread of the idle process....\n");*/
		kthread_t *idle_thread = kthread_create(idle, idleproc_run, 0, NULL);
		curthr = idle_thread;
		KASSERT(NULL != curthr);
		dbg(DBG_MM, "GRADING1 1.a Thread for idle process has been created successfully\n");

		/*dbg(DBG_MM, "Activating the context...\n");*/
		context_make_active(&curthr->kt_ctx);
		dbg(DBG_MM, "Bootstrap exited...\n");
        /*panic("weenix returned to bootstrap()!!! BAD!!!\n");*/
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
idleproc_run(int arg1, void *arg2)
{
		dbg(DBG_MM, "idleproc_run enters.....\n");
		int status;
        pid_t child;
		/*dbg(DBG_MM, "Running idleproc_run(%d, %s)....\n", arg1, (char *)arg2);*/
        /* create init proc */
        kthread_t *initthr = initproc_create();

        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */


#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        curproc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);

        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);


        do_mkdir("/dev");
        do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID);
        do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID);
        do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2,0));
        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        /*NOT_YET_IMPLEMENTED("VFS: idleproc_run");*/
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);
        dbg(DBG_MM, "Init process has been exited\n");


#ifdef __MTP__
        kthread_reapd_shutdown();
#endif



#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        /*vput(vfs_root_vn);*/
        /*vput(vfs_root_vn);*/
        /*vput(vfs_root_vn);*/
        /*vput(curproc->p_cwd);*/
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif



        /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
        hard_shutdown();
        dbg(DBG_MM, "idleproc_run exited...\n");
        return NULL;
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
static kthread_t *
initproc_create(void)
{
		dbg(DBG_MM, "initproc_create entered.....\n");
		/*NOT_YET_IMPLEMENTED("PROCS: initproc_create");*/

        /*dbg(DBG_MM, "Creating init process.....\n");*/
        proc_t *init = proc_create("init");

        KASSERT(NULL != init);
        dbg(DBG_MM, "GRADING1 1.b Pointer to init process is correct\n");

        KASSERT(PID_INIT == init->p_pid);
        dbg(DBG_MM, "GRADING1 1.b Points to init process whose pid = PID_INIT (pid = 1)\n");

        /*Creating thread of init process(Not sure about zero and NULL)*/
        kthread_t *init_thread = kthread_create(init, initproc_run, 0, NULL);
        KASSERT(init_thread != NULL);
        dbg(DBG_MM, "GRADING1 1.b Init thread has been created successfully and its not NULL\n");
        return init_thread;
}

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/bin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */

/*Test Case.........................................*/


void *vfstest()
{
	vfstest_main(1, NULL);
	return NULL;
}



#define errno (curthr->kt_errno)

#define ksyscall(name, formal, actual)          \
        static int ksys_ ## name formal {       \
                int ret = do_ ## name actual ;  \
                if(ret < 0) {                   \
                        errno = -ret;           \
                        return -1;              \
                }                               \
                return ret;                     \
        }

ksyscall(close, (int fd), (fd))
ksyscall(read, (int fd, void *buf, size_t nbytes), (fd, buf, nbytes))
ksyscall(write, (int fd, const void *buf, size_t nbytes), (fd, buf, nbytes))
ksyscall(dup, (int fd), (fd))
ksyscall(dup2, (int ofd, int nfd), (ofd, nfd))
ksyscall(mkdir, (const char *path), (path))
ksyscall(rmdir, (const char *path), (path))
ksyscall(link, (const char *old, const char *new), (old, new))
ksyscall(unlink, (const char *path), (path))
ksyscall(rename, (const char *oldname, const char *newname), (oldname, newname))
ksyscall(chdir, (const char *path), (path))
ksyscall(lseek, (int fd, int offset, int whence), (fd, offset, whence))
ksyscall(getdent, (int fd, struct dirent *dirp), (fd, dirp))
ksyscall(stat, (const char *path, struct stat *uf), (path, uf))
ksyscall(open, (const char *filename, int flags), (filename, flags))
#define ksys_exit do_exit

#define mkdir(a,b)      ksys_mkdir(a)
#define rmdir           ksys_rmdir
#define mount           ksys_mount
#define umount          ksys_umount
#define open(a,b,c)     ksys_open(a,b)
#define close           ksys_close
#define link            ksys_link
#define rename          ksys_rename
#define unlink          ksys_unlink
#define read            ksys_read
#define write           ksys_write
#define lseek           ksys_lseek
#define dup             ksys_dup
#define dup2            ksys_dup2
#define chdir           ksys_chdir
#define stat(a,b)       ksys_stat(a,b)
#define getdents(a,b,c) ksys_getdents(a,b,c)
#define exit(a)         ksys_exit(a)



#define syscall_fail(expr, err)                                                                 \
        (test_assert((errno = 0, -1 == (expr)), "\nunexpected success, wanted %s (%d)", test_errstr(err), err) ? \
         test_assert((expr, errno == err), "\nexpected %s (%d)"                                 \
                     "\ngot      %s (%d)",                                                      \
                     test_errstr(err), err,                                                     \
                     test_errstr(errno), errno) : 0)
#define syscall_success(expr)                                                                   \
        test_assert(0 <= (expr), "\nunexpected error: %s (%d)",                                 \
                    test_errstr(errno), errno)
#define create_file(file)                                                                       \
        do {                                                                                    \
                int __fd;                                                                       \
                if (syscall_success(__fd = open((file), O_RDONLY|O_CREAT, 0777))) {             \
                        syscall_success(close(__fd));                                           \
                }                                                                               \
        } while (0);

void *renametest()
{
	/*int fd = do_open("old", O_RDONLY | O_CREAT);*/
	int fd;
    /*syscall_success(fd = open("/usr/bin/hello", O_RDONLY, 0));*/
	syscall_success(fd = open("old", O_RDONLY | O_CREAT, 0));
	struct stat s;
	syscall_success(do_stat(".", &s));
	syscall_success(rename("old", "new"));
	syscall_fail(rename("old", "name"), ENOENT);

	syscall_success(close(fd));

	return NULL;
}


void testing(int arg1, void *arg2)
{
	dbg(DBG_MM, "Execution of Process %d begins......\n",arg1);
	char *argv[] = { NULL };
	char *envp[] = { NULL };
	kernel_execve("/usr/bin/hello", argv, envp);
	return;
}

void *hello()
{
	/*int fd = do_open("old", O_RDONLY | O_CREAT);*/
	int fd;
	proc_t *p = proc_create("Hello");
	kthread_t *t = kthread_create(p, (kthread_func_t)testing, 0, NULL);
	sched_make_runnable(t);

	int status;
	do_waitpid(3, 0, &status);
	return NULL;
}

void *segfault()
{
	/*int fd = do_open("old", O_RDONLY | O_CREAT);*/
	int fd;
	char *argv[] = { NULL };
	char *envp[] = { NULL };
	kernel_execve("/usr/bin/segfault", argv, envp);
	return NULL;
}

void *args()
{
	/*int fd = do_open("old", O_RDONLY | O_CREAT);*/
	int fd;
	char *argv[] = { NULL };
	char *envp[] = { NULL };
	/*char *envp[] = "hithere";*/
	kernel_execve("/usr/bin/args", argv, envp);
	return NULL;
}

void *uname()
{
	/*int fd = do_open("old", O_RDONLY | O_CREAT);*/
	int fd;
	char *argv[] = { NULL };
	char *envp[] = { NULL };
	kernel_execve("/bin/uname", argv, envp);
	return NULL;
}

/*Test Case Ends....................................*/

static void *
initproc_run(int arg1, void *arg2)
{
		dbg(DBG_MM, "initproc_run starts...\n");

		/*NOT_YET_IMPLEMENTED("PROCS: initproc_run");*/
        /*int k = (int)testproc(0, NULL);
        int k1 = (int)sunghan_test(0, NULL);
        int k2 = (int)sunghan_deadlock_test(0, NULL);*/
		char *argv[] = { NULL };
		char *envp[] = { NULL };
		kernel_execve("/sbin/init", argv, envp);
		/*kernel_execve("/usr/bin/vfstest", argv, envp);*/
		/*kernel_execve("/usr/bin/fork-and-wait", argv, envp);*/
#ifdef __DRIVERS__

        /*kshell_add_command("testproc", (kshell_cmd_func_t)testproc, "invoke testproc");
        kshell_add_command("sunghan", (kshell_cmd_func_t)sunghan_test, "invoke sunghan_test to test producer/consumer problem");
        kshell_add_command("deadlock",(kshell_cmd_func_t)sunghan_deadlock_test, "invoke deadlock_test to test deadlock");
        kshell_add_command("vfstest",(kshell_cmd_func_t)vfstest, "File Test");
        kshell_add_command("renametest",(kshell_cmd_func_t)renametest, "Rename Test");
        kshell_add_command("hello",(kshell_cmd_func_t)hello, "Hello Test");
        kshell_add_command("segfault",(kshell_cmd_func_t)segfault, "Segfault Test");
        kshell_add_command("args",(kshell_cmd_func_t)args, "Args Test");
        kshell_add_command("uname",(kshell_cmd_func_t)uname, "Uname Test");

        dbg(DBG_PRINT, "Creating kshell.......\n");
        kshell_t *kshell = kshell_create(0);
        if (NULL == kshell) panic("init: Couldn't create kernel shell\n");

        while (kshell_execute_next(kshell));

        kshell_destroy(kshell);*/

    #endif /* __DRIVERS__ */

		dbg(DBG_MM, "initproc_run ends....\n");
        return NULL;
}

/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void
hard_shutdown()
{
#ifdef __DRIVERS__
        vt_print_shutdown();
#endif
        __asm__ volatile("cli; hlt");
}
