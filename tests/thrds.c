#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/**
 * gcc -std=c99 -lpthread -Wall -o thrds thrds.c
 */

//void *print_message_function( void *ptr );

void createSound(int index);
void init();

typedef struct {
	char * name;
	void (*f)(float * data, int index);
} track;

pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;

float * buffer;
int bufferLen = 10;
int index = 0;
track tracks[10];
int nrOfTracks = 0;
int lastInput = 0;

void * inputThread(void * arg);
void * inputThread(void * arg) {

	printf("In input thread");

	while (1) {

		lastInput = getchar();

		if (lastInput == 3) exit(0);
		if (lastInput == 10) continue; //new line char

		printf(">>>> %d\n", lastInput);
	}
}


void * threadContainer(void * arg);
void * threadContainer(void * arg) {
	float * tmpData = (float *) malloc(bufferLen * sizeof(float));

	track * cTrack = (track*) arg;
	cTrack->f(tmpData, index);

	pthread_mutex_lock(&bufferMutex);
	mix(buffer, tmpData);
	pthread_mutex_unlock(&bufferMutex);

	free(tmpData);
}

void ps1(float * data, int index);
void ps2(float * data, int index);
void ps3(float * data, int index);
void ps4(float * data, int index);
void ps5(float * data, int index);
void ps6(float * data, int index);
void ps7(float * data, int index);
void ps8(float * data, int index);

void callback() {

	for (int i = 0; i < bufferLen; i++) {
		buffer[i] = 0.0;
	}

	createSound(index);
	index += bufferLen;

	float ssum = 0.0;
	for (int i = 0; i < bufferLen; i++) {
		ssum += buffer[i];
	}

	printf("\n\nindex %d ssum %f\n\n", index, ssum);
}

void mix(float * to, float * from) {
	for (int i = 0; i < bufferLen; i++) {
		to[i] += from[i];
	}
}

void createSound(int index) {

	pthread_t threads[nrOfTracks];

	int ret1 = 0;
	for (int i = 0; i < nrOfTracks; ++i) {
		ret1 = pthread_create(&threads[i], NULL, threadContainer, (void*) &tracks[i]);
	}

	for (int i = 0; i < nrOfTracks; ++i) {
		pthread_join(threads[i], NULL);
	}


	///////

//	for (int i = 0; i < nrOfTracks; i++) {
//		float * tmpData = (float *) malloc(bufferLen * sizeof(float));
//
//		track cTrack = tracks[i];
//		cTrack.f(tmpData, index);
//
//		mix(buffer, tmpData);
//
//		free(tmpData);
//	}
}

void init() {
	buffer = (float *) malloc(bufferLen * sizeof(float));

	track t1;
	t1.name = "t1";
	t1.f = &ps1;
	tracks[0] = t1;
	nrOfTracks++;

	track t2;
	t2.name = "t2";
	t2.f = &ps2;
	tracks[1] = t2;
	nrOfTracks++;

	track t3;
	t3.name = "t3";
	t3.f = &ps3;
	tracks[2] = t3;
	nrOfTracks++;

	track t4;
	t4.name = "t4";
	t4.f = &ps4;
	tracks[3] = t4;
	nrOfTracks++;

	track t5;
	t5.name = "t5";
	t5.f = &ps5;
	tracks[4] = t5;
	nrOfTracks++;
}

main() {
	printf("init\n");

	init();

	pthread_t th;
	pthread_create(&th, NULL, inputThread, (void*) "14");
	pthread_detach(th);

	time_t start, stop;

	clock_t t1 = clock();
	time(&start);

	while (1) {
		callback();
		sleep(5);
	}

	clock_t t2 = clock();
	time(&stop);

    printf("Elapsed: %ju \n", (t2 - t1) );
    printf("Finished in about %.0f seconds. \n", difftime(stop, start));

	exit(0);
}

void ps1(float * data, int index) {
	printf("ps1\n");
	for (int i = 0; i < bufferLen; i++) {
		data[i] = i;
	}
}

void ps2(float * data, int index) {
	printf("ps2\n");
	for (int i = 0; i < bufferLen; i++) {
		data[i] = i;
	}
}

void ps3(float * data, int index) {
	printf("ps3\n");
	for (int i = 0; i < bufferLen; i++) {
		data[i] = i;
	}
}

void ps4(float * data, int index) {
	printf("ps4\n");
	for (int i = 0; i < bufferLen; i++) {
		data[i] = i;
	}
}

void ps5(float * data, int index) {
	printf("ps5\n");
	for (int i = 0; i < bufferLen; i++) {
		data[i] = i + index/(i+1.0);
	}
}

