# ECS150 PROJECT 3 Semaphore and TPS #
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
## Semaphore ##  
For our semaphore implementation, we have a semaphore struct and it contains a  
counter to store number of resources available and a queue to store blocked  
threads.  

```c  
struct semaphore {  
  int count;  
  queue_t waiting;  
 };
``` 

In `sem_up()`, we increment `count` meaning we are freeing a resource, then we check  to see if there are any blocked threads in the `waiting` queue, if so, we unblock the first thread and wake it up.
 In `sem_down()`, we first check if there are no resources available, we block the thread and put it into `waiting` queue. Then we decrement `count` meaning to remove a resource.  
 As for entering and exiting critical sections, we enter a critical section when we want to modify `count` or `waiting` queue, then exit critical section when we are done modifying.
 
In `sem_getvalue()`
 
## Testing Semaphore ##  
We were able to pass all three testers provided by professor.


## TPS Implementation ##

### Data structures ###  
```c
static queue_t TPSs;

struct TPS{
  pthread_t tid;
  struct page *privateMemoryPage; 
};

struct page{
  void *pageAddress; // phase 3
  int referenceNumber;
};
```
1. **struct TPS**: a struct that stores the tid of a thread and a struct pointer that points to *struct page*, the private memory page.
2. **struct page**: a private memory page struct that contains a reference counter to counter the number of threads sharing the same page and a void pointer that 
points to the page address.
3. **queue_t TPSs**: a queue that I used for stores all TPSs information.
reference: Brendan.

### API functions ###
1. **tps_init()**: for initialization of TPS and signal handler, mainly creates the 
*TPSs queue* and initializes signal handler for the signals of type *SIGSEGV*and *SIGBUS*.
Return -1 when *TPSs queue* is already initialized or other initialization failures.

2. **tps_create()**: for the creation of the TPS for the thread that user wants to make a TPS,
uses the current thread's tid,which obtained through function *pthread_self()* and function 
*queue_iterate* to check if the thread already exists a TPS. If no, create a *newTPS* struct
and *privateMemoryPage* struct by function *malloc* and initialize a memory space for page 
by function *mmap()* (protected by *PROT_NONE* for no read and write permission,and uses 
flag *MAP_ANONYMOUS* and *MAP_PRIVATE*), 
then *queue_enqueue* *newTPS* into *TPSs*. If TPS already exists or other creation failure, 
return -1. *enter_critical_section()* and *exit_critical_section()* is used for *mutual exclusion*.

reference: https://stackoverflow.com/questions/34042915/what-is-the-purpose-of-map-anonymous-flag-in-mmap-system-call

3. **tps_destroy()**, for destroying TPS, uses *pthread_self()* to find the tid for current thread,
then uses *queue_iterate()* to find the TPS and destroy and destroy private page memory using *munmap()*,
free TPS struct using *free()*. *enter_critical_section()* and *exit_critical_section()* is used for 
*mutual exclusion*.

4. **tps_read()**: Obtaining tid using *pthread_self()*, *queue_iterate()* for finding the TPS, *mprotect()*
with *PROT_READ* for read permission, and memcpy for read implementation. Return -1 when encountering issues, 
such as out-of-bound and TPS not exists,etc. When finish, *mprotect()* with *PROT_NONE* used to take back the
read permission.

5. **tps_write()**: Obtaining tid using *pthread_self()*, *queue_iterate()* for finding the TPS, *mprotect()*
with *PROC_WRITE* for granting write permission, and *memcpy()* is used for writing. If detected that current
thread's TPS points to a page whose reference count is greater than 1, we need to decrement that page's reference
count, disconnect from it, and create a new page struct for current thread's TPS to hold the information stored in
*buffer*. After finish, *mprotect()* with *PROT_NONE* to take back the permission. Return -1 when encountering 
issues, such as out-of-bound and one of the TPS we need not exists,etc.
*enter_critical_section()* and *exit_critical_section()* is used for *mutual exclusion*.

6. **tps_clone()**: for phase 2, we bascially create a new TPS struct with its own private memory page
and writes data into it from the thread that we need to clone using *memcpy()*. For phase 3,
we bascially just create a newTPS struct for the current thread, and points the *privateMemoryPage* in
current thread's TPS struct to the page struct of the thread that we will clone it for the current thread.
Return -1 when encountering issues, such as the TPS we need to clone not exists or current thread already
has a TPS,etc.
*enter_critical_section()* and *exit_critical_section()* is used for *mutual exclusion*.
