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
	struct page *privateMemoryPage; 
};

struct page{
	void *pageAddress; // phase 3
}

int find_item(void *data, void *arg)
{
    pthread_t tid = (*(pthread_t*)arg);
  
    if (tid == ((struct TPS*)data)->tid)
    {
        return 1;
    }

    return 0;
}

int find_fault(void *data, void *arg)
{
	
  
    if (arg == ((struct TPS*)data)->privateMemoryPage)
    {
        return 1;
    }

    return 0;
	
}

static void segv_handler(int sig, siginfo_t *si, void *context)
{
    /*
     * Get the address corresponding to the beginning of the page where the
     * fault occurred
     */
    void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

    /*
     * Iterate through all the TPS areas and find if p_fault matches one of them
     */
	struct TPS *foundTPS;
	int match = queue_iterate(TPSs,find_fault,p_fault,(void **)&foundTPS);
    if (match != -1){
        /* Printf the following error message */
		if(foundTPS != NULL){
			fprintf(stderr, "TPS protection error!\n");
		}
	}
    /* In any case, restore the default signal handlers */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    /* And transmit the signal again in order to cause the program to crash */
    raise(sig);
}

int tps_init(int segv)
{
	/* TODO: Phase 2 */
	if(TPSs != NULL){
		return -1;
	}
	
	TPSs = queue_create();
	
	if(TPSs == NULL){
		return -1;
	}
	if (segv) {
        struct sigaction sa;

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = segv_handler;
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
    }

	return 0;
}

int tps_create(void)
{
	/* TODO: Phase 2 */
	int queueSize = queue_length(TPSs);
	struct TPS *newTPS = (struct TPS*)malloc(TPS_SIZE);
	if(newTPS == (void*)-1){
		return -1;
	}
	
	struct page *newPage = mmap(NULL,sizeof(struct page),PROT_EXEC|PROT_READ|PROT_WRITE,MAP_ANONYMOUS,-1,0);
	
	mprotect(newPage, sizeof(struct page),PROT_NONE);
	
	newTPS->privateMemoryPage = newPage;
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
	munmap(currentTPS->privateMemoryPage,sizeof(struct page));
	queue_delete(TPSs,currentTPS);
	free(currentTPS);
	return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	pthread_t currentTid = pthread_self();
	struct TPS *currentThread;
	int sucess = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThread);
	if(success == -1 || currentThread == NULL||offset+length>TPS_SIZE||buffer == NULL){
		return -1;
	}
	mprotect(currentThread->privateMemoryPage,sizeof(struct page),PROT_READ);
	memcpy((void *)buffer, currentThread+offset,length);
	return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	pthread_t currentTid = pthread_self();
	struct TPS *currentThread;
	int sucess = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThread);
	if(success == -1 || currentThread == NULL||offset+length>TPS_SIZE||buffer == NULL){
		return -1;
	}
	mprotect(currentThread->privateMemoryPage, sizeof(struct page),PROT_WRITE);
	memcpy(currentThread+offset,(void *)buffer,length);
	
	return 0;
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
	int queueSize = queue_length(TPSs);
	newTPS->tid = currentTid;
	newTPS->privateMemoryPage = mmap(NULL,sizeof(struct page),PROT_EXEC|PROT_READ|PROT_WRITE,MAP_ANONYMOUS,-1,0);
	memcpy(newTPS,willBeCloned,TPS_SIZE);
	queue_enqueue(newTPS);
	return 0;
}
