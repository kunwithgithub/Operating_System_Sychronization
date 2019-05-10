#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

/* TODO: Phase 2 */
queue_t TPSs;

struct TPS{
	pthread_t tid;
	void *privateMemoryPage; 
};

int find_item(void *data, void *arg)
{
    pthread_t tid = (*(pthread_t*)arg);
  
    if (tid == ((struct TPS*)data)->tid)
    {
        return 1;
    }

    return 0;
}

int tps_init(int segv)
{
	/* TODO: Phase 2 */
	TPSs = queue_create();
}

int tps_create(void)
{
	/* TODO: Phase 2 */
	int queueSize = queue_length(TPSs);
	struct TPS *newTPS = (struct TPS*)malloc(sizeof(struct TPS));
	if(newTPS == (void*)-1){
		return -1;
	}
	newTPS->privateMemoryPage = mmap(NULL,TPS_SIZE,PROT_EXEC|PROT_READ|PROT_WRITE,-1,queueSize*TPS_SIZE);
	newTPS->tid = pthread_self();
	queue_enqueue(newTPS);
	return 0;
}


int tps_destroy(void)
{
	/* TODO: Phase 2 */
	phread_t currentTid;
	currentTid = pthread_self();
	struct TPS *currentTPS;
	int success = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentTPS);
	if(currentTPS==NULL||success==-1){
		return -1;
	}
	munmap(currentTPS->privateMemoryPage,TPS_SIZE);
	queue_delete(TPSs,currentTPS);
	free(currentTPS);
	return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
}

int tps_clone(pthread_t tid)
{
	/* TODO: Phase 2 */
	pthread_t currentTid = pthread_self();
	struct TPS *willBeCloned;
	struct TPS *currentThread;
	int sucess = queue_iterate(TPSs,find_item,(void *)tid,(void **)&willBeCloned);
	int anotherSuccess = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThread);
	if(willBeCloned == NULL || success == -1|| anotherSuccess==-1 ||currentThread!=NULL||willBeCloned==NULL){
		return -1;
	}
	struct TPS *newTPS = (struct TPS*)malloc(sizeof(struct TPS));
	newTPS->tid = currentTid;
	newTPS->privateMemoryPage = mmap(NULL,TPS_SIZE,PROT_EXEC|PROT_READ|PROT_WRITE,-1,queueSize*TPS_SIZE);
	memcpy(newTPS->privateMemoryPage,willBeCloned->privateMemoryPage,TPS_SIZE);
	queue_enqueue(newTPS);
	
}
