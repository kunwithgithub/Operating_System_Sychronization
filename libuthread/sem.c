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
  enter_critical_section();
  sem->count = count;
  sem->waiting = queue_create();
  exit_critical_section();
  return sem;
}

int sem_destroy(sem_t sem)
{
  enter_critical_section();
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
  sem->count += 1;
  
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
  if(sem->count > 0){
    *sval = sem->count;
  }
  else if (sem->count == 0){
    *sval = -1*(queue_length(sem->waiting));
  }
  exit_critical_section();
  
  return 0;
}

