#ifndef EFFECTS_H_
#define EFFECTS_H_

typedef enum {
	INT_VAL, FLOAT_VAL, CHAR_VAL, INT_VAL_LIST, FLOAT_VAL_LIST, CHAR_VAL_LIST
} DATA_TYPE;

typedef struct {
	DATA_TYPE type;
	int size;

	union {
		int intVal;
		float floatVal;
		char * charVal;
		int * intListVal;
		float * floatListVal;
		char ** charListVal;
	};
} PARAM_WRAPPER;

typedef struct {
	int index;
	int bufferLen;
	int samplingRate;
	int nrOfChannels;
} GLOBAL_DATA;

typedef struct {
	float ** buffers;
	int size;
} DATA_BUFFERS;

typedef struct {
	GLOBAL_DATA * globalData;
	int paramSize;
	PARAM_WRAPPER ** params;
	DATA_BUFFERS * input;
	DATA_BUFFERS * output;
} F_INPUT;

typedef void (* F_PTR)(F_INPUT *);

F_PTR getFWrapper(char * name);

void f_repeat(F_INPUT * input);
void f_file(F_INPUT * input);


//float sinGen(int index, int freq, unsigned long int lengthInSamples);
//
//void sinGenArray(float * data, int freq, int samplingRate, unsigned long int lengthInSamples);
//
//float * sinGenA(int freq, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples);
//
//float * sinGenPhase(int freq, int phase, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples);
//
//void silenceGenArray(float * data, unsigned long int lengthInSamples);

#endif /* EFFECTS_H_ */
