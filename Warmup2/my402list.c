#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "cs402.h"
#include "my402list.h"
My402ListElem *My402ListLast (My402List *new_list)
{      
	if((new_list->anchor.prev==NULL) || (new_list->anchor.next==NULL))
		return NULL;
	else  
        return new_list->anchor.prev;
}
My402ListElem *My402ListFirst (My402List *new_list)
{
	My402ListElem *temp1;
	temp1=new_list->anchor.next;        
    return new_list->anchor.next;	
}
int My402ListInit(My402List *new_list)
{
	new_list->anchor.obj= NULL;
	new_list->anchor.next = NULL;
	new_list->anchor.prev = NULL;
	new_list->num_members = 0;
	return 1;
}
My402ListElem *My402ListNext (My402List *new_list, My402ListElem *node)
{
	if(new_list->anchor.next==NULL)
		return NULL;
    if(node->next==&new_list->anchor)
		return NULL;
	else
		return node->next;
}
int My402ListLength (My402List *new_list)
{
	if(new_list->anchor.next==NULL)
		return 0;
	My402ListElem *node=NULL;
	new_list->num_members=0;
	for(node=My402ListFirst(new_list);node!= NULL;node=My402ListNext(new_list,node))
	{
		new_list->num_members=new_list->num_members+1;
	}
	return new_list->num_members;
}
int My402ListEmpty (My402List *new_list)
{
	if(new_list->anchor.next==NULL && new_list->anchor.prev==NULL)
	{
		return 1;
	}
	else
		return 0;
}
int My402ListAppend (My402List *new_list, void *obj_val)
{
    My402ListElem* k=(My402ListElem*)malloc(sizeof(My402ListElem));
    My402ListElem* k1;
    if(k == NULL)
	{
		return 0;        
	}
    else
	{
        k->obj=obj_val;
		if(new_list->anchor.next==NULL || new_list->anchor.prev==NULL)
		{
			k->next=&new_list->anchor;
			new_list->anchor.next=k;
			new_list->anchor.prev =k;
			k->prev=&new_list->anchor;
		}
		else
		{
			k1=My402ListLast(new_list);
			k->next=&new_list->anchor;	
			k->prev=k1;
			new_list->anchor.prev=k;
			k1->next=k;
		}
        return 1;
	}
}
int My402ListPrepend (My402List *new_list, void *obj_val)
{
	My402ListElem* k=(My402ListElem*)malloc(sizeof(My402ListElem));
    My402ListElem* k1;
    if(k == NULL)
	{
    }
    else
	{
        k->obj=obj_val;
		if(new_list->anchor.next==NULL || new_list->anchor.prev==NULL)
		{
		k->next=&new_list->anchor;
		k->prev=&new_list->anchor;
		new_list->anchor.next=k;
		new_list->anchor.prev=k;
		   // printf("prepended1\n");
		}
		else
		{
		k1=My402ListFirst(new_list);
		k->next=k1;	
		k->prev=&new_list->anchor;
		new_list->anchor.next=k;
		k1->prev=k;
		   // printf("prepended2\n");     
		}
        return 1;
	}
	return 0;
}
void My402ListUnlink (My402List *new_list, My402ListElem *node)
{
	My402ListElem *temp1=node;
	if(temp1==NULL)
	{		
	}
	else
	{
		if(temp1==My402ListFirst(new_list)&&temp1==My402ListLast(new_list))
		{
			new_list->anchor.next =NULL;
			new_list->anchor.prev= NULL;
			free(temp1);
		}
		else	
		if((temp1==My402ListFirst(new_list)))
		{
			new_list->anchor.next=temp1->next;
			temp1->next->prev=&new_list->anchor;
			temp1->next=NULL;
            temp1->prev=NULL;
			free(temp1);
		}
		else if(temp1==My402ListLast(new_list))
		{
			temp1->next->prev=temp1->prev;
			temp1->prev->next=temp1->next;
			free(temp1);
		}
		else
		{
			if((new_list->anchor.next==NULL) && (new_list->anchor.prev==NULL))
			{
			}
			else
			{
				temp1->prev->next=temp1->next;
				temp1->next->prev=temp1->prev;
				temp1->next=NULL;
				temp1->prev=NULL;
				free(temp1);
			}
		}
	}
}
void My402ListUnlinkAll (My402List *new_list)
{
	My402ListElem *current = My402ListFirst(new_list);
	My402ListElem *temp =My402ListFirst(new_list);
	if(temp==NULL)
	{
		return;
	}
	if((My402ListLength(new_list))==1)
	{
		temp->next=NULL;
		temp->prev=NULL;
		new_list->anchor.next==NULL;
		new_list->anchor.prev==NULL;
		free(temp);
		return;
	}
	while(temp!=&new_list->anchor)
	{
		temp = current;
		current = My402ListNext(new_list,current);
		My402ListUnlink(new_list,temp);
		if(current==NULL)
		{
			new_list->anchor.prev=NULL;
			new_list->anchor.next=NULL;
			return;
		}
		else
		{
			temp=current;
		}
	}
}
int My402ListInsertAfter (My402List *new_list, void *obj_val, My402ListElem *node)
{
	My402ListElem *elem = (My402ListElem*)malloc(sizeof(*elem));
    if(elem==NULL)
    {
       // printf("Cannot allocate memory.");
        return 0;
    }
	else
	{
		if(node==NULL || new_list->anchor.next==NULL)
		{
			int insert1=0;
			insert1=My402ListAppend(new_list,obj_val);
        }	
		else
		{
			elem->obj=obj_val;        
			elem->next=node->next;
			node->next->prev=elem;
			elem->prev=node;
			node->next=elem;
			return 1;
		}
    }
	return 1;
}
int My402ListInsertBefore (My402List *new_list, void *obj_val, My402ListElem *node)
{
	My402ListElem *elem = (My402ListElem*)malloc(sizeof(*elem));
    if(elem==NULL)
    {
        return 0;
    }
	else
	{
	if(node==NULL ||new_list->anchor.next==NULL)
	{
		int insert1=0;
        insert1=My402ListPrepend(new_list,obj_val);
	}	
	else
	{
		if(node==My402ListFirst(new_list))
		{
			int insert1=0;
			insert1=My402ListPrepend(new_list,obj_val);
		}
		else
		{	
			elem->obj=obj_val;        
			elem->prev=node->prev;
			elem->next=node;
			node->prev->next=elem;
			node->prev=elem;
		}	
	}
	return 1;
    }
}
My402ListElem *My402ListPrev (My402List *new_list, My402ListElem *node)
{
	if(new_list->anchor.next==NULL)
		return NULL;
    if(node->prev==&new_list->anchor)
		return NULL;
	else
		return node->prev;
}
My402ListElem *My402ListFind (My402List *new_list, void *obj_val)
{
	if(new_list->anchor.next==NULL)
		return NULL;
	My402ListElem *node;
	for(node=My402ListFirst(new_list);node!= NULL;node=My402ListNext(new_list,node))
	{
		if(node->obj==obj_val)
		{
			return node;
		}
	}
	return NULL;
}