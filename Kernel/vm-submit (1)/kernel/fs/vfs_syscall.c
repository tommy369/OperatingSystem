/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.1 2012/10/10 20:06:46 william Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read f_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)        /* internal read ka errno returned not checked */
{
	    dbg(DBG_PRINT, " Entering do_read()....fd = %d bytes = %d\n", fd, nbytes);

	    if(fd < 0 || fd >= NFILES)
	    {
	    	dbg(DBG_PRINT, "Exiting read() due to EBADF\n");
	    	return -EBADF;
	    }

	    file_t *sfte = fget(fd);

	    if(sfte == NULL)
	    {
	    	/* fd is not a valid file descriptor */
	    	dbg(DBG_PRINT, " Exiting do_read()(EBADF).....\n");
	    	return -EBADF;
	    }
	    /*dbg(DBG_PRINT, "%d \n", sfte->f_mode & FMODE_READ);*/
	    if(S_ISDIR(sfte->f_vnode->vn_mode))
	    {
	    	/* fd refers to a directory */
	    	/*dbg(DBG_PRINT, "sfte1 = %p\n", sfte);*/
	    	fput(sfte);
	    	dbg(DBG_PRINT, " Exiting do_read() (EISDIR).....\n");
	    	return -EISDIR;
	    }
	    if((sfte->f_mode & FMODE_READ) != FMODE_READ)      /* not sure about the file ka mode checking */
	    {
	    	/*dbg(DBG_PRINT, "sfte2 = %p\n", sfte);*/
	    	/* file is not open for reading. */
	    	fput(sfte);
	    	dbg(DBG_PRINT, " Exiting do_read() (EBADF).....\n");
	        return -EBADF;
	    }

	    if(nbytes == 0) /* Not sure about its placement (above or here)..........*/
	    {
	    	/*dbg(DBG_PRINT, "sfte3 = %p\n", sfte);*/
	    	fput(sfte);
	    	dbg(DBG_PRINT, " Exiting do_read() (EBADF).....\n");
	    	return 0;
	    }
	    /*dbg(DBG_PRINT, " Exiting \n");*/
        int nbr = sfte->f_vnode->vn_ops->read(sfte->f_vnode, sfte->f_pos, buf, nbytes);    /* number of bytes read */
	    if(nbr < 0)
	    {
	    	dbg(DBG_PRINT, " Exiting do_read() (EBADF).....\n");
	    	fput(sfte);
	    	return nbr;
	    }
	    /*else sfte->f_pos = sfte->f_pos + nbr;*/
	    if(nbr == (int)nbytes)
	    {
	    	do_lseek(fd, nbytes, SEEK_CUR);
	    }
	    else
	    {
	    	do_lseek(fd, 0, SEEK_END);
	    }

	    fput(sfte);
	    dbg(DBG_PRINT, " Exiting do_read().....\n");
	    return nbr;

        /*NOT_YET_IMPLEMENTED("VFS: do_read");
        return -1;*/
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * f_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
	dbg(DBG_PRINT, "Entering do_write()...");

	if(fd < 0 || fd >= NFILES)
	{
		dbg(DBG_PRINT, "Exiting do_write() (EBADF).....\n");
		return -EBADF;
	}

	file_t *sfte = fget(fd);
    if(sfte == NULL)
    {
    	/* fd is not a valid file descriptor */
    	dbg(DBG_PRINT, "Exiting do_write() (EBADF).....\n");
    	return -EBADF;
    }
    if(S_ISDIR(sfte->f_vnode->vn_mode))
    {
         /* fd refers to a directory */
    	fput(sfte);
    	dbg(DBG_PRINT, "Exiting do_write() (EISDIR).....\n");
    	return -EISDIR;
    }
    if(((sfte->f_mode & FMODE_WRITE) != FMODE_WRITE) && ((sfte->f_mode & FMODE_APPEND) != FMODE_APPEND))
    {
    	/* file is not open for writing. */
    	fput(sfte);
    	dbg(DBG_PRINT, "Exiting do_write() (EBADF).....\n");
        return -EBADF;
    }
    if(nbytes == 0)
    {
    	fput(sfte);
    	dbg(DBG_PRINT, "Exiting do_write().....\n");
    	return 0;
    }
    int nbw = 0;    /*sfte->f_vnode->vn_fs->fs_op->read_vnode(sfte->f_vnode);*/   /* to initialize vnode values */

    if((sfte->f_mode & FMODE_APPEND) == FMODE_APPEND)       /* if write + append */
    {

    	int offset1 = do_lseek(fd, 0, SEEK_END);
    }


    nbw = sfte->f_vnode->vn_ops->write(sfte->f_vnode, sfte->f_pos, buf, nbytes);    /* number of bytes written */
    int offset = do_lseek(fd, nbw, SEEK_CUR);
    /*dbg(DBG_PRINT, "F-pos = %d vn_len = %d\n", sfte->f_pos, sfte->f_vnode->vn_len);*/
    KASSERT((S_ISCHR(sfte->f_vnode->vn_mode)) || (S_ISBLK(sfte->f_vnode->vn_mode)) ||
        		((S_ISREG(sfte->f_vnode->vn_mode)) && (sfte->f_pos <= sfte->f_vnode->vn_len)));
    dbg(DBG_PRINT,"(GRADING2A 3.a) It is either a block device or a character device or regular file and it has a valid f_pos\n");

    dbg(DBG_PRINT, "Exiting do_write().....\n");

    fput(sfte);
    /*NOT_YET_IMPLEMENTED("VFS: do_write");*/
    return nbw;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
	dbg(DBG_PRINT, "Entering do_close().....\n");
	if(fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL)
	{
		dbg(DBG_PRINT, "Exiting do_close().....\n");
		return -EBADF;
	}
	file_t *f = curproc->p_files[fd];
	curproc->p_files[fd] = NULL;
	fput(f);
	dbg(DBG_PRINT, "Exiting do_close()\n");
	return 0;
    /*    NOT_YET_IMPLEMENTED("VFS: do_close");
        return -1;*/
}


/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_dup");*/
	dbg(DBG_PRINT, "Entering do_dup().....\n");
		if(fd < 0 || fd >= NFILES)
		{
			dbg(DBG_PRINT, "Exiting do_dup() (EBADF).....\n");
			return -EBADF;
		}
		file_t *f = fget(fd);
		if(f == NULL)
		{
			dbg(DBG_PRINT, "Exiting do_dup() (EBADF).....\n");
			return -EBADF;
		}
		int fd1 = get_empty_fd(curproc);
		if(fd1 == -EMFILE)
		{
			fput(f);
			dbg(DBG_PRINT, "Exiting do_dup() (EMFILE).....\n");
			return -EMFILE;
		}
		curproc->p_files[fd1] = f;
		dbg(DBG_PRINT, "Exiting do_dup().....\n");
        return fd1;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_dup2");*/
	dbg(DBG_PRINT, "Entering do_dup2().....\n");
		if(nfd < 0 || nfd >= NFILES || ofd < 0 || ofd >= NFILES || curproc->p_files[ofd] == NULL)
		{
			dbg(DBG_PRINT, "Exiting do_dup2() (EBADF).....\n");
			return -EBADF;
		}
		file_t *f = fget(ofd);
		if(nfd == ofd)
		{
			fput(f);
			dbg(DBG_PRINT, "Exiting do_dup2().....\n");
			return nfd;
		}
		if(curproc->p_files[nfd] != NULL) do_close(nfd);
		curproc->p_files[nfd] = f;
		dbg(DBG_PRINT, "Exiting do_dup2().....\n");
        return nfd;
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mknod(const char *path, int mode, unsigned devid)
{
    /*NOT_YET_IMPLEMENTED("VFS: do_mknod");*/
	dbg(DBG_PRINT, "Entering do_mknod().....\n");
	const char *name = NULL;
	size_t len = 0;
	vnode_t *dir = NULL;
	vnode_t *file = NULL;

	/*Incorrect mode........*/
	if(!(S_ISCHR(mode) || S_ISBLK(mode)))
	{
		dbg(DBG_PRINT, "Exiting do_mknod() (EINVAL).....\n");
		return -EINVAL;
	}

	/*Checks every directory component......*/
	int dir_ret = dir_namev(path, &len, &name, NULL, &dir);
	/*dbg(DBG_PRINT, "Return value = %d Ref count of dir = %d\n", dir_ret, dir->vn_refcount);*/
	if(dir_ret < 0)
	{
		dbg(DBG_PRINT, "Exiting do_mknod().....\n");
		return dir_ret;
	}

	int look_ret = lookup(dir, name, len, &file);
	vput(dir);
	if(look_ret < 0 && look_ret != -ENOENT) return look_ret;
	else if(look_ret == 0)
	{
		vput(file);
		dbg(DBG_PRINT, "Exiting do_mknod() (EEXIST).....\n");
		return -EEXIST;
	}

	/*Creating a new node.......*/
	vfs_is_in_use(dir->vn_fs);
	KASSERT(NULL != dir->vn_ops->mknod);
		dbg(DBG_PRINT, "(GRADING2A 3.b) mknod function is not null\n");
	dbg(DBG_PRINT, "Creating a new node %s\n", name);
	int ret = dir->vn_ops->mknod(dir, name, len, mode, (devid_t)devid);
	vfs_is_in_use(dir->vn_fs);
	dbg(DBG_PRINT, "Exiting do_mknod().....\n");
	return ret;
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
    /*NOT_YET_IMPLEMENTED("VFS: do_mkdir");*/
	dbg(DBG_PRINT, "Entering do_mkdir().....\n");
	size_t len = 0;
	const char *name = NULL;
	vnode_t *parent_dir_vnode = NULL;
    vnode_t *new_dir = NULL;

	int dir_ret = dir_namev(path, &len, &name, NULL, &parent_dir_vnode);
	if(dir_ret < 0)
	{
		dbg(DBG_PRINT, "Exiting do_mkdir().....\n");
		return dir_ret;
	}

	int look_ret = lookup(parent_dir_vnode, name , len, &new_dir);
	vput(parent_dir_vnode);
	if(look_ret == 0)
	{
		vput(new_dir);
		dbg(DBG_PRINT, "Exiting do_mkdir() (EEXIST).....\n");
	    return -EEXIST;
	}
	if(look_ret < 0)
	{
		if(look_ret != -ENOENT)
		{
			dbg(DBG_PRINT, "Exiting do_mkdir().....\n");
			return look_ret;
		}
	}
	KASSERT(NULL != parent_dir_vnode->vn_ops->mkdir);
	dbg(DBG_PRINT, "(GRADING2A 3.c) mkdir function is not null\n");
	dbg(DBG_PRINT, "Creating a directory %s\n", name);
	int mkdir_ret = parent_dir_vnode->vn_ops->mkdir(parent_dir_vnode, name, len);
	/*dbg(DBG_PRINT, "Ref count of parent_dir_vnode = %d\n", parent_dir_vnode->vn_refcount);*/
	dbg(DBG_PRINT, " Exiting do_mkdir().....\n");
    return mkdir_ret;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_rmdir(const char *path)
{
    /*NOT_YET_IMPLEMENTED("VFS: do_rmdir");*/
	dbg(DBG_PRINT, "Entering do_rmdir()\n");

    size_t name_len = 0;
    int rmdir_ret = 0;
    const char *name = NULL;
    struct dirent *dir_ent = NULL;
    vnode_t *parent_dir_vnode = NULL;
    vnode_t *dir_tob_rm = NULL;
    int dir_ret = dir_namev(path, &name_len, &name, NULL, &parent_dir_vnode);
    if(dir_ret < 0)
    {
    	dbg(DBG_PRINT, "Exiting do_rmdir().....\n");
    	return dir_ret;
    }
    if(path[strlen(path)-1] == '.')
    {
    	if(path[strlen(path)-2] == '.')
    	{
    		dbg(DBG_PRINT, "Exiting do_rmdir() (ENOTEMPTY).....\n");
    		vput(parent_dir_vnode);
    		return -ENOTEMPTY;
    	}
    	else
    	{
    		dbg(DBG_PRINT, "Exiting do_rmdir() (EINVAL).....\n");
    		vput(parent_dir_vnode);
    		return -EINVAL;
    	}
    }
    /*int look_ret = lookup(parent_dir_vnode, name, name_len, &dir_tob_rm);
    if(look_ret < 0)
    {
    	dbg(DBG_PRINT, " Exiting do_rmdir()4.....\n");
    	return look_ret;
    }
    vput(dir_tob_rm);*/
    /*if(dir_tob_rm->vn_ops->readdir(dir_tob_rm, 0, dir_ent) == 0) Not Sure ..............................*/

    KASSERT(NULL != parent_dir_vnode->vn_ops->rmdir);
    dbg(DBG_PRINT, "(GRADING2A 3.d) Remove Directory operation (rmdir) is valid\n");
    rmdir_ret = parent_dir_vnode->vn_ops->rmdir(parent_dir_vnode, name, name_len);
    vput(parent_dir_vnode);
    dbg(DBG_PRINT, "Exiting do_rmdir()\n");
    return rmdir_ret;
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
	dbg(DBG_PRINT, "Entering do_unlink().....\n");
	size_t namelen = NULL;
	const char *name;
	vnode_t *parent_dir_vnode = NULL;
	vnode_t *file_vnode = NULL;
	int dir_ret = dir_namev(path, &namelen, &name, NULL, &parent_dir_vnode);    /* return type of dir_namev not taken care of */
	if(dir_ret < 0)
	{
		dbg(DBG_PRINT, "Exiting do_unlink().....\n");
		return dir_ret;
	}
	int look_ret = lookup(parent_dir_vnode, name, namelen, &file_vnode);
	if(look_ret < 0)
	{
		dbg(DBG_PRINT, "Exiting do_unlink().....\n");
		vput(parent_dir_vnode);
	    return look_ret;
	}

    if(S_ISDIR(file_vnode->vn_mode))
    {
    	dbg(DBG_PRINT, "Exiting do_unlink().....\n");
    	vput(parent_dir_vnode);
    	vput(file_vnode);
    	return -EISDIR;
    }
    KASSERT(NULL != parent_dir_vnode->vn_ops->unlink);
    	dbg(DBG_PRINT, "(GRADING2A 3.e) unlink function is not null\n");
	int unlink_ret = parent_dir_vnode->vn_ops->unlink(parent_dir_vnode, name, namelen);
	vput(parent_dir_vnode);
	vput(file_vnode);
	dbg(DBG_PRINT, "Exiting do_unlink().....\n");
	return unlink_ret;
    /*NOT_YET_IMPLEMENTED("VFS: do_unlink");*/
}


/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 */
int
do_link(const char *from, const char *to)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_link");*/
	dbg(DBG_PRINT,  "Entering do_link().....\n");
	int flag = 0; /*Not Sure.........................*/
	vnode_t *from_vnode = NULL;
	vnode_t *to_vnode = NULL;
	size_t namelen;
	const char *name;
	vnode_t *to_dir_vnode = NULL;


	int open_ret = open_namev(from, flag, &from_vnode, NULL);
	if(open_ret < 0)
	{
		dbg(DBG_PRINT, "Exiting do_link().....\n");
		return open_ret;
	}

	if(S_ISDIR((from_vnode)->vn_mode))
	{
		dbg(DBG_PRINT, "Exiting from open_namev\n");
		vput(from_vnode);
		return -EISDIR;
	}


	int dir_ret = dir_namev(to, &namelen, &name, NULL, &to_dir_vnode);
	if(dir_ret < 0)
	{
		dbg(DBG_PRINT, "Exiting do_link().....\n");
		vput(from_vnode);
		return dir_ret;
	}
	int look_ret = lookup(to_dir_vnode, name, namelen, &to_vnode);
	if(look_ret >= 0)
	{
		vput(from_vnode);
		vput(to_dir_vnode);
		vput(to_vnode);
		dbg(DBG_PRINT, "Exiting do_link().....\n");
		return -EEXIST;
	}
	else if(look_ret != -ENOENT)
	{
		vput(from_vnode);
		vput(to_dir_vnode);
		return look_ret;
	}

	KASSERT(NULL != to_dir_vnode->vn_ops->link);
	dbg(DBG_PRINT, "Link function exists\n");
	int link_ret = to_dir_vnode->vn_ops->link(from_vnode, to_dir_vnode, name, namelen);

	/*vput(to_vnode);*/
	vput(from_vnode); /*Not Sure.........................................*/
	vput(to_dir_vnode);
	dbg(DBG_PRINT, "Exiting do_link().....\n");
	return link_ret;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
	dbg(DBG_PRINT, " Entering do_rename().....\n");
	int link_ret = do_link(oldname, newname);
	if(link_ret < 0)
	{
		dbg(DBG_PRINT, " Exiting do_rename().....\n");
		return link_ret;
	}
	int result = do_unlink(oldname);
	dbg(DBG_PRINT, " Exiting do_rename().....\n");
	return result;
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_chdir");*/
	dbg(DBG_PRINT, " Entering do_chdir().....\n");
	vnode_t *cur_vnode;
	const char *name;

	int openv_ret = open_namev(path, O_RDONLY, &cur_vnode, NULL);
	if(openv_ret < 0) return openv_ret;
	if(!S_ISDIR(cur_vnode->vn_mode))
	{
		vput(cur_vnode);
		dbg(DBG_PRINT, " Exiting do_chdir()(ENOTDIR).....\n");
		return -ENOTDIR;
	}
	vput(curproc->p_cwd);
	curproc->p_cwd = cur_vnode;
	/*dbg(DBG_PRINT, "Inode no of cur_vnode = %d\n", cur_vnode->vn_vno);*/
	dbg(DBG_PRINT, " Exiting do_chdir().....\n");
	return 0;
}

/* Call the readdir f_op on the given fd, filling in the given dirent_t*.
 * If the readdir f_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)     /* not clear why return value of readdir needs to be checked , also error return kaise karna hain, go thru that */
{
	dbg(DBG_PRINT, " Entering do_getdent().....\n");

	if(fd < 0 || fd >= NFILES) return -EBADF;

	file_t *sfte = fget(fd);
	if(sfte == NULL)
	{
		/* fd is not a valid file descriptor */
		dbg(DBG_PRINT, " Exiting do_getdent() (EBADF).....\n");
		return -EBADF;
    }

	if(!S_ISDIR(sfte->f_vnode->vn_mode))
	{
	    /* fd refers to a directory */
	   	fput(sfte);
	   	dbg(DBG_PRINT, " Exiting do_getdent() (ENOTDIR).....\n");
	    return -ENOTDIR;
	}

	if(sfte->f_vnode->vn_ops->readdir == NULL)
	{
		fput(sfte);
		dbg(DBG_PRINT, " Exiting do_getdent() (ENOTDIR).....\n");
		return -ENOTDIR; /*Not Sure.........................*/
	}

	int output = sfte->f_vnode->vn_ops->readdir(sfte->f_vnode, sfte->f_pos, dirp);
	if(output > 0)       /* call to readdir was successful */
	{
		sfte->f_pos = sfte->f_pos + output;
		fput(sfte);
		dbg(DBG_PRINT, " Exiting do_getdent()....\n");
		return sizeof(*dirp);
	}
	else
	{
		fput(sfte);
		dbg(DBG_PRINT, " Exiting do_getdent().....\n");
		return output;
	}
	/*NOT_YET_IMPLEMENTED("VFS: do_getdent");*/
    return 0;
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_lseek");*/
	dbg(DBG_PRINT, "Entering do_lseek().....\n");

		if(fd < 0 || fd >= NFILES) return -EBADF;

		if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
	    {
	    	dbg(DBG_PRINT, "Entering do_lseek() (EINVAL).....\n");
	        return -EINVAL;
	    }
	    file_t *sfte = fget(fd);

        if(curproc->p_files[fd]==NULL)
        {
        	dbg(DBG_PRINT, " Exiting do_lseek() (EBADF).....\n");
        	return -EBADF;      /* fd is not an open file descriptor */
        }
        if(whence == SEEK_SET)
        {
        	sfte->f_pos = offset;
        }
        if(whence == SEEK_CUR)
        {
        	sfte->f_pos = sfte->f_pos + offset;
        }
        if(whence == SEEK_END)
        {
        	sfte->f_pos = sfte->f_vnode->vn_len + offset;
        }
        /*dbg(DBG_PRINT, " Updated fpos = %d\n", sfte->f_pos);*/
        if(sfte->f_pos < 0)
        {
        	sfte->f_pos = 0;
        	fput(sfte);
        	dbg(DBG_PRINT, "Exiting do_lseek() (EINVAL).....\n");
        	return -EINVAL;
        }
        fput(sfte);
        dbg(DBG_PRINT, "Exiting do_lseek().....\n");
        return sfte->f_pos;
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */

int
do_stat(const char *path, struct stat *buf)
{
        /*NOT_YET_IMPLEMENTED("VFS: do_stat");*/
        size_t len = 0;
        dbg(DBG_PRINT, "Entering do stat()\n");
        if(!strcmp(path, ""))
        {
        	dbg(DBG_PRINT, "Exiting from do_stat() due to EINVAL \n");
        	return -EINVAL;
        }

        const char *name;
        vnode_t *dir_vnode = NULL;
        vnode_t *cur_vnode = NULL;

        int dir_ret = dir_namev(path, &len, &name, NULL, &dir_vnode);
        if(dir_ret < 0)
        {
        	dbg(DBG_PRINT, "Exiting do_stat().....\n");
        	return dir_ret;
        }
        /*dbg(DBG_PRINT, "base name is %s and dir_vnode ka ino is %d\n", name, dir_vnode->vn_vno);*/

        int look_ret = lookup(dir_vnode, name, len, &cur_vnode);
        vput(dir_vnode); /* Check this .....................*/
        if(look_ret < 0)
        {
        	dbg(DBG_PRINT, "Exiting do_stat().....\n");
        	return look_ret;
        }
        else vput(cur_vnode);
        KASSERT(NULL != cur_vnode->vn_ops->stat);
        dbg(DBG_PRINT, "(GRADING2A 3.f) Stat operation is valid\n");
        int stat_ret = cur_vnode->vn_ops->stat(cur_vnode, buf);
        dbg(DBG_PRINT, "Exiting do stat().......\n");
        return stat_ret;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif

