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

//queue to store thread private storages
static queue_t TPSs;

struct TPS{
	pthread_t tid;
	struct page *privateMemoryPage; 
};

struct page{
	void *pageAddress; // phase 3
	int referenceNumber;
};

//function to find tps with matching tid
int find_item(void *data, void *arg)
{
    pthread_t tid = (*(pthread_t*)arg);

	pthread_t idNeedToFind = ((struct TPS*)data)->tid;
    if ( idNeedToFind == tid)
    {	
        return 1;
    }

    return 0;
}

//function to find tps with matching pageAddress
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
	//tps_init can only be called once
	if(TPSs != NULL){
		return -1;
	}
	
	TPSs = queue_create();
	
	if(TPSs == NULL){
		return -1;
	}

	//page fault handler to detect TPS protection error and seg fault
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
		
	pthread_t currentTid;
	currentTid = pthread_self();
	enter_critical_section();
	
	//if TPS already existed for calling thread, error
	struct TPS *currentThread = NULL;
	queue_iterate(TPSs,find_item,(void *)&currentTid,(void **)&currentThread);
	if(currentThread!= NULL || TPSs == NULL){
		exit_critical_section();
		return -1;
	}

	//create new TPS
	struct TPS *newTPS = (struct TPS*)malloc(TPS_SIZE);
	if(newTPS == NULL){
		exit_critical_section();
		return -1;
	}

	//initialize memory page
	void* newPage = mmap(NULL,TPS_SIZE,PROT_NONE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
	if(newPage == (void*)-1){
		exit_critical_section();
		return -1;
	}
	
	newTPS->privateMemoryPage = (struct page*)malloc(sizeof(struct page));	
	newTPS->privateMemoryPage->pageAddress = newPage;
	newTPS->privateMemoryPage->referenceNumber = 1;
	newTPS->tid = currentTid;
	queue_enqueue(TPSs,(void*)newTPS);

	exit_critical_section();
	return 0;
}


int tps_destroy(void)
{
	enter_critical_section();
	pthread_t currentTid;
	currentTid = pthread_self();

	//find the current TPS to destroy
	struct TPS *currentTPS = NULL;
	int success = queue_iterate(TPSs,find_item,(void *)&currentTid,(void **)&currentTPS);
	if(currentTPS == NULL || success == -1){
		exit_critical_section();
		return -1;
	}

	//remove mapping of memory page and delete and free TPS from queue
	munmap(currentTPS->privateMemoryPage->pageAddress,TPS_SIZE);
	queue_delete(TPSs,currentTPS);
	free(currentTPS);

	exit_critical_section();
	return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	pthread_t currentTid = pthread_self();
	struct TPS *currentThread = NULL;

	//find current TPS to read from
	int success = queue_iterate(TPSs,find_item,(void *)&currentTid,(void **)&currentThread);
	if(success == -1 || currentThread == NULL || offset+length > TPS_SIZE || buffer == NULL){
		exit_critical_section();
		return -1;
	}

	//TPS gives reading permission and buffer receives the data
	mprotect(currentThread->privateMemoryPage->pageAddress,TPS_SIZE,PROT_READ);
	memcpy((void *)buffer, currentThread->privateMemoryPage->pageAddress+offset,length);

	//after reading, change back to no permission granted
	mprotect(currentThread->privateMemoryPage->pageAddress,TPS_SIZE,PROT_NONE);
	return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	
	enter_critical_section();
	pthread_t currentTid = pthread_self();
	struct TPS *currentThreadTPS = NULL;

	//find current TPS to write to
	int success = queue_iterate(TPSs,find_item,(void *)currentTid,(void **)&currentThreadTPS);
	if(success == -1 || currentThreadTPS->privateMemoryPage == NULL || offset+length > TPS_SIZE || buffer == NULL){
		exit_critical_section();
		return -1;
	}

	//give writing permission
	mprotect(currentThreadTPS->privateMemoryPage->pageAddress,TPS_SIZE,PROT_WRITE);

	//if current thread is sharing a page with another thread
	if(currentThreadTPS->privateMemoryPage->referenceNumber>1){

		//create a new memory page and copy the content from the original memory page.
		void *newPage = mmap(NULL,TPS_SIZE,PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
		if(newPage == (void *)-1){
			exit_critical_section();
			return -1;
		}
		memcpy(newPage,currentThreadTPS->privateMemoryPage->pageAddress,TPS_SIZE);

		//page not sharing with another thread anymore
		currentThreadTPS->privateMemoryPage->referenceNumber--;
		mprotect(currentThreadTPS->privateMemoryPage->pageAddress,TPS_SIZE ,PROT_NONE);

		struct page *newForcurrent = (struct page*)malloc(sizeof(struct page));
		if (newForcurrent == NULL){
			exit_critical_section();
			return -1;
		}

		currentThreadTPS->privateMemoryPage = newForcurrent;
		currentThreadTPS->privateMemoryPage->pageAddress = newPage;
		//give writing permission
		mprotect(currentThreadTPS->privateMemoryPage->pageAddress, TPS_SIZE,PROT_WRITE);
	}
	
	//write buffer into TPS 
	memcpy(currentThreadTPS->privateMemoryPage->pageAddress+offset,buffer,length);

	//after writing, change back to no permission
	mprotect(currentThreadTPS->privateMemoryPage->pageAddress, TPS_SIZE,PROT_NONE);
	exit_critical_section();
	return 0;
}

int tps_clone(pthread_t tid)
{
	if(TPSs == NULL){
		return -1;
	}

	enter_critical_section();
	pthread_t currentTid = pthread_self();

	struct TPS *willBeCloned = NULL; //TPS to be cloned
	struct TPS *currentThread = NULL; //calling thread's TPS

	//find the TPS that calling thread wants to clone
	queue_iterate(TPSs,find_item,(void *)&tid,(void **)&willBeCloned);

	//find the TPS of the calling thread
	queue_iterate(TPSs,find_item,(void *)&currentTid,(void **)&currentThread);

	//if could not find TPS to be cloned or calling thread already has a TPS
	if(willBeCloned == NULL || currentThread != NULL){
		exit_critical_section();
		return -1;
	}
	
	//create a new TPS for cloning
	struct TPS *newTPS = (struct TPS*)malloc(sizeof(struct TPS));
	if(newTPS == NULL){
		exit_critical_section();
		return -1;
	}

	newTPS->tid = currentTid;
	currentThread = newTPS;

	//clone memory page
	newTPS->privateMemoryPage = willBeCloned->privateMemoryPage;

	//current thread is sharing a page with TPS to be cloned
	currentThread->privateMemoryPage->referenceNumber++; //phase 3
	queue_enqueue(TPSs, (void *)newTPS);

	exit_critical_section();
	return 0;
}
