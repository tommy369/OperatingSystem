#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "cs402.h"
#include "my402list.h"

typedef struct transaction
{
    char transtype;
    unsigned int time;
    char amounts[10];
    float amount;
    char desc[100];
}Mytransaction;

int Mychecker(My402List*list,Mytransaction * object)
{
	/*code for traverse from homework slide - start */       
    My402ListElem *elem=NULL;      
    for (elem=My402ListFirst(list);
		elem != NULL;
		elem=My402ListNext(list, elem))
	
	/*code for traverse from homework slide - end */                 
    {  
        Mytransaction * t =(Mytransaction *) elem->obj;
        if( t->time == object->time )
        {
            printf("error-timestamp same");
            return FALSE;
        }
    }  
    return TRUE;                  
} 
                
void Myconverter(Mytransaction *object)
{                  
	char a[24];
    int i;
    time_t br =(time_t)(object->time);
    strcpy( a , ctime(&br));
	printf("\n| ");
	for(i=0;i<24;i++)
	{
		if(i < 10 || i>18)            
        {
			printf("%c", a[i]);            
		}                                        
	}                          
}                                   
    
void MyListTraverse(My402List*list)
{
/*code for traverse from homework slide - start */       
       int bal = 0;
       char balance[10];
       My402ListElem *elem=NULL;      
       for (elem=My402ListFirst(list);
			elem != NULL;
            elem=My402ListNext(list, elem))
/*code for traverse from homework slide - end */ 
			{  
              int c;
              Mytransaction * t1 = (Mytransaction *)elem->obj;     
              Myconverter(t1);
              printf(" | %-24.24s |",t1->desc); 
              float amtf;
              int amti,h,d[10];
              char a1[10];
              char b1[10];
              amtf = atof(t1->amounts) * 100;
              amti = (int)(amtf);
              int op,lo,x,y,s,s1,comma = 0;            
              if( t1->transtype == '+' )
              { 
                  s = y = strlen(t1->amounts);
                  if ( s==7 || s==8 || s==9)
                  {
					s++;
					comma = 1;
				  }
                  else if ( s == 10)
                  {
					s = s+2;
					comma = 2;
                  }
                  s = s+4;
                  printf("  ");                 
                  for(op=16-s;op>0;op--)
                  printf(" ");
                  char tyt[y+comma];
                  int r=1;
                  int chj = y+comma-1;
                  for(lo=y-1; lo>=0;lo--)
                  {
                    if( r== 7 && comma != 0)
                    {    
						tyt[chj] = ',';
                        chj--;
					}
					if( r== 10 && comma == 2)
					{    
						tyt[chj] = ',';
                        chj--;
					} 
					tyt[chj] = t1->amounts[lo];
					chj--; 
					r++;                               
				  }
				  for(lo=0; lo<y+comma;lo++)
				  printf("%c",tyt[lo]);                       
                  printf("  ");
              }
              else if( t1->transtype == '-')
              {   
                  s1 = y = strlen(t1->amounts);
                  if ( s1==7 || s1==8 || s1==9)
                  {
					s1++;
					comma = 1;}
					else if ( s1 == 10)
					{
						s1 = s1+2;
						comma = 2;
					}
				   s1 = s1+4;
                   printf(" (");
                   for(op=16-s1;op>0;op--)
                   printf(" ");
                   char tyt1[y+comma];
                   int r1=1;
                   int chj1 = y+comma-1;
                   int lo1;
                   for(lo1=y-1; lo1>=0;lo1--)
                   {
						if( r1== 7 && comma != 0)
                        {    
							tyt1[chj1] = ',';
							chj1--;
						}
                        if( r1== 10 && comma == 2)
                        {    
							tyt1[chj1] = ',';
							chj1--;
						}            
						tyt1[chj1] = t1->amounts[lo1];
						chj1--;
						r1++; 
				   }
				   for(lo1=0; lo1<y+comma;lo1++)
						printf("%c",tyt1[lo1]); 
						printf(") ");
               }
               else
               {
               } 
               printf("| ");
              
              //balance calculation
              if( t1->transtype == '+' )
              {
                  bal = bal + amti;
			  }
              else if(  t1->transtype == '-' )
              {
                bal = bal - amti;
              }
              else
			  {
              }
              int as;
              int te = 0;
              int asc; 
              int op2,lo2,x2,y2,y3,s2,s12,comma2 = 0;            
              if( bal > 0 )
              { 
                  s2 = y3 = strlen(balance);
                  y2 = y3;
                  y2++;
                  s2++;//for decimal
                  if ( s2==6 || s2==7 || s2==8 || s2==9)
                  {
					s2++;
					comma2 = 1;}
					else if ( s2 == 10)
					{
						s2 = s2+2;
						comma2 = 2;
					}
					s2 = s2+4;
					char tyt2[y2+comma2];
					if(y3 ==1)
					printf("          0.0%c  ",balance[0]);
					else if(y3 == 2)
					printf("          0.%c%c  ",balance[0],balance[1]);
					else
					{
						printf("  ");                 
						for(op2=16-s2;op2>0;op2--)
							printf(" ");
					    int r2=1;
					    int chj2 = y2+comma2-1;
                        for(lo2=y2-2; lo2>=0;lo2--)
                        {
							if( r2 == 3)
							{ 
								tyt2[chj2] = '.';
								chj2--;
							}
							if( r2== 6 && comma2 != 0)
							{    
								tyt2[chj2] = ',';
								chj2--;
                            }
							if( r2== 9 && comma2 == 2)
							{    
								tyt2[chj2] = ',';
								chj2--;
                            } 
                            tyt2[chj2] = balance[lo2];
							chj2--; 
							r2++;                               
                        }
						for(lo2=0; lo2<y2+comma2;lo2++)
						printf("%c",tyt2[lo2]);  
                    }                     
                  printf("  ");
              }
              else if( bal < 0)
              {   
                  s12 = y3 = strlen(balance);
                  y2 = y3;
                  y2++;
                  s2++;//for decimal
                  if ( s12==6 || s12==7 || s12==8 || s12==9)
                  {
					s12++;
					comma2 = 1;
				  }
                  else if ( s12 == 10)
                  {
					s12 = s12+2;
					comma2 = 2;
                  }
                  s12 = s12+4;
                  char tyt12[y2+comma2];
                  if(y3 ==1)
                  printf(" (        0.0%c) ",balance[0]);
                  else if(y3 == 2)
                  printf(" (        0.%c%c) ",balance[0],balance[1]);
                  else
				  {
					printf(" (");
					for(op2=16-s12;op2>0;op2--)
					printf(" ");               
					int r12=1;
					int chj12 = y2+comma2-1;
					int lo12;
					for(lo12=y2-2; lo12>=0;lo12--)
					{
                        if( r12 == 3)
                        { 
							tyt12[chj12] = '.';
							chj12--;
                        }
                        if( r12== 6 && comma2 != 0)
                        {    
							tyt12[chj12] = ',';
							chj12--;
                        }
                        if( r12== 9 && comma2 == 2)
                        {    
							tyt12[chj12] = ',';
							chj12--;
                        }            
                        tyt12[chj12] = balance[lo12];
                        chj12--;
                        r12++; 
                    }
					for(lo12=0; lo12<y2+comma2;lo12++)
                       printf("%c",tyt12[lo12]);
                  } 
                  printf(") ");
               }
               else
               {
               } 
               printf("|");
            }      
}  
/*code for traverse from homework slide - start */ 
static
void BubbleForward(My402List *pList, My402ListElem **pp_elem1, My402ListElem **pp_elem2)
{
    My402ListElem *elem1=(*pp_elem1), *elem2=(*pp_elem2);
    void *obj1=elem1->obj, *obj2=elem2->obj;
    My402ListElem *elem1prev=My402ListPrev(pList, elem1);   
    My402ListElem *elem2next=My402ListNext(pList, elem2);

    My402ListUnlink(pList, elem1);
    My402ListUnlink(pList, elem2);
    if (elem1prev == NULL) 
	{
        (void)My402ListPrepend(pList, obj2);
        *pp_elem1 = My402ListFirst(pList);
    } 
	else 
	{
        (void)My402ListInsertAfter(pList, obj2, elem1prev);
        *pp_elem1 = My402ListNext(pList, elem1prev);
    }
    if (elem2next == NULL) 
	{
        (void)My402ListAppend(pList, obj1);
        *pp_elem2 = My402ListLast(pList);
    } 
	else 
	{
        (void)My402ListInsertBefore(pList, obj1, elem2next);
        *pp_elem2 = My402ListPrev(pList, elem2next);
    }
}

static
void BubbleSortForwardList(My402List *pList, int num_items)
{
    My402ListElem *elem=NULL;
    int i=0;
    if (My402ListLength(pList) != num_items) 
	{
        fprintf(stderr, "List length is not %1d in BubbleSortForwardList().\n", num_items);
        exit(1);
    }
    for (i=0; i < num_items; i++) 
	{
        int j=0, something_swapped=FALSE;
        My402ListElem *next_elem=NULL;
        for (elem=My402ListFirst(pList), j=0; j < num_items-i-1; elem=next_elem, j++) 
		{
           Mytransaction * tr1 = (Mytransaction *)( elem-> obj);// own code
           int cur_val=tr1->time, next_val = 0;// own code
            next_elem=My402ListNext(pList, elem);
            Mytransaction * tr2 = (Mytransaction *)(next_elem->obj);// own code
			next_val = tr2->time;// own code
            if (cur_val > next_val) 
			{
                BubbleForward(pList, &elem, &next_elem);
                something_swapped = TRUE;
            }
        }
        if (!something_swapped) break;
    }
} 
/*code used from slide end */       

void display(My402List * list)
{
     My402ListElem * elem = NULL;
     for (elem=My402ListFirst(list);elem != NULL;elem=My402ListNext(list, elem)) 
	 {
        Mytransaction * disp =(Mytransaction*) elem->obj; 
        printf("%d ", disp->time );
        printf("%d address %d\n", elem , &(list->anchor) );
        if( elem->next == &(list->anchor) )
            printf("%d points to anchor\n", elem);
     } 
}
main( int argc, char *argv[] )
{
      int status;
      char tt;
      unsigned int ti;
      float ff;
      char amt[10];
      char des[100];
      int number = 1024;
      char line[1026];
      My402List *list =(My402List *)malloc(sizeof(My402List));
      printf("start");
      FILE * fp;
      if( argc == 1)
      { 
		fp = stdin;
        if(ferror (fp))
		{
            printf("error opening file!");
            clearerr(fp);
            exit(0);
		} 
      }
      else
	  {
		fp = fopen( "gg.txt" , "r" );
		if(ferror (fp))
		{
            printf("error opening file!");
            clearerr(fp);
            exit(0);
		} 
		else
		{                              
			My402ListElem *elem = (My402ListElem *)malloc(sizeof(My402ListElem));
			status = My402ListInit(list);
			if(status != FALSE)
			{
				while( fgets(line,number,fp) != NULL)
				{         
					sscanf(line,"%c\t%d\t%s\t%[^\n]s",&tt,&ti,amt,des);
					printf("%c\n%d\n%s\n%s\n",tt,ti,amt,des);
					Mytransaction * tr =(Mytransaction*)malloc(sizeof(Mytransaction));
					tr->transtype = tt;
					tr->time      = ti;
					strcpy( tr->amounts , amt );
					strcpy( tr->desc , des );      
					status = Mychecker(list,tr);
					if(status == TRUE)
					{
						status = My402ListPrepend(list,tr);
						if(status != TRUE )
						printf("not prepended");
					}
				} 
            }
            else
            {
                printf("list not initialised");
            }              
		}	
      
      BubbleSortForwardList(list,My402ListLength(list));
      //***output********* 
      printf("\n00000000011111111112222222222333333333344444444445555555555666666666677777777778");
      printf("\n12345678901234567890123456789012345678901234567890123456789012345678901234567890");
      printf("\n+-----------------+--------------------------+----------------+----------------+");
      printf("\n|       Date      | Description              |         Amount |        Balance |");
      printf("\n+-----------------+--------------------------+----------------+----------------+");
      MyListTraverse(list);  
      printf("\n+-----------------+--------------------------+----------------+----------------+");  
      }
}

   
