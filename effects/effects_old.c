//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <math.h>
//#include <pthread.h>
//#include <samplerate.h>
//#include <sndfile.h>
//#include <portaudio.h>
//#include <jansson.h>
//
//#include "../utils/utils.h"
//#include "../utils/noiz_utils.h"
//#include "../cache/cache.h"
//#include "effects.h"
//
//F_PTR getFWrapper(char * name) {
//
//	if (strcmp(name, "repeat") == 0)
//		return f_repeat;
//	if (strcmp(name, "file") == 0)
//		return f_file;
//
//	return NULL;
//}
//
//
//
////float sinGen(int index, int freq, unsigned long int lengthInSamples) {
////	if (lengthInSamples == 0) {
////		lengthInSamples = 1;
////	}
////
////	return sin(6.283 * freq * index / lengthInSamples);
////}
////
////void sinGenArray(float * data, int freq, int samplingRate, unsigned long int lengthInSamples) {
////    if (samplingRate == 0)
////        return;
////
////    for (int i = 0; i< lengthInSamples; i++) {
////        data[i] = sin(2 * 3.1415 * freq * i / samplingRate);
////    }
////}
////
////void silenceGenArray(float * data, unsigned long int lengthInSamples) {
////    for (int i = 0; i< lengthInSamples; i++) {
////        data[i] = 0.0;
////    }
////}
////
////float * sinGenA(int freq, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples) {
//////	return sinGenPhase(freq, 0, startIndex, nrOfSamples, totalNrOfSamples);
////
////	float * res = malloc(nrOfSamples * sizeof(float));
////
////		for (int i = 0; i < nrOfSamples; ++i) {
////			res[i] = sin((6.283 * freq * (startIndex + i) )/ totalNrOfSamples);
////		}
////
////		return res;
////}
////
////float * sinGenPhase(int freq, int phase, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples) {
////	float * res = malloc(nrOfSamples * sizeof(float));
////
////	for (int i = 0; i < nrOfSamples; ++i) {
////		res[i] = sin((6.283 * freq * (startIndex + i) + phase * 3.1415 / 180)/ totalNrOfSamples);
////	}
////
////	return res;
////}
////
////void f_repeat(int index, int bufferLen, int repeatAtIndex, int nrOfSeqs, int seqLen, int nrOfPoints, int * points, float * input, float * output ) {
////
////}
//
//CACHE * localCache = NULL;
//
//sndData * getCachedSnd(char * name) {
//
//	if (localCache == NULL) {
//		localCache = initCache(10);
//	}
//
//	void* co;
//	co = getCachedObj(localCache, name);
//
//	if (co == NULL) {
//		sndData snd;
//		snd = readFile(name);
//		co = putObjInCache(localCache, &snd, sizeof(snd), name);
//	}
//
//	return co;
//}
//
//void f_repeat(FUNCTION_DATA * input) {
//	// repeatIndex, maxSequences, tempo, nrOfPoints, [points]
//
//	if (input->paramSize != 5) {
//		printf("Wrong params - f_repeat");
//		return;
//	}
//
//	int repeatIndex = input->params[0]->intVal;
//	int totalNrOfSeqs = input->params[1]->intVal;
//	int tempo = input->params[2]->intVal;
//	int nrOfPoints = input->params[3]->intVal;
//	int * points = input->params[4]->intListVal;
//	int samplingRate = input->globalData->samplingRate; // TODO ??? framesPerSecond must be multiple of channel nr + what happens if inputdata.dataLength is even/ not multiple of nr channels
//	int framesPerSeq = (60.0 / tempo) * samplingRate;
//	int totalFrames = totalNrOfSeqs * framesPerSeq;
//	int repeat = totalFrames;
//	int currentSeq = -1;
//
//	int lastComputedSeq = -1;
//
//	for (int i = 0; i < input->globalData->bufferLen; i += 1) {
//
////		int ch = i % data->nrOfChannels;
//
//		int mainIndex = input->globalData->index + i;
//		int trackIndex = mainIndex % repeat;
//
//		currentSeq = (int)trackIndex / framesPerSeq;
//
//		int foundSeq = 0;
//
////		input->output->buffers[0][i] = input->input->buffers[0][i];
//
//
//		for (int var = 0; var < nrOfPoints; var += 2) {
//
//			if (currentSeq >= points[var] && currentSeq < points[var + 1]) {
//				foundSeq = 1;
//
//				int sequenceIndex = (trackIndex - (points[var] * framesPerSeq));
//
//				input->output->buffers[0][i] = input->input->buffers[0][sequenceIndex];
//
//				break; //TODO do not 'break' but instead mix all vals from each sequence ?
//			}
//		}
//
//
//		if (foundSeq == 0) {
//			input->output->buffers[0][i] = 0.0; //sin(2 * 3.1415 * 5 * i / BUFF_LEN) * 0.6;
//		}
//	}
//}
//
//void f_file(FUNCTION_DATA * input) {
//	// name, reverse, samplingRate
//
//	if (input->paramSize != 3) {
//		printf("Wrong params - f_file");
//		return;
//	}
//
//	char * fileName = input->params[0]->charVal;
//
//
//	sndData* csnd;
//	csnd = getCachedSnd(fileName);
//
//	if (csnd == NULL) {
//		return;
//	}
//
//	for (int i = 0; i < input->globalData->bufferLen; ++i) {
//		if (input->globalData->index + i >= csnd->dataLength) {
//			input->output->buffers[0][i] = 0.0;
//		} else {
//			input->output->buffers[0][i] = csnd->data[input->globalData->index + i];
//		}
//	}
//}
