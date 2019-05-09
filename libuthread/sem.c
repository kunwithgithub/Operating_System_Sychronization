#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	
	int count;
	queue_t waiting;

};

sem_t sem_create(size_t count)
{
	sem_t sem = (sem_t)malloc(sizeof(struct semaphore));
	if (sem == NULL){
		return NULL;
	}
	sem->count = count;
	sem->waiting = queue_create();
	return sem;
}

int sem_destroy(sem_t sem)
{
	if(sem == NULL || (queue_length(sem->waiting) > 0)){
		return -1;
	}
	queue_destroy(sem->waiting);
	free(sem);
	return 0;
}

/*void sem_down(sem)
{
spinlock_acquire(sem->lock);
while (sem->count == 0) {
// Block self 
...
}
sem->count -= 1;
spinlock_release(sem->lock);
}*/

int sem_down(sem_t sem)
{
	if(sem == NULL){
		return -1;
	}
    
    enter_critical_section();
	while (sem->count == 0) {
        pthread_t TID = pthread_self();
        queue_enqueue(waiting, (void*)TID); //
		thread_block();
	}
    sem->count -= 1;
    exit_critical_section();
    
    return 0;
	
}

/*void sem_up(sem)
{
spinlock_acquire(sem->lock);
sem->count += 1;
 //Wake up first in line 
	//(if any) 
...
spinlock_release(sem->lock);
} */

int sem_up(sem_t sem)
{
    if(sem == NULL){
        return -1;
    }
    pthread_t tempTID;
    enter_critical_section();
    if(queue_length(sem->waiting)>0){
        sem->count += 1;
        queue_dequeue(sem->waiting, (void**)&tempTID);
        thread_unblock(tempTID);
    }
    exit_critical_section();
    return 0;
}

/*
 * sem_getvalue - Inspect semaphore's internal state
 * @sem: Semaphore to inspect
 * @sval: Address of data item where value is received
 *
 * If semaphore @sem's internal count is greater than 0, assign internal count
 * to data item pointed by @sval.
 *
 * If semaphore @sems's internal count is equal to 0, assign a negative number
 * whose absolute value is the count of the number of threads currently blocked
 * in sem_down().
 *
 * Return: -1 if @sem or @sval are NULL. 0 if semaphore was successfully
 * inspected.
 */
int sem_getvalue(sem_t sem, int *sval)
{
	if(sem == NULL || sval == NULL){
		return -1;
	}
    
    enter_critical_section();
	if(sem->count > 0){
		sval = sem->count;
	}
	else if (sem->count == 0){
		sval = -1*(queue_length(sem->waiting));
	}
    exit_critical_section();
    
	return 0;
}

