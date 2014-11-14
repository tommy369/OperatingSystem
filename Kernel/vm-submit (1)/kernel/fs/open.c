/*
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int
do_open(const char *filename, int oflags)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_open");*/
		dbg(DBG_PRINT, "Entering do_open()....%s\n", filename);
        int fd = get_empty_fd(curproc);
        if(fd == -EMFILE)
        {
        	dbg(DBG_PRINT, "Exiting do_open()....\n");
        	return fd;
        }

        if((oflags & O_RDONLY) != O_RDONLY && (oflags & O_WRONLY) != O_WRONLY && (oflags & O_RDWR) != O_RDWR)
        {
        	dbg(DBG_PRINT,"None of the access modes(O_RDONLY, O_WRONLY, O_RDWR) set......\n");
        	dbg(DBG_PRINT, "Exiting do_open()....\n");
        	return -EINVAL;
        }

        if((oflags & O_RDWR) == O_RDWR && (oflags & O_WRONLY) == O_WRONLY)
        {
        	dbg(DBG_PRINT, "Invalid flags set.........\n");
        	dbg(DBG_PRINT, "Exiting do_open()....\n");
        	return -EINVAL;
        }

        if((oflags & O_TRUNC) == O_TRUNC)
        {
        	if((oflags & O_RDWR) != O_RDWR && (oflags & O_WRONLY) != O_WRONLY)
        	{
        		dbg(DBG_PRINT, "O_TRUNC set but none of the write flags are set...\n");
        		dbg(DBG_PRINT, "Exiting do_open()....\n");
        		return -EINVAL;
        	}
        }

        if((oflags & O_APPEND) == O_APPEND)
        {
        	if((oflags & O_RDWR) != O_RDWR && (oflags & O_WRONLY) != O_WRONLY)
        	{
        		dbg(DBG_PRINT, "O_APPEND set but none of the write flags are set...........\n");
        		dbg(DBG_PRINT, "Exiting do_open()....\n");
        		return -EINVAL;
        	}
        }

        int count = 0;

        /*const char *p = filename;*/

        if(strlen(filename) > MAXPATHLEN) return -ENAMETOOLONG;

        file_t *sft = fget(-1);
        if(sft == NULL) return ENOMEM; /* not sure ..........................*/

        curproc->p_files[fd] = sft;

        if((oflags & O_WRONLY) == O_WRONLY)
        {
        	sft->f_mode = FMODE_WRITE;
        }

        else if((oflags & O_RDWR) == O_RDWR)
        {
            sft->f_mode = FMODE_WRITE | FMODE_READ;
        }

        else
        {
          	sft->f_mode = FMODE_READ;
        }

        if((oflags & O_APPEND) == O_APPEND)
        {
        	sft->f_mode |= FMODE_APPEND;
        }

        vnode_t *res_vnode;

        /*vfs_is_in_use(vfs_root_vn->vn_fs);*/
        int openv_ret = open_namev(filename, oflags, &res_vnode, NULL);
        if(openv_ret < 0)
        {
        	/*dbg(DBG_PRINT, "do_open()\n", openv_ret);*/
        	fput(sft);  /*Not Sure................................*/
        	if(openv_ret == -ENOENT && (oflags & O_CREAT) != O_CREAT)
        	{
        		dbg(DBG_PRINT, "Exiting do_open()....\n");
        		return -ENOENT;
        	}
        	else
        	{
        		dbg(DBG_PRINT, "Exiting do_open()....\n");
        		return openv_ret;
        	}
        }

        if(S_ISDIR(res_vnode->vn_mode) && ((oflags & O_WRONLY) == O_WRONLY || (oflags & O_RDWR) == O_RDWR))
        {
        	dbg(DBG_PRINT, "Exiting from do_open()\n");
        	fput(sft);
        	vput(res_vnode);
        	return -EISDIR;
        }

        sft->f_pos = 0;
        sft->f_vnode = res_vnode;
        /*sft->f_refcount += 1;*/

        if((oflags & O_TRUNC) == O_TRUNC)
        {
        	sft->f_vnode->vn_len = 0;

        }

        if(S_ISCHR(sft->f_vnode->vn_mode))
        {
        	if(sft->f_vnode->vn_cdev == NULL)
        	{
        		dbg(DBG_PRINT, "Exiting do_open()....\n");
        		/*vput(res_vnode);*/
        		return -ENXIO;
        	}
        }

        if(S_ISBLK(sft->f_vnode->vn_mode))
        {
            if(sft->f_vnode->vn_bdev == NULL)
            {
            	dbg(DBG_PRINT, "Exiting do_open()....\n");
            	/*vput(res_vnode);*/
                return -ENXIO;
            }
        }
        dbg(DBG_PRINT, "Exiting do_open() normally....\n");
        return fd;
}
