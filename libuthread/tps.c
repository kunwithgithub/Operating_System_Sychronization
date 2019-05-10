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
queue_t tids;

struct TPS{
	pthread_t tid;
};

int tps_init(int segv)
{
	/* TODO: Phase 2 */
	tids = queue_create();
}

int tps_create(void)
{
	/* TODO: Phase 2 */
	int queueSize = queue_length(tids);
	struct TPS *newTPS = (struct TPS*)mmap(NULL,TPS_SIZE,PROT_EXEC|PROT_READ|PROT_WRITE,-1,queueSize*TPS_SIZE); 
	if(newTPS == (void*)-1){
		return -1;
	}
	return 0;
}

int tps_destroy(void)
{
	/* TODO: Phase 2 */
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
}
