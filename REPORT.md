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
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
In `sem_up()`, we increment `count` meaning we are freeing a resource, then we   
check to see if there are any blocked threads in the `waiting` queue, if so, we   
unblock the first thread and wake it up.  

In `sem_down()`, we first check if there are no resources available, we block   
the thread and put it into `waiting` queue. Then we decrement `count` meaning to  
remove a resource.  

As for entering and exiting critical sections, we enter a critical section when   
we want to modify `count` or `waiting` queue, then exit critical section when we   
are done modifying.
 
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
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
1. **struct TPS**: a struct that stores the tid of a thread and a struct pointer   
that points to *struct page*, the private memory page.
2. **struct page**: a private memory page struct that contains a reference   
counter to counter the number of threads sharing the same page and a void   
pointer that points to the page address.
3. **queue_t TPSs**: a queue to store all the thread private storages(`TPSs`)
reference: Brendan.

### API functions ###
1. **tps_init()** 
For initialization of TPS and signal handler, we mainly create the `TPSs` queue  
and initializes signal handler for the signals of type *SIGSEGV* and *SIGBUS* to  
detect tps protection errors and seg faults. We only let caller call tps_init()   
once by checking if `TPSs` queue is already created or not.

AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
2. **tps_create()**
To create a TPS for the calling thread, we need to get the current thread's tid  
through function `pthread_self()` and pass it to `queue_iterate()` to check if   
the current thread already have a TPS. 
If TPS already exists or there are other creation failures, then we return -1.
If not, create a `newTPS` struct and `privateMemoryPage` struct using `malloc()`   
and initialize a  memory space for page using `mmap()` (protected by *PROT_NONE*   
for no read and write permission, and uses flag *MAP_ANONYMOUS* and   
*MAP_PRIVATE*), then we enqueue the `newTPS` to our `TPSs` queue which stores   
all tps's. 

reference: https://stackoverflow.com/questions/34042915/what-is-the-purpose-of-  
map-anonymous-flag-in-mmap-system-call

3. **tps_destroy()**  
For destroying TPS, we use the current tid and iterate through our `TPSs` queue  
to find the TPS. If found, we destroy its private page memory using `munmap()`,
delete the TPS from the queue and free the TPS struct. 

4. **tps_read()**
Again, we need to iterate TPSs queue to find the TPS to read from.
If found, we use `mprotect()` to give the calling thread read permission of TPS
by using *PROT_READ*, then we `memcpy()` TPS's data to `buffer`. 
This function returns -1 when encountering issues, such as out-of-bound, TPS not   
exists,etc. 
When finish reading, need to use `mprotect()` with *PROT_NONE* to take back the
read permission.

AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
5. **tps_write()**  
First iterate TPSs queue to find the TPS to write to.
If found, we use `mprotect()` to give the calling thread write permission of TPS
by using *PROT_Write*,
If detected that current thread's TPS points to a page whose reference count is greater than 1, we need to decrement that page's reference
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


We use `enter_critical_section()` and `exit_critical_section()` is used for mutual exclusion.
