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
//#include "parser.h"
//#include "../utils/utils.h"
//#include "../utils/noiz_utils.h"
//#include "../cache/cache.h"
//#include "../effects/effects.h"
//
///**
//
//-g -pedantic -Wall -Wextra -lefence
//
//gcc  -std=c99 -o p_func ../utils/utils.c ../utils/noiz_utils.c ../cache/cache.c ../effects/effects.c p_func.c
//
//example:
//file,'name',reverse,samplingRate,<,>1
//repeat,after,totalSeqs,tempo,<1,>2
//channel,1,<1,>3
//
//
// */
//
//typedef struct {
//	FUNCTION_DATA * f_in;
//	F_PTR f;
//} Func;
//
//float ** buffers = NULL;
//
//void initFunctions(int nrOfBuffers, int bufferLen) {
//	buffers = malloc(nrOfBuffers * sizeof(float *));
//
//	for (int i = 0; i < nrOfBuffers; ++i) {
//		buffers[i] = malloc(bufferLen * sizeof(float));
//	}
//}
//
//int isFloat(char * input) {
//	if (input == NULL) {
//		return 0;
//	}
//
//	for (int i = 0; i < strlen(input); ++i) {
//		if (input[i] == '.') {
//			return 1;
//		}
//	}
//
//	return 0;
//}
//
//void parseParam(PARAM_WRAPPER * wrapper, char * param) {
//
//	if (isdigit(param[0]) || (strlen(param) > 1 && isdigit(param[1]) && param[0] == '-')) {
//		if (isFloat(param)) {
//			wrapper->type = FLOAT_VAL;
//			wrapper->floatVal = string2float(param);
//		} else {
//			wrapper->type = INT_VAL;
//			wrapper->intVal = string2int(param);
//		}
//	} else if (param[0] == '[') {
//		int listElemSize = 0;
//		char ** listParts = split(&param[1], ' ', &listElemSize);
//
//		if (isFloat(listParts[0])) {
//			wrapper->type = FLOAT_VAL_LIST;
//			wrapper->size = listElemSize;
//			wrapper->floatListVal = malloc(listElemSize * sizeof(float));
//
//			for (int i = 0; i < listElemSize; ++i) {
//				wrapper->floatListVal[i] = string2float(listParts[i]);
//			}
//		} else {
//			wrapper->type = INT_VAL_LIST;
//			wrapper->size = listElemSize;
//			wrapper->intListVal = malloc(listElemSize * sizeof(int));
//
//			for (int i = 0; i < listElemSize; ++i) {
//				wrapper->intListVal[i] = string2int(listParts[i]);
//			}
//		}
//	} else if (param[0] == '\'') {
//		wrapper->type = CHAR_VAL;
//		wrapper->charVal = malloc((strlen(param)) * sizeof(char));
//		strcpy(wrapper->charVal, &param[1]);
//	}
//}
//
//void parseDataBuffers(DATA_BUFFERS * input, char * param) {
//	int nrOfParams = 0;
//	char ** paramParts = split(param, ' ', &nrOfParams);
//
//	input->size = nrOfParams;
//	input->buffers = malloc(nrOfParams * sizeof(float *));
//
//	for (int i = 0; i < nrOfParams; ++i) {
//		input->buffers[i] = buffers[string2int(paramParts[i])];
//		printf("PPPP %p\n", input->buffers[i]);
//	}
//}
//
//Func ** parseFunc(char * input, int * nrOfFunc) {
//	char ** funcParts = split(input, '\n', nrOfFunc);
//
//	Func ** ret = malloc(*nrOfFunc * sizeof(Func *));
//
//	for (int i = 0; i < *nrOfFunc; ++i) {
//		ret[i] = malloc(sizeof(Func));
//
//		int nrOfParams = 0;
//		char ** parts = split(funcParts[i], ',', &nrOfParams);
//
//		ret[i]->f = getFWrapper(parts[0]);
//
//		ret[i]->f_in = malloc(sizeof(FUNCTION_DATA));
//		ret[i]->f_in->globalData = malloc(sizeof(GLOBAL_DATA));
//		ret[i]->f_in->paramSize = nrOfParams - 3;
//		ret[i]->f_in->params = malloc(ret[i]->f_in->paramSize * sizeof(PARAM_WRAPPER *));
//
//		for (int j = 1; j < nrOfParams; ++j) {
//			if (parts[j][0] == '<') {
//
//				if (strlen(parts[j]) == 1) {
//					ret[i]->f_in->input = NULL;
//				} else {
//					ret[i]->f_in->input = malloc(sizeof(DATA_BUFFERS));
//					printf("input\n");
//					parseDataBuffers(ret[i]->f_in->input, &parts[j][1]);
//				}
//			} else if (parts[j][0] == '>') {
//
//				if (strlen(parts[j]) == 1) {
//					ret[i]->f_in->output = NULL;
//				} else {
//					ret[i]->f_in->output = malloc(sizeof(DATA_BUFFERS));
//					printf("output\n");
//					parseDataBuffers(ret[i]->f_in->output, &parts[j][1]);
//				}
//			} else {
//				ret[i]->f_in->params[j-1] = malloc(sizeof(PARAM_WRAPPER));
//				parseParam(ret[i]->f_in->params[j-1], parts[j]);
//			}
//		}
//	}
//
//	return ret;
//}
//
//void eval(Func * ff, GLOBAL_DATA * gd) {
//	ff->f_in->globalData = gd;
//	ff->f(ff->f_in);
//}
//
//int lastHash = -1;
//Func ** parsedFunc = NULL;
//int nrFunc = 0;
//
//float * getValue(char * input, int sequenceIndex, int bufferLen) {
//	if (buffers == NULL) {
//		initFunctions(10, bufferLen);
//	}
//
//	int currentHash = hash(input);
//
//	if (currentHash != lastHash) {
//		parsedFunc = parseFunc(input, &nrFunc);
//		lastHash = currentHash;
//	}
//
//	GLOBAL_DATA * gd = malloc(sizeof(GLOBAL_DATA));
//	gd->bufferLen = bufferLen;
//	gd->index = sequenceIndex;
//	gd->nrOfChannels = 2;
//	gd->samplingRate = 44104;
//
//	for (int i = 0; i < nrFunc; ++i) {
//		eval(parsedFunc[i], gd);
//	}
//
////
////	for (int i = 0; i < bufferLen; ++i) {
////		printf("REZ %f\n", buffers[nrFunc-1][i]);
////	}
//
//	return buffers[nrFunc-1];
//}
//
