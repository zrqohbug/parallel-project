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

int count[MAX][2];
int maxV[MAX][1];
//int Get;
int Get[MAX][MAX];
int FIN[MAX][MAX];

pthread_barrier_t mybarrier;
pthread_mutex_t lock; 
pthread_cond_t condP;
void *func(void *arg)
{
    printf("thread %d\n", (int)arg);
    return NULL;
}


typedef struct {
    int pid;
} GM;

void nQueens(int t, int k, TYPE checkrow, TYPE checkdig1, TYPE checkdig2, int* pre)
{
    int i;
    for(i = 0 ; i < n ; i++)
    {
        if(((checkrow >> i) & 1) || ((checkdig1 >> (t + i)) & 1) || ((checkdig2 >> (t - i)) & 1))
        {
            continue;
        }
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
                //printf("%d\n", maxR);
                //printf("%x || %x \n", tempdig1, tempdig2);
            }
            break;
        }
        else
        {
            nQueens(t + 1, k, temprow, tempdig1, tempdig2, pre);
        }
    }
    return;
}

void *firstLine(void *varg) {
    GM *arg = (GM*)varg;
    register int i, j, start, end, round, k, l;
    struct timeval t0, t1, dt;
    int pid, block;
    pid = arg->pid;
    block = n / (p * Size);
    start = 0;
    end = p - 1;
    TYPE checkrow = 0;
    TYPE checkdig1 = 0;
    TYPE checkdig2 = 0; 
    int p = pid;
    checkrow = (1 << p);
    checkdig1 = (1 << (+ p));
    checkdig2 = (1 << (- p));
    int pre[16];
    pre[0] = p;
    gettimeofday(&t0, NULL);
    nQueens(1, p, checkrow, checkdig1, checkdig2, pre);
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &dt);
    //fprintf(stderr, "doSomeThing (thread %ld = %d) took %d.%06d sec\n",
    //    (long)pthread_self(), pid, dt.tv_sec, dt.tv_usec);
    //Get = Get | (1 << p);
    Get[p][0] = 1;
    return NULL;
}


int main(int argc, char **argv) {
    struct timespec start, end;
    int i, j;
    int newn, on;
    double time;
    if(argc != 3) {
        printf("Usage: nqueens problem n p\nAborting...\n");
        exit(0);
    }
    n = atoi(argv[1]);      // n -> nqueens
    p = atoi(argv[2]);      // p -> threadnumber;
    int halfn = (n + 1) / 2;
    // if p > halfn we need to dynamic parallel inside subfunction;
    // if p < halfn we create thread pool                               


    /*
    pthread_t *threads = (pthread_t*)malloc(p * sizeof(threads));
    for(i = 0; i < p; i++) {
        GM *arg = (GM*)malloc(sizeof(*arg));
        arg->pid = i;               // thread pid
        pthread_create(&threads[i], NULL, firstLine, arg);
    }
    for(i = 0; i < p; i++)
        pthread_join(threads[i], NULL);
    free(threads);
    */
    
    if (tpool_create(p) != 0) {
        printf("tpool_create failed\n");
        exit(1);
    }
    clock_gettime(CLOCK_MONOTONIC, &start);    
    for (i = 0; i < halfn; ++i) {
        GM *arg = (GM*)malloc(sizeof(*arg));
        arg->pid = i;               // thread pid
        tpool_add_work(firstLine, (void*)arg);
    }
    int pre_i = 0;
    while(1)
    {
        for(i = pre_i; i < halfn; i++)
            if (Get[i][0] == 0)
            {
                break;
            }
            else
            {
                pre_i = i;
            }
        if(i == halfn)
            break;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    tpool_destroy();
    time = BILLION *(end.tv_sec - start.tv_sec) +(end.tv_nsec - start.tv_nsec);
    time = time / BILLION;
    printf("Elapsed time: %lf seconds (n = %d, p = %d)\n", time, n, p);
    int sum = 0;
    int maxS = 0;
    int PRINT[MAX];
    for(i = 0; i < n; i++){
        int i2 = (i > halfn) ? n - i : i;
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
    printf("available result number is %d\n", sum);
    printf("max result number is %d\n", maxS);
    printf("\n");
    return 0;
}
