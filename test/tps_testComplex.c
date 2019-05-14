#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tps.h>
#include <sem.h>
static sem_t sem1,sem2;
static char msg1[TPS_SIZE] = "Hello world 1!\n";
static char msg2[TPS_SIZE] = "hello world 2!\n";

void thread_B(pthread_t tid){
	assert(tps_clone(tid)==0);
	printf("OK - Thread B clones success\n");
	char *buffer = malloc(TPS_SIZE);
	memset(buffer, 0, TPS_SIZE);
	assert(tps_read(0, TPS_SIZE, buffer)==0);
	assert(!memcmp(msg1, buffer, TPS_SIZE));
	printf("OK - Thread B reads from itself success\n");

	assert(tps_destroy()==0);
	printf("OK - TPS destroy\n");
	sem_up(sem1);
	tps_destroy();
	return NULL;
}

void thread_A(){
	assert(tps_init(1)==-1);
	printf("OK - init fail when initialized already in main thread\n");
	assert(tps_create()==0);
	printf("OK - tps created\n");
	assert(tps_write(0, TPS_SIZE, msg1)==0);
	printf("OK - tps written\n");
	pthread_t tidB;
	pthread_create(&tidB, NULL, thread_B, tidB);
	sem_down(sem1);
	char *buffer = malloc(TPS_SIZE);
	
	sem_up(sem2);
	tps_destroy();
	return NULL;
}


int main(){
    	pthread_t currentTid = pthread_self();
	assert(tps_destroy()==-1);
	printf("OK - test destroy when tps not initialized\n");
	assert(tps_create()==-1);
	printf("OK - test create when tps not initialized\n");
	assert(tps_init(1)==0);
	printf("OK - tps initialized\n");
	assert(tps_clone(currentTid)==-1);
	printf("OK - test clone when thread @tid does not have tps\n");
	assert(tps_create()==0);
	printf("OK - tps created\n");
	
	assert(tps_clone(currentTid)==-1);
	printf("OK - test clone when current thread has tps\n");
	
	pthread_t tidA;
	pthread_create(&tid, NULL, thread_A, NULL);
	pthread_join(tid, NULL);
	
	sem_destroy(sem1);
	sem_destroy(sem2);

	return 0;
}
