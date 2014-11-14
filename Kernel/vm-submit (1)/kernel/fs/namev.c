#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

#include "mm/kmalloc.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{

		/*vfs_is_in_use(dir->vn_fs);*/
		char *current = ".";
		dbg(DBG_PRINT, "Entering lookup function....\n");
		if(name == NULL)
		{
			*result = dir;
			vref(*result);
			return 0;
		}
	    KASSERT(NULL != dir);
		dbg(DBG_PRINT, "dir is not null \n");
	    KASSERT(NULL != name);
		dbg(DBG_PRINT, "Name is not null \n");
		KASSERT(NULL != result);
		dbg(DBG_PRINT, "result vnode is not null \n");

		if(!strcmp(current, name))
		{
			*result = dir;
			/*vget((*result)->vn_fs, (*result)->vn_vno);*/
			vref (*result);
			return 0;
		}

		if(!S_ISDIR(dir->vn_mode))
		{
			dbg(DBG_PRINT, "Exiting lookup(ENOTDIR)....\n");
			return -ENOTDIR;
		}
		if(len > NAME_LEN)
		{
			dbg(DBG_PRINT, "Exiting lookup(ENAMETOOLONG)....\n");
			return -ENAMETOOLONG;
		}
		if(dir->vn_ops->lookup == NULL)
		{
			dbg(DBG_PRINT, "Exiting lookup(NULL)....\n");
			return -ENOTDIR;
		}
		dbg(DBG_PRINT, "Searching for %s in %p\n", name, dir);
		int ret = dir->vn_ops->lookup(dir, name, len, result);
		if(ret < 0) return ret;
		/*vref(*result);*/
        return 0;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
	/* NOT_YET_IMPLEMENTED("VFS: dir_namev"); */
		/*KASSERT(NULL != pathname);
		dbg(DBG_PRINT, "(GRADING2A 2.b) Pre-condition: The pathname to look up is not NULL.\n");

		KASSERT(NULL != namelen);
		dbg(DBG_PRINT, "(GRADING2A 2.b) Pre-condition: The address for holding the length of the name is not NULL.\n");

		KASSERT(NULL != name);
		dbg(DBG_PRINT, "(GRADING2A 2.b) Pre-condition: The address for holding the name is not NULL.\n");

		KASSERT(NULL != res_vnode);
		dbg(DBG_PRINT, "(GRADING2A 2.b) Pre-condition: The address for holding the vnode of the parent directory is not NULL.\n");

		vnode_t* curr_base = NULL;
		vnode_t* next_base = NULL;
		char next_dir[NAME_LEN];
		const char *dir_start = NULL, *dir_end = NULL;
		size_t len = 0;
		int ret = 0;

		*namelen = 0;

		dir_start = pathname;
		if (pathname == NULL || namelen == NULL || name == NULL|| res_vnode == NULL || strnlen(pathname, MAXPATHLEN) == 0)
			return -EINVAL;

		if (pathname[0] == '/')
		{
			dbg(DBG_PRINT, "(GRADING2C Additional Checks): Path starts with / (root directory). Tested by vfstest_paths() as part of VFS tests.\n");
			curr_base = vfs_root_vn;
			++dir_start;
		}
		else if (base == NULL)
			curr_base = curproc->p_cwd;
		else
			curr_base = base;

		dir_end = strchr(dir_start, '/');

		vref(curr_base);

		while (dir_end != NULL)
		{
			len = MIN(dir_end - dir_start, NAME_LEN - 1);
			if (len >= NAME_LEN - 1)
			{
				dbg(DBG_PRINT, "(GRADING2C Additional Checks): File name of a component in the path is too long. Tested in vfs_additional_test() as part of the additional VFS Tests.\n");
				*namelen = 0;
				*name = NULL;
				*res_vnode = NULL;
				vput(curr_base);
				return -ENAMETOOLONG;
			}

			if (len == 0 && *dir_start == '/')
			{
				dbg(DBG_PRINT, "(GRADING2C Additional Checks): Additional '/' in file name. Tested in vfstest_paths() as part of VFS tests.\n");
				dir_start++;
				dir_end = strchr(dir_start, '/');
				continue;
			}
			strncpy(next_dir, dir_start, len);
			next_dir[len] = '\0';


			if ((ret = lookup(curr_base, next_dir, len, &next_base)) != 0)
			{
				dbg(DBG_PRINT, "(GRADING2C Additional Checks): Cannot find a component in the given path. Tested in vfstest_mkdir() as part of VFS tests.\n");
				*namelen = 0;
				*name = NULL;
				*res_vnode = NULL;
				vput (curr_base);
				return -ENOENT;
			}

			vput (curr_base);

			KASSERT(NULL != next_base);
			dbg(DBG_PRINT, "(GRADING2A 2.b) After looking up the next component in the path: The vnode for the component: %s is not NULL.\n", next_dir);

			curr_base = next_base;



			if (!(S_ISDIR(curr_base->vn_mode)))
			{
				dbg(DBG_PRINT, "(GRADING2C Additional Checks): The component: %s in the given path is not a directory. Tested in vfstest_mkdir() as part of VFS tests.\n", next_dir);
				*namelen = 0;
				*name = NULL;
				*res_vnode = NULL;
				vput(curr_base);
				return -ENOTDIR;
			}

			dir_start = dir_end + 1;
			dir_end = strchr(dir_start, '/');
		}

		len = strnlen(dir_start, NAME_LEN - 1);
		if (len >= NAME_LEN - 1)
		{
			dbg(DBG_PRINT, "(GRADING2C Additional Checks): File name of a component in the path is too long. Tested in vfstest_mkdir() as part of VFS tests.\n");
			*namelen = 0;
			*name = NULL;
			*res_vnode = NULL;
			vput(curr_base);
			return -ENAMETOOLONG;
		}

		*res_vnode = curr_base;
		*name = dir_start;
		*namelen = len;*/
	dbg(DBG_PRINT, "Entering dir_namev() function...\n");
				KASSERT(NULL != pathname);
				dbg(DBG_PRINT, "(GRADING2A 2.b) Pathname is not null... \n");

			       /*NOT_YET_IMPLEMENTED("VFS: dir_namev");*/

				if(!strcmp(pathname, ""))
				{
					dbg(DBG_PRINT, " Exiting dir_namev().....\n");
					return -ENOENT;
				}

				vnode_t *temp = NULL;
				int index = 0;

				/*Setting up the base*/
				if(pathname[0] == '/')
				{
					temp = vfs_root_vn;
				}
				else if(base == NULL) temp = curproc->p_cwd;
				else temp = base;

				vref(temp);

				int count = 0;
				int length = strlen(pathname);
				if(length > MAXPATHLEN) return -ENAMETOOLONG;

				while(pathname[index] == '/')
				{
					index++;
					if(index == length)
					{
						*namelen = 0;
						*name = NULL;
						*res_vnode = temp;
						/*vget((*res_vnode)->vn_fs, (*res_vnode)->vn_vno);*/
						/*vref(*res_vnode);*/
						return 0;
					}
				}			/*dbg(DBG_PRINT, "Index = %d Length = %d %c/n", index, length, pathname[index]);*/


				if(pathname[index] == '\0')
				{
					*namelen = 0;
					*name = NULL;
					*res_vnode = temp;
					/*vget((*res_vnode)->vn_fs, (*res_vnode)->vn_vno);*/
					/*vref(*res_vnode);*/
					return 0;
				}

				/*dbg(DBG_PRINT, "Entering dir_namev3....\n");*/

				char *pathname1 = (char *)kmalloc(strlen(pathname) + 1);
				strcpy(pathname1, &pathname[index]);

				/*dbg(DBG_PRINT, "Dir_namev Pathname1 = %s....\n", pathname1);*/
				int l = strlen(pathname1);
				int j = 0;
				while(j < l)
				{
					if(pathname1[j] == '/')
					{
						if(pathname1[j + 1 ] != '\0' && pathname1[j + 1] == '/')
						{
							j++;
							continue;
						}
						count++;
					}
					j++;
				}

				char *cname = NULL;
				vnode_t *vname = NULL;
				index = 0;
				vnode_t *cur_vnode;
				size_t len;
				int look_ret;
				int k = length;

				while(pathname[k - 1] == '/')
				{
					k--;
				}
				if(k != length)
				{
					count--;
				}
			    pathname1[strlen(pathname1) - length + k] = '\0';

			    dbg(DBG_PRINT, "Pathname = %s\n", pathname1);

				if(count == 0)
				{
					*res_vnode = temp;
					/*vget((*res_vnode)->vn_fs, (*res_vnode)->vn_vno);*/
					/*vref(*res_vnode);*/
					/*dbg(DBG_PRINT, "Inode no of res_vnode = %d\n", (*res_vnode)->vn_vno);*/
					*name = pathname1;
					/*dbg(DBG_PRINT, "File Name = %s\n", *name);*/
					*namelen = strlen(pathname1);
					KASSERT(NULL != namelen);
					dbg(DBG_PRINT, "(GRADING2A 2.b) Length of name is not null... \n");
					KASSERT(NULL != name);
					dbg(DBG_PRINT, "(GRADING2A 2.b) Name is not null... \n");
					KASSERT(NULL != *res_vnode);
					dbg(DBG_PRINT, "(GRADING2A 2.b) res_vnode (vnode of parent directory of name) is not null... \n");
					dbg(DBG_PRINT, "Exiting dir_namev()........\n");
					return 0;
				}

				dbg(DBG_PRINT, "Dir_namev function....Pathname = %s count = %d\n", pathname1, count);
				/*dbg(DBG_PRINT, "Entering dir_namev4....\n");*/

				int i = 0;
				cname = strtok((char *)pathname1, "/");

				for(i = 0; i < count; i++)
				{
					/*dbg(DBG_PRINT, "cname = %s\n", cname);*/

					look_ret = lookup(temp, cname, (size_t) strlen(cname), &vname);
					if(look_ret < 0)
					{
						*namelen = 0;
						*name = NULL;
						*res_vnode = NULL;
						vput(temp);
						return look_ret;
					}
					/*dbg(DBG_PRINT, "Ref count of temp = %d\n",(int)vname->vn_refcount);*/

					dbg(DBG_PRINT, "Ref count of temp = %d\n",(int)temp->vn_refcount);
					vput(temp);
					cname = strtok(NULL, "/");
					temp = vname;

					if(!S_ISDIR(temp->vn_mode))
					{
						dbg(DBG_PRINT, "Exiting dir_namev (ENOTDIR)....\n");
						vput(temp);
						return -ENOTDIR;
					}
				}

				*namelen = (strlen(cname));
				*name = cname;
				*res_vnode = temp;

				/*dbg(DBG_PRINT, "Inode no of res_vnode = %d\n", (*res_vnode)->vn_vno);*/
				KASSERT(NULL != namelen);
				dbg(DBG_PRINT, "(GRADING2A 2.b) Length of name is not null... \n");
				KASSERT(NULL != name);
				dbg(DBG_PRINT, "(GRADING2A 2.b) Name is not null... \n");
				KASSERT(NULL != *res_vnode);
				dbg(DBG_PRINT, "(GRADING2A 2.b) res_vnode (vnode of parent directory of name) is not null... \n");
				/*vref(*res_vnode);*/

				dbg(DBG_PRINT, "Exiting dir_namev....\n");
				return 0;


	        return 0;
		}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fnctl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
	dbg(DBG_PRINT, "Entering open_namev()....\n");
	size_t namelen = 0;
	const char *name = NULL;
	vnode_t *res_vnode_fordirnamev = NULL;
	int dir_ret = dir_namev(pathname, &namelen, &name, NULL, &res_vnode_fordirnamev);
	if(dir_ret < 0)
	{
		dbg(DBG_PRINT, "Exiting open_namev()....\n");
		return dir_ret;
	}

	int look_ret = lookup(res_vnode_fordirnamev, name, namelen, res_vnode);
	/*vput(res_vnode_fordirnamev);*/
	if(look_ret < 0)
	{
		if(look_ret == -ENOENT && (flag & O_CREAT) == O_CREAT)
		{
			KASSERT(NULL != res_vnode_fordirnamev->vn_ops->create);
			dbg(DBG_PRINT, "(GRADING2A 2.c) File system specific function create() exists and is valid.... \n");
			int create_ret = res_vnode_fordirnamev->vn_ops->create(res_vnode_fordirnamev, name, namelen, res_vnode);
			vput(res_vnode_fordirnamev);
			dbg(DBG_PRINT, "Exiting open_namev()...\n");
		    return create_ret;
		}
		vput(res_vnode_fordirnamev);
		dbg(DBG_PRINT, "Exiting open_namev()....\n");
		return look_ret;
	}
	vput(res_vnode_fordirnamev);
	/*size_t namelen = 0;
		const char *name = NULL;
		vnode_t *res_vnode_fordirnamev = NULL;
		dir_namev(pathname, &namelen, &name, base, &res_vnode_fordirnamev);
		lookup(res_vnode_fordirnamev, name, namelen, res_vnode);
		if((flag & O_CREAT) == O_CREAT && *res_vnode == NULL)
		{
	        res_vnode_fordirnamev->vn_ops->create(res_vnode_fordirnamev, name, namelen, res_vnode);
		}
	        NOT_YET_IMPLEMENTED("VFS: open_namev");*/


	/*if(S_ISDIR((*res_vnode)->vn_mode))
	{
		dbg(DBG_PRINT, "Exiting from open_namev\n");
		return -EISDIR;
	}*/

	dbg(DBG_PRINT, "Exiting open_namev()...\n");
        /*NOT_YET_IMPLEMENTED("VFS: open_namev");*/
    return 0;
}


#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
