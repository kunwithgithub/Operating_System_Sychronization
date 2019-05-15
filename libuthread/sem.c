#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

//semaphore struct which contains a counter to store number of resources
//and a waiting queue to store blocked threads
struct semaphore {
  
  int count;
  queue_t waiting;
  
};

//initialize the semaphore
sem_t sem_create(size_t count)
{
  sem_t sem = (sem_t)malloc(sizeof(struct semaphore));
  if (sem == NULL){
    return NULL;
  }
  enter_critical_section();
  sem->count = count;
  sem->waiting = queue_create();
  exit_critical_section();
  return sem;
}

int sem_destroy(sem_t sem)
{
  enter_critical_section();
  
  //if no more threads in waiting queue, we can then destroy the semaphore
  if(sem == NULL || (queue_length(sem->waiting) > 0)){
    return -1;
  }
  queue_destroy(sem->waiting);
  free(sem);
  exit_critical_section();
  return 0;
}


int sem_down(sem_t sem)
{
  if(sem == NULL){
    return -1;
  }
  
  enter_critical_section();
  
  //block threads into waiting queue if no more resources available
  while (sem->count == 0) {
    pthread_t TID = pthread_self();
    queue_enqueue(sem->waiting, (void*)TID); //
    thread_block();
  }
  sem->count -= 1;
  exit_critical_section();
  
  return 0;
  
}


int sem_up(sem_t sem)
{
  if(sem == NULL){
    return -1;
  }
  
  enter_critical_section();
  
  //free a resource
  sem->count += 1;
  
  //unblock the first thread in waiting queue since we have resource available
  if(queue_length(sem->waiting)>0){
    
    pthread_t tempTID;  
    queue_dequeue(sem->waiting, (void**)&tempTID);
    thread_unblock(tempTID);
    
  }
  
  exit_critical_section();
  return 0;
}

int sem_getvalue(sem_t sem, int *sval)
{
  if(sem == NULL || sval == NULL){
    return -1;
  }
  
  enter_critical_section();
  
  //if we have resources available, store count in *sval
  if(sem->count > 0){
    *sval = sem->count;
  }
  
  //if not, store number of blocked threads instead
  else if (sem->count == 0){
    *sval = -1*(queue_length(sem->waiting));
  }
  exit_critical_section();
  
  return 0;
}

