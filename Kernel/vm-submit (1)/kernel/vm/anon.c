#include "globals.h"
#include "errno.h"

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage  = anon_fillpage,
        .dirtypage = anon_dirtypage,
        .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void
anon_init()
{
	/*NOT_YET_IMPLEMENTED("VM: anon_init");*/
	/*mmobj_t *new_mmobjj = (mmobj_t *) slab_obj_alloc(anon_allocator);*/
	anon_allocator = slab_allocator_create("anon",sizeof(mmobj_t));
	dbg(DBG_PRINT,"anon_allocator object initialized");
	KASSERT(anon_allocator);
	dbg(DBG_PRINT,"(GRADING3A 4.a) slab allocation for anon_allocator is done successfully\n");
	return;
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
anon_create()
{
        /*NOT_YET_IMPLEMENTED("VM: anon_create");*/
	mmobj_t *new_mmobj = (mmobj_t *) slab_obj_alloc(anon_allocator);
	if(new_mmobj==NULL)
		return NULL;
	mmobj_init(new_mmobj, &anon_mmobj_ops);
	new_mmobj->mmo_refcount = 1;
	return new_mmobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_ref");*/
	KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
			dbg(DBG_PRINT,"(GRADING3A 4.b) mmobj o is not NULL and refcount of the memobj is greater than zero and mmo_ops of the memobj  o are same as anonymous  mmobj_ops \n");
	o->mmo_refcount ++;
	return;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
	KASSERT(o  && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT,"(GRADING3A 4.c) mmobj o is not NULL and refcount of the memobj is greater than zero and mmo_ops of the memobj  o are same as anonymous  mmobj_ops  \n");

  static uint32_t count = 0;

  o->mmo_refcount--;

  if(o->mmo_refcount == o->mmo_nrespages)
  {
      pframe_t* pf;
      list_t* temp = o->mmo_respages.l_next;

		while(temp != &o->mmo_respages)
		{
		  count++;
		  pf = list_item(temp, pframe_t, pf_olink);
		  while(pf->pf_pincount > 0) pframe_unpin(pf);
		  while (pframe_is_busy(pf))
			sched_sleep_on(&(pf->pf_waitq));

		  pframe_free(pf);
		  count--;
		  temp = o->mmo_respages.l_next;
		}

		if(!count) slab_obj_free( anon_allocator, o);
    }
}
/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_lookuppage");*/
		/*KASSERT(NULL!=o);
		KASSERT(NULL!=pf);
		return o->mmo_ops->lookuppage(o,pagenum,forwrite,pf);*/
		return pframe_get(o, pagenum, pf);
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_fillpage");*/
		/*int ret;
		pframe_set_busy(pf);
	    ret = o->mmo_ops->fillpage(o,pf);
	    pframe_clear_busy(pf);
	    sched_broadcast_on(&pf->pf_waitq);
	    return ret;*/
		KASSERT(pframe_is_busy(pf));
			dbg(DBG_PRINT,"(GRADING3A 4.d) pframe pf is busy\n");
		KASSERT(!pframe_is_pinned(pf));
			dbg(DBG_PRINT,"(GRADING3A 4.d) pframe pf is not pinned\n");
		memset(pf->pf_addr, 0, PAGE_SIZE);
		return 0;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_dirtypage");*/
	    /*pframe_clear_busy(pf);
	    sched_broadcast_on(&pf->pf_waitq);*/
	    return 0;
        /*return -1;*/
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_cleanpage");*/
		/*int ret;
	    KASSERT(pframe_is_dirty(pf) && "Cleaning page that isn't dirty!");
	    KASSERT(pf->pf_pincount == 0 && "Cleaning a pinned page!");
	        dbg(DBG_PFRAME, "cleaning page %d of obj %p\n", pf->pf_pagenum, o);*/
	        /*
	         * Clear the dirty bit *before* we potentially (depending on this
	         * particular object type's 'dirtypage' implementation) block so
	         * that if the page is dirtied again while we're writing it out,
	         * we won't (incorrectly) think the page has been fully cleaned.
	         */
	        /*pframe_clear_dirty(pf);*/
	        /* Make sure a future write to the page will fault (and hence dirty it) */
	        /*tlb_flush((uintptr_t) pf->pf_addr);
	        pframe_remove_from_pts(pf);
	        pframe_set_busy(pf);
	        if ((ret = o->mmo_ops->cleanpage(o, pf)) < 0) {
	                pframe_set_dirty(pf);
	        }
	        pframe_clear_busy(pf);
	        sched_broadcast_on(&pf->pf_waitq);*/
	    return 0;
}
