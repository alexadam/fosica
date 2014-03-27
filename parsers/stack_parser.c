
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <matheval.h>
#include "../utils.h"
#include "../parser.h"

typedef struct
{
    char ** data;
    int index;
    int capacity;
} charStack;

typedef struct
{
    float * data;
    int index;
    int capacity;
} floatStack;

charStack cStack;
floatStack fStack;

void initStacks(int capacity) {
	fStack.data = malloc(capacity * sizeof(float));
	fStack.index = 0;
	fStack.capacity = capacity;
	cStack.data = (char **)malloc(capacity * sizeof(char *));
	cStack.index = 0;
	cStack.capacity = capacity;
}

void destroyStacks() {
	free(fStack.data);
	free(cStack.data);
}

void pushFloat(float val) {
	fStack.data[fStack.index] = val;
	fStack.index++;
	fStack.index = fStack.index % fStack.capacity;
}

void pushString(char * val) {
	cStack.data[cStack.index] = malloc((strlen(val) + 1) * sizeof(char));
	strcpy(cStack.data[cStack.index], val);

	cStack.index++;
	cStack.index = cStack.index % cStack.capacity;
}

float popFloat() {
	fStack.index--;
	fStack.index = fStack.index % fStack.capacity;
	return fStack.data[fStack.index];
}

char * popString() {
	cStack.index--;
	cStack.index = cStack.index % cStack.capacity;
	return cStack.data[cStack.index];
}

void printFStack() {
	for (int i = 0; i < fStack.index; ++i) {
		printf("Float Stack %f\n", fStack.data[i]);
	}
}

void printCStack() {
	for (int i = 0; i < cStack.index; ++i) {
		printf("Char Stack %s\n", cStack.data[i]);
	}
}

float parseValue(char * input) {
	int inputLen = strlen(input);
	int lastPosition = 0;
	float res = 0.0;
	initStacks(1024);

	int elemCount = 0;
	char ** parts = split(input, '|', &elemCount);

	for (int i = 0; i < elemCount; ++i) {
		char * param = parts[i];

		if (param[0] == '\'') {
			//string
			pushString(param);
		} else if (isdigit(param[0]) || (strlen(param) > 1 && isdigit(param[1]) && param[0] == '-')) {
			float val = string2float(param);
			pushFloat(val);
		} else {
			//op
			if (strcmp(param, "+") == 0) {
				float v1 = popFloat();
				float v2 = popFloat();
				res = v1 + v2;
			} else if (strcmp(param, "-") == 0) {
				float v1 = popFloat();
				float v2 = popFloat();
				res = v2 - v1;
			} else if (strcmp(param, "*") == 0) {
				float v1 = popFloat();
				float v2 = popFloat();
				res = v1 * v2;
			} else if (strcmp(param, "/") == 0) {
				float v1 = popFloat();
				float v2 = popFloat();
				if (v1 == 0) {
					v1 = 1.0;
				}
				res = v2 / v1;
			} else {
				//function name
//				if (strcmp(param, "b") == 0) {
//					int v1 = (int) popFloat();
//					char * tmp = popString();
//
//					res = b(tmp, v1);
//
//					free(tmp);
//				} else if (strcmp(param, "a") == 0) {
//					float v3 = (float) popFloat();
//					int v2 = (int) popFloat();
//					int v1 = (int) popFloat();
//					res = a(v1, v2, v3);
//				} else {
//					continue;
//				}
			}

			pushFloat(res);
		}

		lastPosition = i+1;
		free(param);
	}
	free(parts);
	res = fStack.data[0];

	destroyStacks();

	return res;
}
