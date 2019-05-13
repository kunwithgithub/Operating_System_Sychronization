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
	int referenceNumber;
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

int find_fault(void *data, void *arg)
{
	
  
    if (arg == ((struct TPS*)data)->privateMemoryPage->pageAddress)
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
	pthread_t currentTid;
	currentTid = pthread_self();
	enter_critical_section();
	struct TPS *currentThread = NULL;
	queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThread);
	if(currentThread!= NULL){
		printf("return -1 and \n");
		return -1;
	}
	struct TPS *newTPS = (struct TPS*)malloc(TPS_SIZE);
	if(newTPS == NULL){
		return -1;
	}
	void* newPage = mmap(NULL,TPS_SIZE,PROT_NONE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
	if(newPage == (void*)-1){
		return -1;
	}else{
//		printf("mmap success! \n");
	}
	newTPS->privateMemoryPage = (struct page*)malloc(sizeof(struct page));	
	newTPS->privateMemoryPage->pageAddress = newPage;
	newTPS->privateMemoryPage->referenceNumber = 1;
	newTPS->tid = pthread_self();
	queue_enqueue(TPSs,(void*)newTPS);
	exit_critical_section();
	return 0;
}


int tps_destroy(void)
{
	/* TODO: Phase 2 */
	pthread_t currentTid;
	currentTid = pthread_self();
	struct TPS *currentTPS = NULL;
	int success = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentTPS);
	if(currentTPS==NULL||success==-1){
		return -1;
	}
	munmap(currentTPS->privateMemoryPage->pageAddress,TPS_SIZE);
	queue_delete(TPSs,currentTPS);
	free(currentTPS);
	return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	pthread_t currentTid = pthread_self();
	struct TPS *currentThread = NULL;
	int success = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThread);
	if(success == -1 || currentThread == NULL||offset+length>TPS_SIZE||buffer == NULL){
		return -1;
	}
	mprotect(currentThread->privateMemoryPage->pageAddress,TPS_SIZE,PROT_READ);
//	fprintf(stderr,"currentThread %d \n",currentThread->privateMemoryPage->referenceNumber);
	memcpy((void *)buffer, currentThread->privateMemoryPage->pageAddress+offset,length);
	mprotect(currentThread->privateMemoryPage->pageAddress,TPS_SIZE,PROT_NONE);
	return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	pthread_t currentTid = pthread_self();
	struct TPS *currentThreadTPS = NULL;
	int success = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThreadTPS);
	if(success == -1 || currentThreadTPS->privateMemoryPage == NULL||offset+length>TPS_SIZE||buffer == NULL){
		return -1;
	}
//	printf("success is %d referenceNumber = %d \n",success,currentThreadTPS->privateMemoryPage->referenceNumber);
	mprotect(currentThreadTPS->privateMemoryPage->pageAddress,TPS_SIZE,PROT_WRITE);
	if(currentThreadTPS->privateMemoryPage->referenceNumber>1){
		void *newPage = mmap(NULL,TPS_SIZE,PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
		if(newPage == (void *)-1){
			fprintf(stderr,"newPage fail!\n");
		}
		memcpy(newPage,currentThreadTPS->privateMemoryPage->pageAddress,TPS_SIZE);
		currentThreadTPS->privateMemoryPage->referenceNumber--;
		mprotect(currentThreadTPS->privateMemoryPage->pageAddress,TPS_SIZE ,PROT_NONE);
		struct page *newForcurrent = (struct page*)malloc(sizeof(struct page));
		currentThreadTPS->privateMemoryPage = newForcurrent;
		currentThreadTPS->privateMemoryPage->pageAddress = newPage;
		mprotect(currentThreadTPS->privateMemoryPage->pageAddress, TPS_SIZE,PROT_WRITE);
	}
	
	memcpy(currentThreadTPS->privateMemoryPage->pageAddress+offset,buffer,length);
	mprotect(currentThreadTPS->privateMemoryPage->pageAddress, TPS_SIZE,PROT_NONE);
	exit_critical_section();
	return 0;
}

int tps_clone(pthread_t tid)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	pthread_t currentTid = pthread_self();
	struct TPS *willBeCloned = NULL;
	struct TPS *currentThread = NULL;
	int success = queue_iterate(TPSs,find_item,(void *)tid,(void **)&willBeCloned);
	int anotherSuccess = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThread);
//	fprintf(stderr,"success is %d and anotherSuccess is %d tid is %ld and currentTid is %ld\n",success,anotherSuccess,tid,currentTid);
	if(willBeCloned == NULL || success == -1|| anotherSuccess==-1 ||currentThread!=NULL){
		return -1;
	}
	/*	phase 2
	struct TPS *newTPS = (struct TPS*)malloc(sizeof(struct TPS));
	newTPS->tid = currentTid;
	
	newTPS->privateMemoryPage = (struct page*)malloc(sizeof(struct page);
	newTPS->privateMemoryPage->pageAddress = mmap(NULL,sizeof(struct page),PROT_EXEC|PROT_READ|PROT_WRITE,MAP_ANONYMOUS,-1,0);
	memcpy(newTPS,willBeCloned,TPS_SIZE);
	*/
	/* phase 3 */
	struct TPS *newTPS = (struct TPS*)malloc(sizeof(struct TPS));
	newTPS->tid = currentTid;
	currentThread = newTPS;
	newTPS->privateMemoryPage = willBeCloned->privateMemoryPage;
	currentThread->privateMemoryPage->referenceNumber++; //phase 3
	queue_enqueue(TPSs,(void *)newTPS);
//	fprintf(stderr,"currentThread in clone %d and willBeCloned is %d \n",currentThread->privateMemoryPage->referenceNumber, willBeCloned->privateMemoryPage->referenceNumber);
	exit_critical_section();
	return 0;
}
