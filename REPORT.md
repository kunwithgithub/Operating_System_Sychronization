# ECS150 PROJECT 3 Semaphore and TPS #  
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
In `sem_up()`, we increment `count` meaning we are freeing a resource, then we   
check to see if there are any blocked threads in the `waiting` queue, if so, we   
unblock the first thread and wake it up.  

In `sem_down()`, we first check if there are no resources available, we block   
the thread and put it into `waiting` queue. Then we decrement `count` meaning to  
remove a resource.  

As for entering and exiting critical sections, we enter a critical section when   
we want to modify `count` or `waiting` queue, then exit critical section when we   
are done modifying.

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
1. **struct TPS**: a struct that stores the tid of a thread and a struct pointer   
that points to *struct page*, the private memory page.
2. **struct page**: a private memory page struct that contains a reference   
counter to counter the number of threads sharing the same page and a void   
pointer that points to the page address.
3. **queue_t TPSs**: a queue to store all the thread private storages(`TPSs`)  
reference: Brendan.

### API functions ###  
**tps_init()**  
For initialization of TPS and signal handler, we mainly create the `TPSs` queue  
and initializes signal handler for the signals of type *SIGSEGV* and *SIGBUS* to  
detect tps protection errors and seg faults. We only let caller call tps_init()   
once by checking if `TPSs` queue is already created or not.
  
**tps_create()**  
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

**tps_destroy()**  
For destroying TPS, we use the current tid and iterate through our `TPSs` queue  
to find the TPS. If found, we need to look at it's `referenceNumber`, if its  
sharing a page with aother thread, we decrement the reference counter and then  
delete and free the TPS.  
If reference counter = 1, we destroy its private page memory using `munmap()`,  
delete the TPS from the queue and lastly free the TPS struct.  

**tps_read()**  
Again, we need to iterate TPSs queue to find the TPS to read from.  
If found, we use `mprotect()` to give the calling thread read permission of TPS  
by using *PROT_READ*, then we `memcpy()` TPS's data into `buffer`.  
This function returns -1 when encountering issues, such as out-of-bound, TPS not  
exists, did not find TPS, etc.  
When finish reading, need to use `mprotect()` with *PROT_NONE* to take back the  
read permission.  

**tps_write()**  
We first iterate TPSs queue to find the current TPS to write to.  
If found, we use `mprotect()` to give the calling thread write permission of TPS  
by using *PROT_Write*.  
If detected that current thread's TPS points to a page whose reference count is  
greater than 1, that is, if the current thread is sharing a page with another   
thread, we then need to create a new memory page with `mmap()` and `memcpy()`and  
copy the content from the original memory page. Then we decrement the reference  
counter since page is not sharing with another thread anymore.  
After this, we use `mprotect()` with *PROT_NONE* to take back the permission.  
Then we give the new memory page to the current thread's TPS and give it write  
permission.  
Lastly, we write `buffer` into the page and take back writing permission.  

**tps_clone()**  
For phase 2, we basically create a new TPS struct with its own private memory  
page and writes data into it from the thread that we need to clone using   
`memcpy()`.  
For phase 3, we added a page struct to contain the page address so that multiple  
TPSs can point to the same page structure.  
We create `currentThread` TPS for the calling thread, and `willBeCloned` TPS  
which stores the TPS calling thread wants to clone.  
Then we just clone `willBeCloned`'s private memeory page into current thread's  
page. Also, we increment the reference counter since calling thread is sharing  
a page with willBeCloned. Lastly, we enqueue the current TPS into `TPSs` queue.  
This function returns -1 when the TPS we need to clone does not exist or current  
thread already has a TPS, etc.  

## Critical Sections ##
We use `enter_critical_section()` and `exit_critical_section()` for mutual  
exclusion. We enter mutual exclusion before we modify TPS's, queue, or perform  
mmap(), memcpy(), mprotect() operations and etc. We exit critical sections  
before everytime we return 0 or -1.

## Testing ##  
We were able to pass all three semaphore testers provided by professor.  

For TPS testing, we were also able to pass the tps.c tester given by professor.  
We also created two additional testers for testing tps protection errors and  
seg fault, and for testing more complex cases of TPS and corner cases.  

### tps_protection.c ###  
In this tester, we basically followed professor's slides to test for tps  
protection error by letting same thread accesses its own tps illegally and we  
were also able to get the expected outputs: 
```c
TPS protection error!  
segmentation fault (core dumped)
```  

### tps_testComplex.c ###  
In this tester, we have 23 test cases which test all corner cases such as:  
* destorying a tps or creating a tps before tps_init() is called  
* cloning a tps before creating a tps  
* reading or writing when buffer is null, tps is not created, and offsets are  
invalid  
* calling tps_init() more than once  
* calling tps_create() more than once, etc.  

