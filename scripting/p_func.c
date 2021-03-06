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
#include "../effects/effects.h"

/**

-g -pedantic -Wall -Wextra -lefence

gcc  -std=c99 -o p_func ../utils/utils.c ../utils/noiz_utils.c ../cache/cache.c ../effects/effects.c p_func.c

 */

FUNCTION ** functions = NULL;
GLOBAL_DATA * gd = NULL;
int lastHash = -1;
int nrFunc = 0;

int isFloat(char * input) {
	if (input == NULL) {
		return 0;
	}

	for (int i = 0; i < strlen(input); ++i) {
		if (input[i] == '.') {
			return 1;
		}
	}

	return 0;
}

void parseParam(PARAM_WRAPPER * wrapper, char * param) {

	if (isdigit(param[0]) || (strlen(param) > 1 && isdigit(param[1]) && param[0] == '-')) {
		if (isFloat(param)) {
			wrapper->type = FLOAT_VAL;
			wrapper->floatVal = string2float(param);
		} else {
			wrapper->type = INT_VAL;
			wrapper->intVal = string2int(param);
		}
	} else if (param[0] == '[') {
		int listElemSize = 0;
		char ** listParts = split(&param[1], ' ', &listElemSize);

		if (isFloat(listParts[0])) {
			wrapper->type = FLOAT_VAL_LIST;
			wrapper->size = listElemSize;
			wrapper->floatListVal = malloc(listElemSize * sizeof(float));

			for (int i = 0; i < listElemSize; ++i) {
				wrapper->floatListVal[i] = string2float(listParts[i]);
			}
		} else {
			wrapper->type = INT_VAL_LIST;
			wrapper->size = listElemSize;
			wrapper->intListVal = malloc(listElemSize * sizeof(int));

			for (int i = 0; i < listElemSize; ++i) {
				wrapper->intListVal[i] = string2int(listParts[i]);
			}
		}
	} else if (param[0] == '\'') {
		wrapper->type = CHAR_VAL;
		wrapper->charVal = malloc((strlen(param)) * sizeof(char));
		strcpy(wrapper->charVal, &param[1]);
	}
}

void parseDataBuffers(FUNCTION_BUFFERS * input, char * param) {
	int nrOfParams = 0;
	char ** paramParts = split(param, ' ', &nrOfParams);

	input->size = nrOfParams;
	input->functions = malloc(nrOfParams * sizeof(FUNCTION));

	for (int i = 0; i < nrOfParams; ++i) {
		input->functions[i] = functions[string2int(paramParts[i])];
	}

	for (int i = 0; i < nrOfParams; ++i) {
		free(paramParts[i]);
	}
	free(paramParts);
}

void parseFunc(char * input, int * nrOfFunc, int bufferLen) {
	char ** funcParts = split(input, '\n', nrOfFunc);

	functions = malloc(*nrOfFunc * sizeof(FUNCTION *));

	for (int i = 0; i < *nrOfFunc; ++i) {
		functions[i] = malloc(sizeof(FUNCTION));

		int nrOfParams = 0;
		char ** parts = split(funcParts[i], ',', &nrOfParams);

		functions[i]->f_ptr = getFWrapper(parts[0]);

		functions[i]->f_data = malloc(sizeof(FUNCTION_DATA));
		functions[i]->f_data->globalData = malloc(sizeof(GLOBAL_DATA));
		functions[i]->f_data->paramSize = nrOfParams - 2;
		functions[i]->f_data->params = malloc(functions[i]->f_data->paramSize * sizeof(PARAM_WRAPPER *));
		functions[i]->f_data->output = malloc(bufferLen * sizeof(float));

		for (int j = 1; j < nrOfParams; ++j) {
			if (parts[j][0] == '<') {

				if (strlen(parts[j]) == 1) {
					functions[i]->f_data->input = NULL;
				} else {
					functions[i]->f_data->input = malloc(sizeof(FUNCTION_BUFFERS));
					parseDataBuffers((FUNCTION_BUFFERS *)functions[i]->f_data->input, &parts[j][1]);
				}
			} else {
				functions[i]->f_data->params[j-1] = malloc(sizeof(PARAM_WRAPPER));
				parseParam(functions[i]->f_data->params[j-1], parts[j]);
			}
		}
	}

	for (int i = 0; i < *nrOfFunc; ++i) {
		free(funcParts[i]);
	}
	free(funcParts);
}

void destroyFunc() {
	for (int i = 0; i < nrFunc; ++i) {
		free(functions[i]->f_data->output);
		free(functions[i]->f_data->input);
		free(functions[i]->f_data->globalData);

		for (int j = 0; j < functions[i]->f_data->paramSize; ++j) {
			if (functions[i]->f_data->params[j]->type == FLOAT_VAL_LIST) {
				free(functions[i]->f_data->params[j]->floatListVal);
			} else if (functions[i]->f_data->params[j]->type == INT_VAL_LIST) {
				free(functions[i]->f_data->params[j]->intListVal);
			} else if (functions[i]->f_data->params[j]->type == CHAR_VAL) {
				free(functions[i]->f_data->params[j]->charVal);
			} else if (functions[i]->f_data->params[j]->type == CHAR_VAL_LIST) {
				free(functions[i]->f_data->params[j]->charListVal);
			}
		}

		free(functions[i]->f_data->params);
		free(functions[i]->f_data);
	}
	free(functions);
}

void eval(FUNCTION * ff, GLOBAL_DATA * gd) {
	ff->f_data->globalData->bufferLen = gd->bufferLen;
	ff->f_data->globalData->index = gd->index;
	ff->f_data->globalData->nrOfChannels = gd->nrOfChannels;
	ff->f_data->globalData->samplingRate = gd->samplingRate;

	ff->f_ptr(ff->f_data);
}

float * getValue(char * input, int sequenceIndex, int bufferLen) {
	int currentHash = hash(input);

	if (currentHash != lastHash) {
		//TODO replace global variables: #index, #sampling-rate, #channels etc.

		if (functions != NULL) {
			destroyFunc();
		}
		nrFunc = 0;
		parseFunc(input, &nrFunc, bufferLen);
		lastHash = currentHash;
	}

	if (gd == NULL) {
		gd = malloc(sizeof(GLOBAL_DATA));
	}

	gd->bufferLen = bufferLen;
	gd->index = sequenceIndex;
	gd->nrOfChannels = 2;
	gd->samplingRate = 44104;

	for (int i = 0; i < nrFunc; ++i) {
		eval(functions[i], gd);
	}

	return functions[nrFunc-1]->f_data->output;
}
