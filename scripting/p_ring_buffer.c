#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <samplerate.h>
#include <sndfile.h>
#include <portaudio.h>
#include <jansson.h>

#include "parser.h"
#include "../utils/utils.h"
#include "../utils/noiz_utils.h"
#include "../cache/cache.h"

const int RING_BUFF_SIZE = 80000; //TODO multiple of BUFF_LEN
CACHE * che = NULL;

typedef struct {
	float * data;
	int start;
	int end;
	int capacity;
} RingBuff;

typedef struct {
	RingBuff *  bufferData;
	char * instructions;
	int lastHash;
} cFunc;

void addToBuffer(RingBuff * buffer, float val) {
	buffer->data[buffer->end] = val;
	buffer->end += 1;
	buffer->end = buffer->end % buffer->capacity;
}

float getFromBuffer(RingBuff * buffer) {
	float ret = buffer->data[buffer->start];
	buffer->start += 1;
	buffer->start = buffer->start % buffer->capacity;
	return ret;
}

void resetBuffer(RingBuff * buffer) {
	buffer->start = 0;
	buffer->end = 0;
}

sndData * getCachedSnd(char * name) {
	void* co;
	co = getCachedObj(che, name);

	if (co == NULL) {
		sndData snd;
		snd = readFile(name);
		co = putObjInCache(che, &snd, sizeof(snd), name);
	}

	return co;
}

void file(char * name, int index, RingBuff * out, int bufferLen) {
	sndData* csnd;
	csnd = getCachedSnd(name);

	for (int i = 0; i < bufferLen; ++i) {
		if (index + i >= csnd->dataLength) {
			addToBuffer(out, 0.0);
		} else {
			addToBuffer(out, csnd->data[index + i]);
		}
	}
}

void eval(cFunc ** chain, int nextFunc, int index, int bufferLen) {

	int diff = chain[nextFunc]->bufferData->end - chain[nextFunc]->bufferData->start;

	if (diff < 0) {
		diff = chain[nextFunc]->bufferData->capacity - diff;
	}

	if (diff >= bufferLen) {
		return;
	}

	int localIndex = index;
	int uuu = 0;

	while (diff < bufferLen && uuu<2000000) {

		uuu++;

		int elemCount = 0;
		char ** parts = split(chain[nextFunc]->instructions, ',', &elemCount);

		if (strcmp(parts[0], "file") == 0) {
			file(parts[1], localIndex, chain[nextFunc]->bufferData, bufferLen);
		}

		localIndex += bufferLen;

		diff = chain[nextFunc]->bufferData->end - chain[nextFunc]->bufferData->start;

		if (diff < 0) {
			diff = chain[nextFunc]->bufferData->capacity - diff;
		}

		for (int i = 0; i < elemCount; ++i) {
			free(parts[i]);
		}
		free(parts);

		//multiple buffer requests only if the 'final' buffer is smaller that bufferLen TODO test
		if (nextFunc != 0) {
			return;
		}

	}
}

cFunc ** parse(char * fileContent) {
	int elemCount = 0;
	char ** parts = split(fileContent, '|', &elemCount);

	cFunc ** chain = malloc(elemCount * sizeof(cFunc *));

	for (int i = 0; i < elemCount; ++i) {
		chain[i] = malloc(sizeof(cFunc));
		chain[i]->instructions = malloc(sizeof(char) * (strlen(parts[i]) + 1));
		strcpy(chain[i]->instructions, parts[i]);
		chain[i]->lastHash = hash(parts[i]);
		chain[i]->bufferData = malloc(sizeof(RingBuff));
		chain[i]->bufferData->start = 0;
		chain[i]->bufferData->end = 0;
		chain[i]->bufferData->capacity = RING_BUFF_SIZE;
		chain[i]->bufferData->data = malloc(chain[i]->bufferData->capacity * sizeof(float));
	}

	for (int i = 0; i < elemCount; ++i) {
		free(parts[i]);
	}
	free(parts);

	return chain;
}

int lastHash = 0;
cFunc ** chain = NULL;
float * retBuffer = NULL;

float * getValue(char * input, int sequenceIndex, int bufferLen) {
	if (che == NULL) {
		che = initCache(10);
	}

	if (retBuffer == NULL) {
		retBuffer = malloc(bufferLen * sizeof(float));
	}

	int currentHash = hash(input);

	if (input && currentHash != lastHash) {
		lastHash = currentHash;
		chain = parse(input);
		printf("in get %s\n", input);
	}


	eval(chain, 0, sequenceIndex, bufferLen);

	for (int i = 0; i < bufferLen; ++i) {
		retBuffer[i] = getFromBuffer(chain[0]->bufferData);
	}

	return retBuffer;
}

