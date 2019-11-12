#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__
 
#include <pthread.h>
  

typedef struct tpool_work {
     void*               (*routine)(void*);       // current work function
     void                *arg;                    // current work arg
     struct tpool_work   *next;                   // linked list to the next work 
}tpool_work_t;
  
typedef struct tpool {
     int             shutdown;                    
     int             max_thr_num;                
     pthread_t       *thr_id;                    
     tpool_work_t    *queue_head;                
     pthread_mutex_t queue_lock;                    
     pthread_cond_t  queue_ready;    
 }tpool_t;

int tpool_create(int max_thr_num);

void tpool_destroy();

int tpool_add_work(void*(*routine)(void*), void *arg);

#endif
