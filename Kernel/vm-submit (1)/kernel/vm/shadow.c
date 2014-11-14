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

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
 * number of shadow objects with a single parent, that is another shadow
 * object in the shadow objects tree(singletons)
 */
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int  shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int  shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int  shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = {
  .ref = shadow_ref,
  .put = shadow_put,
  .lookuppage = shadow_lookuppage,
  .fillpage  = shadow_fillpage,
  .dirtypage = shadow_dirtypage,
  .cleanpage = shadow_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * shadow page sub system. Currently it only initializes the
 * shadow_allocator object.
 */
void
shadow_init()
{

  shadow_allocator = slab_allocator_create("Shadow Object",sizeof(mmobj_t));
  KASSERT(shadow_allocator);
  		dbg(DBG_PRINT,"(GRADING3A 6.a)  slab allocation for shadow_allocator is done successfully\n");
}

/*
 * You'll want to use the shadow_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
shadow_create()
{
  mmobj_t *s = slab_obj_alloc(shadow_allocator);
  mmobj_init(s, &shadow_mmobj_ops);
  s->mmo_refcount = 1;

  return s;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
shadow_ref(mmobj_t *o)
{
  KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
  		dbg(DBG_PRINT,"(GRADING3A 6.b) mmobj 'o' is not NULL and refcount of mmobj 'o 'is  greater than zero and mmo_ops of mmobj 'o' are same as that of shadow object\n");
  o->mmo_refcount++;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is a shadow object, it will never
 * be used again. You should unpin and uncache all of the object's
 * pages and then free the object itself.
 */
static void
shadow_put(mmobj_t *o)
{

	KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
			dbg(DBG_PRINT,"(GRADING3A 6.c) mmobj 'o' is not NULL and refcount of mmobj 'o' is greater than zero and mmo_ops of mmobj are same as that of shadow object\n");
  o->mmo_refcount--;
	if(o->mmo_refcount == 0)
	{
		o->mmo_shadowed->mmo_ops->put(o->mmo_shadowed);
		slab_obj_free(shadow_allocator, o);
		return;
	}

  if(o->mmo_refcount == o->mmo_nrespages)
    {
      pframe_t* pf;
      list_t* temp = o->mmo_respages.l_next;
      while(temp != &o->mmo_respages)
      {
		  pf = list_item(temp, pframe_t, pf_olink);
		  pframe_unpin(pf);


		  while (pframe_is_busy(pf))
			sched_sleep_on(&(pf->pf_waitq));

		  pframe_free(pf);


		  temp = o->mmo_respages.l_next;
      }
    }


}

/* This function looks up the given page in this shadow object. The
 * forwrite argument is true if the page is being looked up for
 * writing, false if it is being looked up for reading. This function
 * must handle all do-not-copy-on-not-write magic (i.e. when forwrite
 * is false find the first shadow object in the chain which has the
 * given page resident). copy-on-write magic (necessary when forwrite
 * is true) is handled in shadow_fillpage, not here. */
static int
shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
  pframe_t *t;
  mmobj_t *temp = o;
  pframe_t *source;
  uint32_t f = 0;
  while(temp->mmo_shadowed)
  {
      list_iterate_begin(&temp->mmo_respages, t, pframe_t, pf_olink)
	  {
    	  if(t->pf_pagenum == pagenum)
    	  {
			source = t;
			/*f = 1;*/
			if(forwrite)
			{
				if(source->pf_obj == o)
				{
					*pf = source;
					return 0;
				}
				else
				{
					pframe_get(o, pagenum, pf);
					return 0;
				}
			}
			else
			{
				*pf = source;
				return 0;
			}
    	  }
      } list_iterate_end();

      /*if(f) break;*/
      temp = temp->mmo_shadowed;
  }

  /*If not found in shadow chain, check in bottom object*/
  /*if(!f)
  {*/
      pframe_get(o->mmo_un.mmo_bottom_obj, pagenum, &source);
  /*}*/

  	if(forwrite)
    {
      	if(source->pf_obj == o)
		{
		  *pf = source;
		  return 0;
		}
        else
        {
		  pframe_get(o, pagenum, pf);
		  return 0;
        }
    }
  	else
    {
      *pf = source;
      return 0;
    }
}

/* As per the specification in mmobj.h, fill the page frame starting
 * at address pf->pf_addr with the contents of the page identified by
 * pf->pf_obj and pf->pf_pagenum. This function handles all
 * copy-on-write magic (i.e. if there is a shadow object which has
 * data for the pf->pf_pagenum-th page then we should take that data,
 * if no such shadow object exists we need to follow the chain of
 * shadow objects all the way to the bottom object and take the data
 * for the pf->pf_pagenum-th page from the last object in the chain). */
static int
shadow_fillpage(mmobj_t *o, pframe_t *pf)
{
  KASSERT(pframe_is_busy(pf));
  dbg(DBG_PRINT,"(GRADING3A 6.d) pframe is busy\n");
  KASSERT(!pframe_is_pinned(pf));
  dbg(DBG_PRINT,"(GRADING3A 6.d) pframe is not pinned\n");
  pframe_pin(pf);

  mmobj_t *temp = o->mmo_shadowed;
  int f = 0;
  pframe_t* tpf;
  pframe_t* new_pf;

   while(temp->mmo_shadowed)
   {
      list_iterate_begin(&temp->mmo_respages, tpf, pframe_t, pf_olink)
      {
		if(tpf->pf_pagenum == pf->pf_pagenum)
		  {
			new_pf = tpf;
			memcpy(pf->pf_addr, new_pf->pf_addr, PAGE_SIZE);
			return 0;
		  }
      }list_iterate_end();
      temp = temp->mmo_shadowed;
   }

  pframe_get(temp, pf->pf_pagenum, &new_pf);
  memcpy(pf->pf_addr, new_pf->pf_addr, PAGE_SIZE);
  return 0;



}

/* These next two functions are not difficult. */

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{


  return 0;
}

static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
  return 0;
}
