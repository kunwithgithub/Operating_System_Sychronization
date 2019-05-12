/*
1. Thread A creates a TPS
2. Thread B tries to access A's TPS illegally
3. Check that we have a TPS violation error

Outputs should be:
    TPS protection error!
    segmentation fault (core dumped)
*/
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>


void *latest_mmap_addr; // global variable to make address returned by mmap accessible
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
    return latest_mmap_addr;
}

void *thread_A(void*parameter)
{
    /* Create TPS */
    tps_create();
    /* Get TPS page address as allocated via mmap() */
    char *tps_addr = latest_mmap_addr;
    /* Cause an intentional TPS protection error */
    tps_addr[0] = '\0';

    return NULL;
}

int main(){
    
    pthread_t tid;

    /* Init TPS API */
	tps_init(1);

    /* Create thread 1 and wait */
	pthread_create(&tid, NULL, thread_A, NULL);
	pthread_join(tid, NULL);

    return 0;
}
