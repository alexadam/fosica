#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <samplerate.h>
#include <sndfile.h>
#include <portaudio.h>
#include <jansson.h>

#include "../scripting/parser.h"
#include "../utils/utils.h"
#include "../utils/noiz_utils.h"
#include "../cache/cache.h"

/**

gcc -g -pedantic -Wall -Wextra -lefence -std=c99 -o test-parser1 ../utils/utils.c ../cache/cache.c test-parser1.c

 */

typedef enum {
	INT, FLOAT, CHAR, INT_LIST, FLOAT_LIST, CHAR_LIST, OB
} T;


typedef struct {
	T type;
	int size;

	union {
		int intVal;
		float floatVal;
		char * charVal;
		int * intListVal;
		float * floatListVal;
		char ** charListVal;
	};
} P;

typedef struct {

} F;


void testfunc(int freq, int phase, int index, int period, int channels, int bufferLen, int inputNr, float ** inputBuffers, float * output);

float * getValue(char * input, int sequenceIndex, int bufferLen) {
	return NULL;
}


int main(int argc, char **argv) {
	F obj;

}
