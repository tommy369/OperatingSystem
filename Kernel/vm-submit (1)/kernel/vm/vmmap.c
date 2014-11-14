#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "limits.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
  vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
  KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
  vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
  KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{       /* didn't initialize links of list*/
  vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
  if (newvma) {
    newvma->vma_vmmap = NULL;
  }
  return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
  KASSERT(NULL != vma);
  slab_obj_free(vmarea_allocator, vma);
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{

  vmmap_t* vmap_p = slab_obj_alloc(vmmap_allocator);
  KASSERT(vmap_p && "failed to allocate vmmap obj");
  list_init(&(vmap_p->vmm_list));
  vmap_p->vmm_proc = NULL;

  return vmap_p;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
    /*NOT_YET_IMPLEMENTED("VM: vmmap_destroy");*/
	KASSERT(NULL != map);
	dbg(DBG_PRINT,"(GRADING3A 3.a) vmmap is valid and is not NULL\n");
	int ret = vmmap_remove(map, 0, UINT_MAX);
	if(ret < 0) return; /*Check.....................................*/
	slab_obj_free(vmmap_allocator, map);
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
	void
	vmmap_insert(vmmap_t *map, vmarea_t *newvma)
	{
	        /*NOT_YET_IMPLEMENTED("VM: vmmap_insert");*/
		KASSERT(NULL != map && NULL != newvma);
		dbg(DBG_PRINT,"(GRADING3A 3.b) vmmap and vmarea are not NULL\n");
		KASSERT(NULL == newvma->vma_vmmap);
		dbg(DBG_PRINT,"(GRADING3A 3.b) vmmap under vmaarea  is  NULL\n");
		KASSERT(newvma->vma_start < newvma->vma_end);
		dbg(DBG_PRINT,"(GRADING3A 3.b) start of vmarea is less than end of vmarea\n");
		KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start &&
		ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
		dbg(DBG_PRINT,"(GRADING3A 3.b) USER_MEM_LOW is less than start of vma and USER_MEM_HIGH is greater than end of vma \n");
		vmarea_t *temp;
		if(list_empty(&map->vmm_list) || newvma->vma_end <= (list_head(&map->vmm_list, vmarea_t, vma_plink))->vma_start)
		{
			list_insert_head(&map->vmm_list, &newvma->vma_plink);
			newvma->vma_vmmap = map;
			return;
		}
		list_iterate_begin(&map->vmm_list, temp, vmarea_t, vma_plink)
		{
			if(newvma->vma_end <= temp->vma_start)
			{
				list_insert_before(&temp->vma_plink, &newvma->vma_plink);
				newvma->vma_vmmap = map;
				return;
			}
		}list_iterate_end();
		list_insert_tail(&map->vmm_list, &newvma->vma_plink);
		newvma->vma_vmmap = map;
	}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
	KASSERT(NULL != map);
			dbg(DBG_PRINT,"(GRADING3A 3.c) vmmap is not NULL\n");
	KASSERT(0 < npages);
			dbg(DBG_PRINT,"(GRADING3A 3.c) Number of pages is greater than 0\n");
	vmarea_t *temp;
	uint32_t start = ADDR_TO_PN(USER_MEM_LOW);
	uint32_t end = ADDR_TO_PN(USER_MEM_HIGH);
	if(dir == VMMAP_DIR_LOHI)
	{
		list_iterate_begin(&map->vmm_list, temp, vmarea_t, vma_plink)
		{
			if(temp->vma_start - start > npages)
			{
				return (start);
			}
			else
			{
				start = temp->vma_end;
			}
		}list_iterate_end();
		if(end - start >= npages)
		{
			return (start);
		}
		else return -1;
	}
	else if(dir == VMMAP_DIR_HILO)
	{
		list_iterate_reverse(&map->vmm_list, temp, vmarea_t, vma_plink)
		{
			if(end - temp->vma_end >= npages)
			{
				return end - npages;
			}
			else
			{
				end = temp->vma_start;
			}
		}list_iterate_end();
		if(end - start > npages)
		{
			return end - npages;
		}
		else return -1;
	}
	return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        /*NOT_YET_IMPLEMENTED("VM: vmmap_lookup");*/
		KASSERT(NULL != map);
			dbg(DBG_PRINT,"(GRADING3A 3.d) vmmap is not NULL\n");
		vmarea_t *temp;
		list_iterate_begin(&map->vmm_list, temp, vmarea_t, vma_plink)
		{
			if(vfn >= temp->vma_start && vfn < temp->vma_end)
			{
				return temp;
			}
		}list_iterate_end();
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        /*NOT_YET_IMPLEMENTED("VM: vmmap_clone");*/
		vmmap_t *clone = vmmap_create();
		if(!clone) return NULL;
		vmarea_t *temp;
		list_iterate_begin(&map->vmm_list, temp, vmarea_t, vma_plink)
		{
			vmarea_t *clone_vmarea = vmarea_alloc();
			if(!clone_vmarea) return NULL;
			clone_vmarea->vma_start = temp->vma_start;
			clone_vmarea->vma_end = temp->vma_end;
			clone_vmarea->vma_off = temp->vma_off;
			clone_vmarea->vma_flags = temp->vma_flags;
			clone_vmarea->vma_prot = temp->vma_prot;
			clone_vmarea->vma_obj = NULL;
			list_init(&clone_vmarea->vma_olink);
			list_init(&clone_vmarea->vma_plink);
			vmmap_insert(clone, clone_vmarea);
			clone_vmarea->vma_vmmap = clone;
		}list_iterate_end();
		clone->vmm_proc = map->vmm_proc;
        return clone;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{

				KASSERT(NULL != map);
				dbg(DBG_PRINT,"(GRADING3A 3.f) vmmap is not NULL\n");
				KASSERT(0 < npages);
				dbg(DBG_PRINT,"(GRADING3A 3.f) Number of pages is greater than 0\n");
				KASSERT(!(~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC) & prot));
				dbg(DBG_PRINT,"(GRADING3A 3.f\n)Access protection is valid ");
				KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
				dbg(DBG_PRINT,"(GRADING3A 3.f) Mapping flag is set (either MAP_SHARED or MAP_PRIVATE )\n");
				KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
				dbg(DBG_PRINT,"(GRADING3A 3.f) Low Page value is zero OR USER_MEM_LOW is less than or equal ro low page value \n");
		        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
				dbg(DBG_PRINT,"(GRADING3A 3.f) Low Page is zero OR USER_MEM_HIGH value is greater than value of (low page + number of pages) i.e. high page value\n");
				KASSERT(PAGE_ALIGNED(off));
				dbg(DBG_PRINT,"(GRADING3A 3.f) PAGE_ALIGNED(off) exists\n");

			vmarea_t *temp = vmarea_alloc();
			int start = 0;
			if(lopage == 0)
			{
				start = vmmap_find_range(map, npages, dir);
				if(start < 0)
				{
					return ENOMEM;
				}
			}
			else
			{
				int ret = vmmap_is_range_empty(map, lopage, npages);

				if(ret == 0)
				{
					vmmap_remove(map, lopage, npages);
				}
				start = lopage;
			}
			temp->vma_start = start;
			temp->vma_end = temp->vma_start + npages;
			temp->vma_off = off;
			temp->vma_flags = flags;
			temp->vma_prot = prot;

			list_link_init(&temp->vma_plink);
			list_link_init(&temp->vma_olink);
			vmmap_insert(map, temp);

			if(!file)
			{
				if((flags & MAP_PRIVATE) == MAP_PRIVATE)
				{
					temp->vma_obj = shadow_create();
					temp->vma_obj->mmo_un.mmo_bottom_obj = anon_create();
					temp->vma_obj->mmo_shadowed = temp->vma_obj->mmo_un.mmo_bottom_obj;
					list_insert_tail(&temp->vma_obj->mmo_shadowed->mmo_un.mmo_vmas, &temp->vma_olink);

				}
				else
				{
					temp->vma_obj = anon_create();
					list_insert_tail(&temp->vma_obj->mmo_un.mmo_vmas, &temp->vma_olink);
				}
			}
			else
			{
				if((flags & MAP_PRIVATE) == MAP_PRIVATE)
				{
					temp->vma_obj = shadow_create();
					file->vn_ops->mmap(file, temp, &temp->vma_obj->mmo_shadowed);
					temp->vma_obj->mmo_un.mmo_bottom_obj = temp->vma_obj->mmo_shadowed;
					list_insert_tail(&temp->vma_obj->mmo_shadowed->mmo_un.mmo_vmas, &temp->vma_olink);
				}
				else
				{
					file->vn_ops->mmap(file, temp, &temp->vma_obj);
					list_insert_tail(&temp->vma_obj->mmo_un.mmo_vmas, &temp->vma_olink);
				}
			}
			if(new) *new = temp;

	        return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
  /*NOT_YET_IMPLEMENTED("VM: vmmap_remove");*/

			vmarea_t *temp;
			uint32_t hipage = lopage + npages;
			list_iterate_begin(&map->vmm_list, temp, vmarea_t, vma_plink)
			{
				/*Completely outside the region*/
				if((lopage < temp->vma_start && hipage <= temp->vma_start) || lopage >= temp->vma_end) continue;
				/*Case 1:  [   ******    ]*/
				else if(lopage > temp->vma_start && hipage < temp->vma_end)
				{
					pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(hipage + 1));
					vmarea_t *new = vmarea_alloc();
					new->vma_start = hipage;
					new->vma_end = temp->vma_end;
					new->vma_off = hipage - temp->vma_start + temp->vma_off;
					new->vma_flags = temp->vma_flags;
					new->vma_prot = temp->vma_prot;
					new->vma_obj = temp->vma_obj;
					list_link_init(&new->vma_olink);
					list_link_init(&new->vma_plink);
					vmmap_insert(map, new);

					list_insert_tail(mmobj_bottom_vmas(temp->vma_obj), &new->vma_olink);
					temp->vma_obj->mmo_ops->ref(temp->vma_obj);
					/*Previous vmarea*/
					temp->vma_end = lopage;
				}
				/*Case 2:  [      *******]** */
				else if(lopage > temp->vma_start && lopage < temp->vma_end && hipage >= temp->vma_end)
				{
					pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(temp->vma_end+ 1));
					temp->vma_end = lopage;
					temp->vma_off = temp->vma_off + hipage - temp->vma_start;

				}
				/*Case 3 : *[*****        ] */
				else if(lopage <= temp->vma_start && hipage < temp->vma_end && hipage > temp->vma_start)
				{
					 pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(temp->vma_start), (uintptr_t)PN_TO_ADDR(hipage + 1));
					 temp->vma_start = hipage;
					 temp->vma_off = temp->vma_off + hipage - temp->vma_start;
				}
				/* Case 4: *[*************]** */
				else if(lopage <= temp->vma_start && hipage >= temp->vma_end)
				{
					pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(temp->vma_start), (uintptr_t)PN_TO_ADDR(temp->vma_end + 1));
					list_remove(&temp->vma_plink);
					list_remove(&temp->vma_olink);
					temp->vma_obj->mmo_ops->put(temp->vma_obj);
					vmarea_free(temp);
				}

			}list_iterate_end();
	        return 0;

}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
	       /*NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");*/
			uint32_t end = startvfn + npages;
			KASSERT((startvfn < end) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= end));
	 	 	 dbg(DBG_PRINT,"(GRADING3A 3.e) start of vfn is less than end of vfn , USER_MEM_LOW is less than start of vma and USER_MEM_HIGH is greater than end of vma. \n");

			vmarea_t *temp;
			list_iterate_begin(&map->vmm_list, temp, vmarea_t, vma_plink)
			{
				if(startvfn >= temp->vma_start && startvfn < temp->vma_end) return 0;
				if(end >= temp->vma_start && end <= temp->vma_end) return 0;
			}list_iterate_end();
	        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
  dbg(DBG_CORE,"Started execution of vmmap_read function to read from %x address for size =%u\n",(uint32_t)vaddr,count);
  vmarea_t* vma;
  uint32_t start_vfn = ADDR_TO_PN(vaddr);
  uint32_t last_vfn = ADDR_TO_PN((uint32_t)vaddr + count)+1;
  int byt = 0;

  void* addr = (void*)vaddr;
  list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
    {
      if(start_vfn >= vma->vma_end)
	continue;
      else if(last_vfn <= vma->vma_start)
	{
	  break;
	}
      /*NEED TO ADD A CHECK TO MAKE SURE THERE ARE NO BREAKS IN BETWEEN vaddr and vaddr+count*/
      else
	{

	  uint32_t temp_start;
	  uint32_t temp_end;
	  if(start_vfn < vma->vma_start)
	    temp_start = vma->vma_start;
	  else
	    temp_start = start_vfn;

	  if(last_vfn >= vma->vma_end)
	    temp_end = vma->vma_end;
	  else
	    temp_end = last_vfn;

	  /*Comes here only after validation of mappings so good to go*/
	  pframe_t* pf;
	  uint32_t i;
	  for(i= temp_start; i < temp_end;i++)
	    {

	      pframe_get(vma->vma_obj, vma->vma_off + i - vma->vma_start , &pf);
	      dbg(DBG_CORE,"going to read page %d\n",pf->pf_pagenum);
	      uint32_t off = ((uint32_t)addr)%PAGE_SIZE;
	      addr = PN_TO_ADDR(i+1);

	      buf = (void *)((uint32_t)buf + byt);
	      memcpy( buf, (void*)((uint32_t)(pf->pf_addr)+off), MIN(count-byt, PAGE_SIZE-off));
	      byt += MIN(count-byt, PAGE_SIZE-off);
	    }

	}
    }list_iterate_end();
  return 0;

}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
  vmarea_t* vma;
  uint32_t start_vfn = ADDR_TO_PN(vaddr);
  uint32_t last_vfn = ADDR_TO_PN((uint32_t)vaddr + count)+1;
  dbg(DBG_CORE,"Write from address %x (page = %d) of size =%u\n",(uint32_t)vaddr,start_vfn,count);
  int byt = 0;
  /*		uint32_t check;*/

  void* addr = vaddr;
  list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
    {
      if(start_vfn >= vma->vma_end)
	continue;
      else if(last_vfn <= vma->vma_start)
	break;
      /*NEED TO ADD A CHECK TO MAKE SURE THERE ARE NO BREAKS IN BETWEEN vaddr and vaddr+count*/
      /*-----------------------------------------------------------------------------------*/
      else
	{
	  uint32_t temp_start;
	  uint32_t temp_end;

	  if(start_vfn < vma->vma_start)
	    temp_start = vma->vma_start;
	  else
	    {
	      temp_start = start_vfn;
	    }

	  if(last_vfn >= vma->vma_end)
	    temp_end = vma->vma_end;
	  else
	    temp_end = last_vfn;

	  /*Comes here only after validation of mappings so good to go*/
	  pframe_t* pf;
	  uint32_t i;
	  for(i= temp_start; i < temp_end;i++)
	    {

	      pframe_get(vma->vma_obj, vma->vma_off + i - vma->vma_start , &pf);
	      dbg(DBG_CORE,"Going to write in page %d\n",pf->pf_pagenum);
	      uint32_t off = ((uint32_t)addr)%PAGE_SIZE;
	      addr = PN_TO_ADDR(i+1);

	      buf = (void*) ((uint32_t)buf + byt);
	      pframe_dirty(pf);
	      pframe_set_busy(pf);
	      /*memcpy(pf->pf_addr, buf, MIN(count-byt, PAGE_SIZE-off));*/
	      memcpy( (void*)((uint32_t)(pf->pf_addr)+off),buf, MIN(count-byt, PAGE_SIZE-off));
	      pframe_clear_busy(pf);
	      sched_broadcast_on(&pf->pf_waitq);
	      byt += MIN(count-byt, PAGE_SIZE-off);
	    }

	}
    }list_iterate_end();
  return 0;
}


/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
  KASSERT(0 < osize);
  KASSERT(NULL != buf);
  KASSERT(NULL != vmmap);

  vmmap_t *map = (vmmap_t *)vmmap;
  vmarea_t *vma;
  ssize_t size = (ssize_t)osize;

  int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
		     "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
		     "VFN RANGE");
  /*dbg(DBG_CORE,"%21s %5s %7s %8s %10s %12s\n",
    "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
    "VFN RANGE");*/

  list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
    size -= len;
    buf += len;
    if (0 >= size) {
      goto end;
    }

    len = snprintf(buf, size,
		   "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
		   vma->vma_start << PAGE_SHIFT,
		   vma->vma_end << PAGE_SHIFT,
		   (vma->vma_prot & PROT_READ ? 'r' : '-'),
		   (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
		   (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
		   (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
		   vma->vma_obj/*,vma->vma_obj->mmo_un.mmo_bottom_obj*/, vma->vma_off, vma->vma_start, vma->vma_end);
				
    /*dbg(DBG_CORE,"%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
      vma->vma_start << PAGE_SHIFT,
      vma->vma_end << PAGE_SHIFT,
      (vma->vma_prot & PROT_READ ? 'r' : '-'),
      (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
      (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
      (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
      vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);*/
  } list_iterate_end();

 end:
  if (size <= 0) {
    size = osize;
    buf[osize - 1] = '\0';
  }
  /*
    KASSERT(0 <= size);
    if (0 == size) {
    size++;
    buf--;
    buf[0] = '\0';
    }
  */
  return osize - size;
}
