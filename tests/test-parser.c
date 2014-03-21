#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../utils.h"
#include "../effects.h"

// gcc -g -std=c99 test-parser.c ../utils.c ../effects.c -lm -o test-parser


float dummyParser(char * instructions, unsigned long mainIndex, int tempo, int nrOfSeq, int samplingRate);
float getValueFromFunction(char * functionSignature, unsigned long mainIndex, int tempo, int nrOfSeq, int samplingRate);
float getValueFromFile(char * fileName, unsigned long int position);

#define VECTOR_INITIAL_CAPACITY 100

typedef struct {
  int size;      // slots used so far
  int capacity;  // total available slots
  char *data;     // array of integers we're storing
} Vector;

void vector_init(Vector *vector) {
  // initialize size and capacity
  vector->size = 0;
  vector->capacity = VECTOR_INITIAL_CAPACITY;

  // allocate memory for vector->data
  vector->data = malloc(sizeof(char) * vector->capacity);
}

void vector_append(Vector *vector, char * value) {
  // make sure there's room to expand into
//  vector_double_capacity_if_full(vector);

  // append the value and increment vector->size
  vector->data[vector->size] = malloc((strlen(value) + 1) * sizeof(char));
  strcpy(vector->data[vector->size], value);
  vector->size++;
}

char * vector_get(Vector *vector, int index) {
  if (index >= vector->size || index < 0) {
    printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
    exit(1);
  }
  return vector->data[index];
}

//void vector_set(Vector *vector, int index, int value) {
//  // zero fill the vector up to the desired index
//  while (index >= vector->size) {
//    vector_append(vector, 0);
//  }
//
//  // set the value at the desired index
//  vector->data[index] = value;
//}

//void vector_double_capacity_if_full(Vector *vector) {
//  if (vector->size >= vector->capacity) {
//    // double vector->capacity and resize the allocated memory accordingly
//    vector->capacity *= 2;
//    vector->data = realloc(vector->data, sizeof(char) * vector->capacity);
//  }
//}

void vector_free(Vector *vector) {
  free(vector->data);
}

char ** split(char * input, char sep, int * elemCoount) {

	char ** result = NULL;
	int inputLen = strlen(input);
	int lastPosition = 0;

	for (int i = 0; i < inputLen; ++i) {
		if (input[i] == sep) {
			(*elemCoount)++;
		}
	}

	(*elemCoount)++;

	int curElem = 0;

	result = malloc((*elemCoount) * sizeof(char *));

	for (int i = 0; i < inputLen; ++i) {
		if (input[i] == sep || i == inputLen - 1) {
			int newLen = ((i == (inputLen - 1)) ? i - lastPosition + 2 : i - lastPosition + 1);

			*(result + curElem) = malloc(newLen * sizeof(char));

			memcpy ((result + curElem) , input + lastPosition, newLen - 1);

			lastPosition = i+1;
			curElem++;
		}
	}

	return result;
}

float getValue(char * instructions, unsigned long int mainIndex, int tempo, int nrOfSeq, int samplingRate) {
	return dummyParser(instructions, mainIndex, tempo, nrOfSeq, samplingRate);
}

float dummyParser(char * input, unsigned long mainIndex, int tempo, int nrOfSeq, int samplingRate) {

	char * instructions = NULL;

	char * tmp1 =  ulint2string(mainIndex);
	char * tmp2 = int2string(tempo);

	instructions = str_replace(input, "#globalIndex", tmp1); //TODO unsigned long int
	instructions = str_replace(instructions, "#tempo", tmp2);

	free(tmp1);
	free(tmp2);

	//TODO de-alloc int2string() stuff

	int lenObj = strlen(instructions);
	int has = 0;

	for (int i = 0; i < lenObj; ++i) {
		if (instructions[i] == '|') {
			has = 1;
			break;
		}
	}

	int elemCount = 0;
	char ** tokens = NULL;

	if (!has) {
		tokens = &instructions;
		elemCount = 1;
	} else {
		tokens = split(instructions, '|', &elemCount);
		printf("merge 1 %s \n", tokens);
	}

	if (!tokens) {
		return 0.0;
	}

	float * valuesStack = malloc(elemCount * sizeof(float));

	for (int i = 0; i < elemCount; ++i) {
		char * token = (tokens + i);

		if (strcmp(token, "*") == 0) {
			valuesStack[i] = valuesStack[i - 2] * valuesStack[i - 1];
		} else if (strcmp(token, "+") == 0) {
			valuesStack[i] = valuesStack[i - 2] + valuesStack[i - 1];
		} else if (strcmp(token, "-") == 0) {
			valuesStack[i] = valuesStack[i - 2] - valuesStack[i - 1];
		} else if (strcmp(token, "/") == 0) {
			if (valuesStack[i - 1] == 0.0) {
				valuesStack[i] = valuesStack[i - 2];
			} else {
				valuesStack[i] = valuesStack[i - 2] / valuesStack[i - 1];
			}
		} else {
			int hasCommas = 0;
			int elemLen = strlen(token);

			for (int j = 0; j < elemLen; ++j) {
				if (*(token + j) == ',') {
					hasCommas = 1;
					break;
				}
			}

			if (!hasCommas) {
				//is number
				valuesStack[i] = strtof(*(tokens + i), NULL);
			} else {
				//is function
				valuesStack[i] = getValueFromFunction(*(tokens + i), mainIndex, tempo, nrOfSeq, samplingRate);
			}
		}

	}

	float result = valuesStack[elemCount - 1];

	free(valuesStack);
	free(instructions);

	return result;
}

float getValueFromFunction(char * functionSignature, unsigned long mainIndex, int tempo, int nrOfSeq, int samplingRate) {
	int lenObj = strlen(functionSignature);
	int has = 0;

	for (int i = 0; i < lenObj; ++i) {
		if (functionSignature[i] == ',') {
			has = 1;
			break;
		}
	}

	if (!has) {
		return 0.0;
	}

	int elemCount = 0;

	char ** tokens = split(functionSignature, ',', &elemCount);
//	printf("merge 2 %s %s\n", (tokens+1), tokens);

	if (strcmp(tokens, "file") == 0) {
		return getValueFromFile((tokens + 1), mainIndex);
	} else if (strcmp((tokens), "sin") == 0) {
		char * freqChar = (tokens + 1);
		int freq = string2int(freqChar);

		char * lengthInSamplesChar = (tokens + 2);
		int lengthInSamples = string2int(lengthInSamplesChar);

		return sinGen(mainIndex, freq, lengthInSamples);
	}

	for (int i = 0; i < elemCount; ++i) {
		free((tokens + i));
	}
	free(tokens);

	return 0.0;
}

float getValueFromFile(char * fileName, unsigned long int position) {
	return 12.3;
}

int main()
{
//    char * inputStr = "2|3|*|1.77|+|file,sounds/gigi|+|#globalIndex|-|#tempo|+|sin,250,1000|*";
    char * inputStr = "file,sounds/hat.wav";

    float a = getValue(inputStr, 100, 50, 100, 100);

    printf("%f\n", a);

    return 0;
}
