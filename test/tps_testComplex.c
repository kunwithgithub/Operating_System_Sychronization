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
char *buffer = NULL;
pthread_t globalA;
void *thread_B(void *arg){

	pthread_t tid = *((pthread_t *)arg);
	assert(tps_clone(tid)==0);
	printf("OK - Thread B clones success\n");
	buffer = malloc(TPS_SIZE);
	memset(buffer, 0, TPS_SIZE);
	assert(tps_read(100, TPS_SIZE, buffer)==-1);
	printf("OK - invalid read offset case passed\n");
	assert(tps_read(0, TPS_SIZE+1, buffer)==-1);
	printf("OK - invalid read length case passed\n");
	assert(tps_read(0, TPS_SIZE, buffer)==0);
	printf("OK - Thread B reads success! \n");
	assert(!memcmp(msg1, buffer, TPS_SIZE));
	printf("OK - Thread B reads from itself success and which is same as what thread A has \n");

	assert(tps_destroy()==0);
	printf("OK - TPS B destroy\n");
	sem_up(sem1);
	return NULL;
}

void *thread_A(void *arg){

	pthread_t tidA = pthread_self();
	assert(tps_init(1)==-1);
	printf("OK - init fail when already initialized in main thread\n");
	assert(tps_create()==0);
	printf("OK - tps created \n");
	assert(tps_create()==-1);
	printf("OK - tps already existed \n");
	assert(tps_write(100, TPS_SIZE, msg1)==-1);
	printf("OK - invalid write offset case passed\n");
	assert(tps_write(0, TPS_SIZE+1, msg1)==-1);
	printf("OK - invalid write length case passed\n");
	assert(tps_write(0, TPS_SIZE, msg1)==0);
	printf("OK - tps written\n");
	pthread_t tidB;
	tidB = pthread_create(&tidB, NULL, thread_B,&tidA);
	sem_up(sem2);
	sem_down(sem1);
	
	
	assert(tps_destroy()==0);
	printf("OK - TPS A destroy\n");
	return NULL;
}


int main(){

    pthread_t currentTid = pthread_self();
	sem1 = sem_create(0);
	sem2 = sem_create(0);

	assert(tps_destroy()==-1);
	printf("OK - test destroy when tps not initialized\n");
	assert(tps_create()==-1);
	printf("OK - test create when tps not initialized\n");
	assert(tps_init(1)==0);
	printf("OK - tps initialized\n");
	assert(tps_clone(currentTid)==-1);
	printf("OK - test clone when thread @tid does not have tps\n");
	
	assert(tps_read(0, TPS_SIZE, buffer)==-1);
	printf("OK - test read when buffer is NULL\n");
	assert(tps_write(0, TPS_SIZE, buffer)==-1);
	printf("OK - test write when buffer is NULL\n");

	buffer = malloc(TPS_SIZE);
	assert(tps_read(0, TPS_SIZE, buffer)==-1);
	printf("OK - test read when tps is not created\n");
	assert(tps_write(0, TPS_SIZE, msg1)==-1);
	printf("OK - test write when tps is not created\n");
	assert(tps_create()==0);
	printf("OK - tps created\n");
	
	assert(tps_clone(currentTid)==-1);
	printf("OK - test clone when current thread has tps\n");
	
	pthread_t tidA;
	pthread_create(&tidA, NULL, thread_A, NULL);
	pthread_join(tidA, NULL);
	
	sem_destroy(sem1);
	sem_destroy(sem2);

	return 0;
}
