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
	GLOBAL_DATA * globalData;
	int paramSize;
	PARAM_WRAPPER ** params;
	void * input; //FUNCTION_BUFFERS
	float * output;
} FUNCTION_DATA;

typedef void (* F_PTR)(FUNCTION_DATA *);

typedef struct {
	FUNCTION_DATA * f_data;
	F_PTR f_ptr;
} FUNCTION;

typedef struct {
	FUNCTION ** functions;
	int size;
} FUNCTION_BUFFERS;

F_PTR getFWrapper(char * name);

void f_repeat(FUNCTION_DATA * input);
void f_file(FUNCTION_DATA * input);
void f_mix(FUNCTION_DATA * input);
void f_channel(FUNCTION_DATA * input);
void f_sin_osc(FUNCTION_DATA * function_data);
void f_tri_osc(FUNCTION_DATA * function_data);
void f_mul(FUNCTION_DATA * function_data);

#endif /* EFFECTS_H_ */
