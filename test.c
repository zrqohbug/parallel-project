#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"
#include <time.h>
#include <pthread.h>
#include "sys/time.h"
#define BILLION 1000000000L
#define Size 8
#define MAX 24
#define TYPE int
#define max(A,B) A > B?A:B
int n;
int p;

// 2D array to evade cache line invalidation at the same time
int count[MAX][2];
// each thread own its local max
int maxV[MAX][1];
// main thread check the whole work has finished or not
int Get[MAX][MAX];
// each thread own one line in FIN
int FIN[MAX][MAX];
int halfn;
int tt;

pthread_barrier_t mybarrier;
pthread_mutex_t lock; 
pthread_cond_t condP;


typedef struct {
    int pid;
    int start;
    int end;
} GM;

/* 
 * Arg for nQueens: for pre thread
 * t              : current row
 * k              : current col
 * checkrow       : bitmap for row
 * checkdig1      : bitmap for dig1
 * checkdig2      : bitmap for dig2
 * pre            : local prev history (pre thread has its own history)
 */

void nQueens(int t, int k, TYPE checkrow, TYPE checkdig1, TYPE checkdig2, int* pre, int i_start, int i_end)                 
{
    int i;
    for(i = i_start ; i < i_end ; i++)
    {
        if(((checkrow >> i) & 1) || ((checkdig1 >> (t + i)) & 1) || ((checkdig2 >> (t - i)) & 1))       // check bitmap have occupied or not
        {
            continue;
        }
        // move bitmap
        TYPE temprow = checkrow | (1 << i);
        TYPE tempdig1 = checkdig1 | (1 << (t + i));                 
        TYPE tempdig2 = checkdig2 | (1 << (t - i));
        pre[t] = i;
        if(t == n - 1)
        {
            count[k][0]++;
            int maxR = 0;
            int j;
            for(j = 1; j < 15; j++)
                maxR = maxR + (j) * ((tempdig2 >> j) & 1);
            for(j = -1; j > -15; j--)
                maxR = maxR + (-j) * ((tempdig2 >> j) & 1);
            if(maxV[k][0] < maxR)
            {
                maxV[k][0] = maxR;
                for(j = 0 ; j < n; j++){
                    FIN[k][j] = pre[j];              
                    //printf(" %d ", pre[j]);
                }
            }
            break;
        }
        else
        {
            nQueens(t + 1, k, temprow, tempdig1, tempdig2, pre, 0 , n);
        }
    }
    return;
}

void *firstLine(void *varg) {
    GM *arg = (GM*)varg;
    register int i, j, round, k, l;
    struct timeval t0, t1, dt;
    int pid, start, end;
    pid = arg->pid;
    start = arg->start;
    end = arg->end;
    TYPE checkrow = 0;
    TYPE checkdig1 = 0;
    TYPE checkdig2 = 0; 
    int p = pid;
    if(pid >= halfn)
        p = pid - tt;
    checkrow = (1 << p);
    checkdig1 = (1 << (+ p));
    checkdig2 = (1 << (- p));
    int pre[16];
    pre[0] = p;
    gettimeofday(&t0, NULL);
    nQueens(1, pid, checkrow, checkdig1, checkdig2, pre, start , end);
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &dt);
    fprintf(stderr, "doSomeThing (thread %ld = %d executing %d) took %d.%06d sec\n",
        (long)pthread_self(), pid, p, dt.tv_sec, dt.tv_usec);
    Get[pid][0] = 1;
    return NULL;
}


int main(int argc, char **argv) {
    struct timespec start, end_schedule, end_init, end_compute, end_finish;
    
    int i, j, k;
    int newn, on;
    double time;
    double time_total, time_schedule, time_init, time_compute, time_finish;
    if(argc != 3) {
        printf("Usage: nqueens problem n p\nAborting...\n");
        exit(0);
    }
    n = atoi(argv[1]);      // n -> nqueens
    p = atoi(argv[2]);      // p -> threadnumber;
    halfn = (n + 1) / 2;
    // if p > halfn we need to dynamic parallel inside subfunction;
    // if p < halfn we create thread pool                               
    clock_gettime(CLOCK_MONOTONIC, &start);      
    if (tpool_create(p) != 0) {
        printf("tpool_create failed\n");
        exit(1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_init);     
    /* 
     * schedule
     */ 
    // dynamic balance
    tt = 0;
    if(p > halfn && n > 12){
        tt = (p - halfn) > halfn ? halfn : p - halfn;
        for(i = 0; i < halfn - tt; ++i)
        {
            GM *arg = (GM*)malloc(sizeof(*arg));
            arg->pid = i;               // thread pid
            arg->start = 0;             // thread start
            arg->end = n;               // thread end
            tpool_add_work(firstLine, (void*)arg);        
        }
        for(i = halfn - tt; i < halfn ; i++)
        {
            GM *arg = (GM*)malloc(sizeof(*arg));
            arg->pid = i;               // thread pid
            arg->start = 0;             // thread start
            arg->end = n / 2;           // thread end
            tpool_add_work(firstLine, (void*)arg);
            GM *arg2 = (GM*)malloc(sizeof(*arg2));
            arg2->pid = i + tt;       
            arg2->start = n / 2;         // thread start
            arg2->end = n;               // thread end
            tpool_add_work(firstLine, (void*)arg2);
        }
    }
    else{

        for (i = 0; i < halfn; ++i) {
            GM *arg = (GM*)malloc(sizeof(*arg));
            arg->pid = i;               // thread pid
            arg->start = 0;             // thread start
            arg->end = n;               // thread end
            tpool_add_work(firstLine, (void*)arg);
        }
    }
   clock_gettime(CLOCK_MONOTONIC, &end_schedule);     
    
    // main thread waiting for all thread finished
    int pre_i = 0;
    int checkn = halfn + tt;
    while(1)
    {
        for(i = pre_i; i < checkn; i++)
            if (Get[i][0] == 0)
            {
                break;
            }
            else
            {
                pre_i = i;
            }
        if(i == checkn)
            break;
    }
    clock_gettime(CLOCK_MONOTONIC, &end_compute);
    int sum = 0;
    int maxS = 0;
    int PRINT[MAX];
    for(i = halfn - tt ; i < halfn; i++)
    {
        if(i == halfn  && n % 2 == 1)
            break;
        int j = i + tt;
        count[i][0] += count[j][0];
        if(maxV[i][0] < maxV[j][0])
        {
            maxV[i][0] = maxV[j][0];
            for(k = 0 ; k < n ; k++)
            {
                maxV[i][k] = maxV[j][k];
                //printf(" %d ", PRINT[j]);
            }
        }
    }
    for(i = 0; i < n; i++){
        int i2 = (i >= halfn) ? n - i - 1  : i;
        sum = sum + count[i2][0];
        if(maxS < maxV[i2][0])
        {
            maxS = maxV[i2][0];
            for(j = 0 ; j < n ; j++)
            {
                PRINT[j] = FIN[i2][j];
                //printf(" %d ", PRINT[j]);
            }           
            //printf("\n");
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_finish);  
    if(sum > 0)
    {
        char P[MAX][MAX] = {0};
        for(i = 0; i < n; i++)
        {
            P[i][PRINT[i]] = 'Q' - '.';
        }
        for(i = 0; i < n; i++)
        {
            for(j = 0 ; j < n; j++)
                printf("%c ",P[i][j] + '.');
            printf("\n");
        }   
    }

    time_init    = BILLION * (end_init.tv_sec - start.tv_sec) + (end_init.tv_nsec - start.tv_nsec);
    time_schedule   = BILLION * (end_schedule.tv_sec - end_init.tv_sec) + (end_schedule.tv_nsec - end_init.tv_nsec);
    time_total   = BILLION * (end_finish.tv_sec - start.tv_sec) + (end_finish.tv_nsec - start.tv_nsec);
    time_init    = BILLION * (end_init.tv_sec - start.tv_sec) + (end_init.tv_nsec - start.tv_nsec);
    time_compute = BILLION * (end_compute.tv_sec - end_init.tv_sec) + (end_compute.tv_nsec - end_init.tv_nsec);
    time_finish  = BILLION * (end_finish.tv_sec - end_compute.tv_sec) + (end_finish.tv_nsec - end_compute.tv_nsec);
    time_total   = time_total   / BILLION;
    time_init    = time_init    / BILLION;
    time_compute = time_compute / BILLION;
    time_finish  = time_finish  / BILLION;
    time_schedule  = time_schedule  / BILLION;    

    printf("n = %d, p = %d, total: %lf seconds\n",   n, p, time_total);
    printf("n = %d, p = %d, init: %lf seconds\n",    n, p, time_init);
    printf("n = %d, p = %d, schedule: %lf seconds\n",  n, p, time_schedule);
    printf("n = %d, p = %d, compute: %lf seconds\n", n, p, time_compute);
    printf("n = %d, p = %d, finish: %lf seconds\n",  n, p, time_finish);

        
    printf("available result number is %d\n", sum);
    printf("max result number is %d\n", maxS);
    printf("\n");
    return 0;
}
