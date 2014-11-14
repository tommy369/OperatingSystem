//where to write free?..
//if possible test with valgrind
//check out .6f ...12.06g..etc
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include "cs402.h"
#include "my402list.h"
#include "shared.h"
typedef struct packet_struct
	{
		int token_id;
		int num_of_tokens_reqd;
		double enter_timeq1;
		double enter_timeq2;
		double exit_timeq1;
		double exit_timeq2;
		double inter_arrival_time;
		double q1_service;
	}packet;
struct timeval tv;
char *pass_arg1;
void *handler();
void interrupt();
int pass_arg2;
int tokens_drop=0;
int ctrl_signl=1;
int packets_drop=0;
double avg_time_in_Q1=0;
double q2_packet_time=0.0;
double q1_packet_time=0;
double avg_time_in_Q2=0;
double avg_packet_in_S=0;
double avg_time_in_system=0.0;
double token_drop_probability;
double packet_drop_probability;
FILE *fp=NULL;
int read_count=0;
int count_serviced_packets=0;
double avg_packet_service_time=0;
double total_service_time=0;
double total_inter_arrival_time=0;
double total_emulation_time=1;
int record_point=0;
int file_input=0;
int visit_chk=0;
int return_val;
double avg_packet_inter_arrival_time;
int visit_count=0;
double packet_inter_arrival_time=3000;
int chk_packet=0;
int x;
char input[4000];
double start_time=0;
double standard_deviation=0;
int chk_counter=0;
int count_token=1;
int counter=0;
void * file_read(char*, int);
void *pthreadCreate(void*);
void *pthreadService(void*);
void *token_bucket(void*);
My402List *new_list;
My402List *service_list;
int num_of_tokens=10;
int service_variable=0;
int count_current_packet=1;
packet *p;
int tid=1;
int tokens_reqd=3;

pthread_t threads1;
pthread_t threads2;
pthread_t threads3;
pthread_t threads4;

pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t q2_not_empty=PTHREAD_COND_INITIALIZER;

int num_of_packets=20;
char *store_tokens_reqd[1000];
float lambda=0.5;
float mu=0.35;
float r=4.0;
float time_enter;
double time_service=9253;
int add_tokens=0;
int alpha=0;
int set_file_input=0;
//sigset_t new;s
//pthread_t user_threadID;
sigset_t action;
int main(int argc, char *argv[])
{				
	int comand_i=1;
	if(argc<1)
	{	
	}
	else
	{
		while(argc!=1)
		{	
			if(strcmp(argv[comand_i],"-t")==0)
			{
				file_input=1;						
				set_file_input=1;
				pass_arg1=argv[++comand_i];
				pass_arg2=comand_i;
				comand_i++;
				file_read(pass_arg1,pass_arg2);
				argc--;
				argc--;
			}
			else if(strcmp(argv[comand_i],"-n")==0)
			{
				comand_i++;
				num_of_packets=atoi(argv[comand_i]);
				comand_i++;
				argc--;
				argc--;
			}
			else if(strcmp(argv[comand_i],"-P")==0)
			{
				comand_i++;
				tokens_reqd=atoi(argv[comand_i]);
				comand_i++;	
				argc--;	
				argc--;	
			}
			else if(strcmp(argv[comand_i],"-B")==0) 
			{
				comand_i++;
				num_of_tokens=atoi(argv[comand_i]);
				comand_i++;
			     argc--;
						argc--;
			}	
			else if(strcmp(argv[comand_i],"-r")==0)
			{	
				comand_i++;
				r=atof(argv[comand_i]);
				comand_i++;
				argc--;
				argc--;
			}
			else if(strcmp(argv[comand_i],"-lambda")==0)
			{	
				comand_i++;
				lambda=atof(argv[comand_i]);
				comand_i++;
				argc--;
				argc--;
			}
			else if(strcmp(argv[comand_i],"-mu")==0)
			{	
				comand_i++;
				mu=atof(argv[comand_i]);
				comand_i++;
				argc--;
				argc--;
			}
			else
			{
				fprintf(stderr,"Invalid Input");
				exit(0);
			}
		}
	}
		sigemptyset(&action);
		sigaddset(&action, SIGINT);
		pthread_sigmask(SIG_BLOCK,&action,NULL);
		pthread_create(&threads4, NULL, handler, argv[1]);
		new_list = (My402List*)malloc(sizeof(My402List));
		My402ListInit(new_list);
		service_list = (My402List*)malloc(sizeof(My402List));
		My402ListInit(service_list);
		if(service_list==NULL)
			{
				fprintf(stderr,"Error allocating memory to my402list");
				exit(0);
			}	
			return_val=pthread_create(&threads1,NULL,(void *)token_bucket,NULL);
			if(return_val==0)
			{
				//printf("Token inserted...Success \n");	
			}
			return_val=pthread_create(&threads2,NULL,(void *)pthreadCreate,NULL);
			if(return_val==0)
			{
				//printf("Thread creation success..append \n");	
			}
			return_val=pthread_create(&threads3,NULL,(void *)pthreadService,NULL);
			if(return_val==0)
			{
				//printf("Thread creation success..service \n");	
			}
			set_file_input=2;
			if(set_file_input==2)
			{
			printf("Emulation Parameters:\n");
			printf("lambda=%f\n",lambda);
			printf("mu=%f\n",mu);
			printf("r=%f\n",r);
			printf("B=%d\n",num_of_tokens);
			printf("P=%d\n",tokens_reqd);
			printf("number to arrive=%d\n",num_of_packets);
			}			
			pthread_join(threads1, NULL);	
			//printf("thread1 destroyed...\n");
			pthread_join(threads2, NULL);
			//printf("thread2 destroyed...\n");
			pthread_join(threads3, NULL);	  
			//printf("thread3 destroyed...\n");
			pthread_join(threads4, NULL);
			//printf("threads 4 destroyed");
 
			//printf("--------------------STATISTICS--------------------------\n");
			
			avg_packet_inter_arrival_time=(total_inter_arrival_time)/(num_of_packets);
			printf("Avg packet inter-arrival time%.6f\n",avg_packet_inter_arrival_time);			
			int packets_serviced=num_of_packets-(My402ListLength(new_list)+My402ListLength(service_list));
			avg_packet_service_time=(total_service_time)/(count_serviced_packets);
			printf("Avg packet service time%.6f\n",avg_packet_service_time);
			avg_time_in_Q1=(q1_packet_time)/(total_emulation_time);
			printf("Avg no. of packets in Q1%.6f\n",avg_time_in_Q1);
			avg_time_in_Q2=(q2_packet_time)/(total_emulation_time);
			printf("Avg no. of packets in Q2%.6f\n",avg_time_in_Q2);
			avg_packet_in_S=(total_service_time)/(total_emulation_time);
			printf("Avg no. of packets at S%.6f\n",avg_packet_in_S);
			avg_time_in_system=(total_emulation_time)/(num_of_packets);
			printf("Avg time a packet spent in system%.6f\n",avg_time_in_system);	
			double param1=((total_emulation_time)*pow(10,-3))*((total_emulation_time)*pow(10,-3));
			printf("Standard deviation for time spent in system\n");
			token_drop_probability=(tokens_drop)/(num_of_tokens);
			printf("token drop probability%.6f\n",token_drop_probability);
			packet_drop_probability=(packets_drop)/(num_of_packets);
			printf("packet drop probability%.6f\n",packet_drop_probability);
	return (0);
	}
void Traverse(My402List *new_list)
	{
		My402ListElem *elem;
		elem=My402ListFirst(new_list);
		if(elem==NULL)
			{	
			//printf("list empty..\n");		
			}
		for(elem=My402ListFirst(new_list);elem!= NULL;elem=My402ListNext(new_list,elem))
			{
			printf("\n");			
			packet *packet_elem = (packet *)(elem->obj);						
			printf("elems1...%d  ",(packet_elem->token_id));
			printf("elems2...%d  ",(packet_elem->num_of_tokens_reqd));
			printf("elems3...%lf  ",(packet_elem->enter_timeq1));			
			printf("elems4...%lf  ",(packet_elem->exit_timeq1));
			printf("elems5...%lf  ",(packet_elem->enter_timeq2));
			printf("elems6...%lf  ",(packet_elem->exit_timeq2));
			//q1_packet_time=q1_packet_time+((packet_elem->exit_timeq1)-(packet_elem->enter_timeq1));		
			printf("\n");
			}
	}
void *pthreadCreate(void* sample)
	{								
				//printf("pThreadCreate Created ...\n");	
				while(TRUE)
				{					
					double prev_inter_arrival_time=0;	
					if(set_file_input==1)
					{				
						int create_sleep=(lambda*1000);
						usleep(create_sleep);
					}
					else
					{
						if((1/lambda)<=10)
						{
							int create_sleep=((1/lambda)*1000*1000);
							usleep(create_sleep);
						}
						else
						{
							int create_sleep=(10*1000*1000);
							usleep(create_sleep);
						}
					}
					pthread_mutex_lock(&m);
					if(count_current_packet>num_of_packets && (My402ListLength(new_list)==0) && My402ListLength(service_list)==0)
					{
						pthread_mutex_unlock(&m);
						pthread_exit(0);
					}					
					if(chk_packet!=num_of_packets)
					{						
						count_current_packet++;
						chk_packet++;	
						if(file_input==1)
						{					
							file_read(pass_arg1,pass_arg2);
						}
						total_service_time=(total_service_time+time_service); 
						packet *p2=(packet*) malloc(sizeof(packet));
						if(p2==NULL)
							fprintf(stderr,"Error allocating memory to argument\n");				
						p2->token_id=tid;
						p2->num_of_tokens_reqd=tokens_reqd;
						gettimeofday(&tv,NULL);	
						double temp_time=((tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0)-start_time;
						tid++;
						gettimeofday(&tv,NULL);	
						p2->enter_timeq1=((tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0)-start_time;
						prev_inter_arrival_time=p2->enter_timeq1;
						printf("%08.03lfms:p%d arrives, needs %d, inter-arrival time%.6f\n",temp_time,p2->token_id,p2->num_of_tokens_reqd,prev_inter_arrival_time);
						printf("%08.03lfms:p%d enters Q1\n",p2->enter_timeq1,p2->token_id);
						My402ListAppend(new_list,p2);
						My402ListElem *temp=My402ListFirst(new_list);
						packet *p3=(packet*)(temp->obj);
						if((p3->num_of_tokens_reqd)>num_of_tokens)
							{
							packets_drop++;
							gettimeofday(&tv,NULL);	
							double packet_time=((tv.tv_sec*1000.0)+(tv.tv_usec)/1000.0)-start_time;	
							printf("%08.03lfms: packet p%d arrives,needs %d tokens, dropped\n",packet_time,(p3->token_id),(p3->num_of_tokens_reqd));
							}
						if(add_tokens>=(p3->num_of_tokens_reqd))
						{
							add_tokens=add_tokens-(p3->num_of_tokens_reqd);				        
							q1_packet_time=q1_packet_time+((p3->exit_timeq1)-(p3->enter_timeq1));
							gettimeofday(&tv,NULL);
							p3->exit_timeq1=((tv.tv_sec*1000.0)+(tv.tv_usec)/1000.0)-start_time;
							My402ListUnlink(new_list,My402ListFirst(new_list));
							printf("%08.03lfms:p%d leaves Q1, time in Q1=%.6f, token bucket now has %d tokens\n",p3->exit_timeq1,p3->token_id,p3->exit_timeq1,add_tokens);
							gettimeofday(&tv,NULL);	
							p3->enter_timeq2=((tv.tv_sec*1000.0)+(tv.tv_usec)/1000.0)-start_time;	
							printf("%08.03lfms:p%d enters Q2\n",p3->enter_timeq2,p3->token_id);
							My402ListAppend(service_list,(void *)p3);	
							gettimeofday(&tv,NULL);	
							p3->q1_service=((tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0)-start_time;
							p3->inter_arrival_time=packet_inter_arrival_time;
							total_inter_arrival_time=total_inter_arrival_time+packet_inter_arrival_time;
								if((packet_inter_arrival_time)>(p3->q1_service))
								{
										double diff1=(p3->inter_arrival_time)-(p3->q1_service);
										int diff2=(int)(diff1);
										pthread_mutex_unlock(&m);
										//printf("Sleeping for...%d",diff2);
										usleep(diff2);
										pthread_mutex_lock(&m);
								}
								else
								{
										//printf("No wait...time exceeded");
										usleep(0);
								}
								if(My402ListLength(service_list)>0)
								{
									//printf("pthreadService has element\n");						
									pthread_cond_signal(&q2_not_empty);
								}
							}
				}
				else
				{
					//printf("not enough tokens\n");				
				}
				pthread_mutex_unlock(&m);
			}		
	}
void *token_bucket(void* val)
{
		//printf("Token bucket thread entered..\n");
		double temp_val1,temp_val2,temp_val3;
		My402ListElem *elem=NULL;
		while(TRUE)
			{
				int bucket_sleep=((1/r)*1000*1000);
				//printf("Sleeping for %d\n",bucket_sleep);
				usleep(bucket_sleep);
				pthread_mutex_lock(&m);
				if(count_current_packet>num_of_packets && My402ListLength(new_list)==0 && My402ListLength(service_list)==0)
				{
					//printf("Thread exiting bucket\n");
					pthread_mutex_unlock(&m);
					pthread_exit(0);
				}
			//tokens to be added for packets
			if(count_current_packet<=num_of_packets)
				{
					//pthread_mutex_lock(&m);
					if(add_tokens<=num_of_tokens)		
					{
						if(chk_counter==0)
						{
						gettimeofday(&tv,NULL);	
						temp_val1=(tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0;
						temp_val2=temp_val1;
						start_time=temp_val2;
						temp_val3=temp_val1-temp_val2;
						//printf("tsfile%s",lambda);
						printf("\n%08.03lfms: Simulation Begins \n",(temp_val3));
						chk_counter++;
						}
						else
						{
						gettimeofday(&tv,NULL);	
						temp_val1=(tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0-start_time;
						//store3=temp_val1-temp_val2;
							if(add_tokens<num_of_tokens )
							{
								if(count_current_packet<=num_of_packets)
									{
									add_tokens++;
									gettimeofday(&tv,NULL);	
									temp_val1=(tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0-start_time;
									printf("%08.03lfms: token t%d arrives, token bucket now has %d tokens	\n",temp_val1,count_token,add_tokens);
								  count_token++;
									}
							if(My402ListLength(new_list)!=0)
							{
									elem=My402ListFirst(new_list);
									if(elem!=NULL)
									{
										packet *bucket_elem = (packet *)(elem->obj);				
										//printf("***reqd...%d\n",(bucket_elem->num_of_tokens_reqd));
										//printf("**add_tokens%d\n",add_tokens);
											if(add_tokens>=(bucket_elem->num_of_tokens_reqd))
											{			
											//printf("Moving Q1 to Q2\n");
											//printf("%fms: token t%d arrives, token bucket now has %d tokens	\n",store3,count_token,add_tokens);
											//count_token++;
											add_tokens=add_tokens-bucket_elem->num_of_tokens_reqd;
											My402ListAppend(service_list,bucket_elem);
											My402ListUnlink(new_list, (void *)elem);
											gettimeofday(&tv,NULL);	
											double tempp=(tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0-start_time;
											bucket_elem->exit_timeq1=tempp;
											printf("%08.03lfms:p%d leaves Q1, time in Q1=%.6f, token bucket now has %d tokens\n",bucket_elem->exit_timeq1,bucket_elem->token_id,bucket_elem->exit_timeq1,add_tokens);
											gettimeofday(&tv,NULL);	
											bucket_elem->enter_timeq2=((tv.tv_sec*1000.0)+(tv.tv_usec)/1000.0)-start_time;	
											printf("%08.03lfms:p%d enters Q2\n",bucket_elem->enter_timeq2,bucket_elem->token_id);
												if(My402ListLength(service_list)>0)
												{			
												//printf("Signalling Q2 not empty\n");
												pthread_cond_signal(&q2_not_empty);
												//pthread_mutex_unlock(&m);	
												}
											}
										}
									}
								}
							else
							{
									tokens_drop++;
									gettimeofday(&tv,NULL);	
									double tempp=(tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0-start_time;
									printf("%08.03lfms: token t%d arrives, dropped\n",tempp,(add_tokens+1));
							}
						}
					}
				else
					{
					//printf("token capacity reached limit\n");
					}
					}
			pthread_mutex_unlock(&m);			
			}
	}
	void *pthreadService(void* pass_param)
	{					
			if(My402ListLength(service_list)==0)
			//printf("service list empty\n");			
			//printf("pthreadService created..\n");				
			while(TRUE)
			{
					pthread_mutex_lock(&m);	
					if((count_current_packet>num_of_packets && My402ListLength(new_list)==0 && My402ListLength(service_list)==0) || (My402ListLength(new_list)==0 && ctrl_signl==0 && My402ListLength(service_list)==0))
					{
						//printf("Thread exiting--Service\n");
						gettimeofday(&tv,NULL);	
						total_emulation_time=((tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0)-start_time;
						pthread_mutex_unlock(&m);		
						break;
						//pthread_exit(0);
					}	
					//printf("Sleeping for ...%d\n",(int)time_service);		
					gettimeofday(&tv,NULL);		
					if(My402ListLength(service_list)==0)
					{
						pthread_cond_wait(&q2_not_empty,&m);
						//printf("Waiting done...");					
						pthread_mutex_unlock(&m);						
					}				
					while(My402ListLength(service_list)!=0)
					{
					//printf("Servicing....\n");
					My402ListElem *temp=My402ListFirst(service_list);
					packet *p3copy=(packet *)(temp->obj);
					//Traverse(service_list);
					My402ListUnlink(service_list, temp);	
					gettimeofday(&tv,NULL);
					p3copy->exit_timeq2=((tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0)-start_time;
					printf("%08.03lfms:p%d begin service at S, time in Q2=%.6lf\n",p3copy->exit_timeq2,p3copy->token_id,p3copy->exit_timeq2);	
					pthread_mutex_unlock(&m);	
					if(set_file_input==1)	
					{
					usleep((int)(time_service*1000));				
					//set_file_input=0;
					}
					else
					{
					if((1/mu)<=10)
					{
					usleep((int)((1/mu)*1000*1000));				
					}
					else
					{	
					usleep((int)(10*1000*1000));				
					}
					}
					pthread_mutex_lock(&m);		
					gettimeofday(&tv,NULL);
					double system_exit_time=((tv.tv_sec)*1000.0+(tv.tv_usec)/1000.0)-start_time;
					printf("%08.03lfp%d:departs from S, service time=%.6lf, time in system=%.6lf\n",system_exit_time,p3copy->token_id,time_service,((system_exit_time)-(p3copy->enter_timeq1)));
					count_serviced_packets++;
					q2_packet_time=q2_packet_time+((p3copy->exit_timeq2)-(p3copy->enter_timeq2))-start_time;
					//printf("Q1-Len=%d Q2-Len=%d\n",My402ListLength(new_list),My402ListLength(service_list));
					//Traverse(service_list);
					}
			pthread_mutex_unlock(&m);						
		}
	pthread_cancel(threads4);
	return NULL;
}
void interrupt()
{					
			pthread_mutex_lock(&m);
			My402ListUnlinkAll(new_list);
			My402ListUnlinkAll(service_list);
			//printf("Cancelling thread1\n");
			pthread_cancel(threads1);
			pthread_cancel(threads2);
			//printf("Cancelled thread1 & 2\n");
			//printf("broadcasting..");
			pthread_cond_broadcast(&q2_not_empty);
			//printf("broadcast done..");
			pthread_mutex_unlock(&m);
}
void *handler(void *argumnt)
{
			struct sigaction act={{0}};					
			act.sa_handler=interrupt;
			ctrl_signl=sigaction(SIGINT, &act,NULL);
			pthread_sigmask(SIG_UNBLOCK, &action, NULL);
			sleep(2147483647);
			return NULL;
}
void *file_read(char *a,int val)
{															
						//printf("File read entered...");						
						char *argv;
						argv=a;			
						char *alpha_chk1;
						alpha_chk1="[";
						char *store_temp=argv;
						if(strncmp((argv),alpha_chk1,1)==0)	
						{
							store_temp[strlen(store_temp)-1]='\0';
							//printf("store_temp%s",store_temp);
							memmove(store_temp, store_temp+1, strlen(store_temp));
							//printf("store_temp%s",store_temp);
						}
						if(read_count==0)
						{
							//printf("Reading...");
							fp=fopen(store_temp,"r");
							read_count++;
						}
						if(fp==NULL)
						{
							fprintf(stderr,"No input present");
							exit(0);
						}
						else
						{
						int chk=0;
						if(counter==0)
						{
						if((fgets(input,sizeof(input),fp) ) != NULL)
						{
						sscanf(input,"%d",&num_of_packets);
						//printf("//////////////////////////////num_of_packets%d\n",num_of_packets);
						++counter;
						}
						}
						if(counter>0)
						{
							if((num_of_packets)>0)
							{
								chk=(num_of_packets+1);
								if(chk!=0)
								{
									chk--;
									//printf("In function...");
									//int point=0;
									if(fgets(input,sizeof(input),fp)!=NULL)
									{
										//printf("\nInput line = %s\n", input);						
										sscanf(input,"%lf%d%lf",&packet_inter_arrival_time,&tokens_reqd,&time_service);
										lambda=(1/packet_inter_arrival_time);
										mu=(1/time_service);
										if(set_file_input==1)
										{
										printf("Emulation Parameters\n");
										printf("lambda%f\n",lambda);
										printf("mu%f\n",mu);
										printf("r%f\n",r);
										printf("B%d\n",num_of_tokens);
										printf("P%d\n",tokens_reqd);
										printf("number to arrive%d\n",num_of_packets);
										//read_file_input=0;
										}	
										//printf("time enter%lf tokens reqd%d time service%lf\n",packet_inter_arrival_time,tokens_reqd,time_service);
									}
								}
							}
							}
							else
							{
							//printf("only one line present in input\n");
							}
						}
						return NULL;
}
