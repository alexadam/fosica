
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../parser.h"
#include "../utils.h"

/*
 *

gcc -g -std=c99 wtack_parser.c ../utils.c -o wtack_parser


 *
 */
int BUFFER_LEN = 10;

typedef enum {cFLOAT, cSTRING, cFLOAT_LIST} ALL_TYPES;

typedef struct
{
    ALL_TYPES type;
    int size;

    union
    {
    	float float_val;
    	char * string_val;
    	float * float_list_val;
    };

} mType;

typedef struct
{
	mType ** data;
    int index;
    int capacity;
} objStack;

objStack oStack;
mType ** vars;

int maxVars = 30;

typedef enum {ADD, SUB, MUL, DIV, MOD, EQ, NOT, NOTEQ, GT, GTE, LT, LTE, AND, OR} OPS;

void initStacks(int capacity) {
	oStack.data = malloc(capacity * sizeof(mType *));
	oStack.index = 0;
	oStack.capacity = capacity;

	vars = malloc(maxVars * sizeof(mType *));

	for (int i = 0; i < maxVars; ++i) {
		vars[i] = NULL;
	}
}

void destroyStacks() {
	for (int i = 0; i < oStack.index; ++i) {
		destroyObj(oStack.data[i]);
	}
	free(oStack.data);
}

void destroyObj(mType * obj) {
	if (obj->type == cFLOAT_LIST) {
		free(obj->float_list_val);
	}

	free(obj);
}

void pushObj(mType * val) {
	oStack.data[oStack.index] = val;
	oStack.index++;
	oStack.index = oStack.index % oStack.capacity;
}

mType * popObj() {
	oStack.index--;
	oStack.index = oStack.index % oStack.capacity;
	return oStack.data[oStack.index];
}

mType * deepCopy(mType * source) {
	mType * res = malloc(sizeof(mType));
	res->type = source->type;

	if (source->type == cFLOAT) {
		res->float_val = source->float_val;
	} else if (source->type == cFLOAT_LIST) {
		res->size = source->size;
		res->float_list_val = malloc(res->size * sizeof(float));
		memcpy(res->float_list_val, source->float_list_val, res->size * sizeof(float));
	}

	return res;
}

mType * parseList(char * listStr) {
	mType * listElem = malloc(sizeof(mType));
	listElem->type = cFLOAT_LIST;

	int slen = strlen(listStr);
	char * sub = substring(listStr, 1, slen - 2);
	int elemCount = 0;
	char ** listParts = split(sub, ' ', &elemCount);

	listElem->size = elemCount;
	listElem->float_list_val = malloc(elemCount * sizeof(float));

	for (int i = 0; i < elemCount; ++i) {
		listElem->float_list_val[i] = string2float(listParts[i]);
	}

	free(sub);
	for (int i = 0; i < elemCount; ++i) {
		free(listParts[i]);
	}
	free(listParts);

	return listElem;
}

void printFStack() {
	for (int i = 0; i < oStack.index; ++i) {
		mType * tmp = oStack.data[i];

		printf("lista are %p %d %d %d\n", tmp, tmp->size, oStack.index, i);

		if (tmp->type == cFLOAT) {
			printf("nr %f\n",tmp->float_val);
		} else if (tmp->type == cFLOAT_LIST) {
			for (int j = 0; j < tmp->size; ++j) {
				printf("Float list %f\n", tmp->float_list_val[j]);
			}
		}
	}
}

mType * file(char * fileName) {
	mType * result = malloc(sizeof(mType));
	result->type = cFLOAT_LIST;
	result->size = BUFFER_LEN;

	result->float_list_val = malloc(BUFFER_LEN * sizeof(float));
	for (int i = 0; i < BUFFER_LEN; ++i) {
		result->float_list_val[i] = i;
	}

	return result;
}

mType * baseOp(OPS op, mType * f1, mType * f2) {
	mType * res = malloc(sizeof(mType));

	if (f1->type == cFLOAT && f2->type == cFLOAT) {
		res->type = cFLOAT;

		if (op == ADD) {
			res->float_val = f1->float_val + f2->float_val;
		} else if (op == SUB) {
			res->float_val = f1->float_val - f2->float_val;
		} else if (op == MUL) {
			res->float_val = f1->float_val * f2->float_val;
		} else if (op == DIV) {
			float tmp = f2->float_val;
			if (tmp == 0.0) {
				tmp = 1.0;
			}

			res->float_val = f1->float_val / tmp;
		} else if (op == MOD) {
			res->float_val = (int)f1->float_val % (int)f2->float_val;
		}
	} else if (f1->type == cFLOAT_LIST && f2->type == cFLOAT_LIST) {
		res->type = cFLOAT_LIST;

		mType * smallerType = f1->size >= f2->size ? f2 : f1;
		mType * biggerType = f1->size >= f2->size ? f1 : f2;

		res->size = biggerType->size;
		res->float_list_val = malloc(biggerType->size * sizeof(float));

		for (int i = 0; i < biggerType->size; ++i) {
			if (i < smallerType->size) {

				if (op == ADD) {
					res->float_list_val[i] = f1->float_list_val[i] + f2->float_list_val[i];
				} else if (op == SUB) {
					res->float_list_val[i] = f1->float_list_val[i] - f2->float_list_val[i];
				} else if (op == MUL) {
					res->float_list_val[i] = f1->float_list_val[i] * f2->float_list_val[i];
				} else if (op == DIV) {
					float tmp = f2->float_list_val[i];
					if (tmp == 0.0) {
						tmp = 1.0;
					}

					res->float_list_val[i] = f1->float_list_val[i] / tmp;
				} else if (op == MOD) {
					res->float_list_val[i] = (int)f1->float_list_val[i] % (int)f2->float_list_val[i];
				}
			} else {
				res->float_list_val[i] = biggerType->float_list_val[i];
			}
		}
	} else if (f1->type == cFLOAT_LIST && f2->type == cFLOAT) {
		res->type = cFLOAT_LIST;
		res->size = f1->size;
		res->float_list_val = malloc(f1->size * sizeof(float));

		for (int i = 0; i < f1->size; ++i) {

			if (op == ADD) {
				res->float_list_val[i] = f1->float_list_val[i] + f2->float_val;
			} else if (op == SUB) {
				res->float_list_val[i] = f1->float_list_val[i] - f2->float_val;
			} else if (op == MUL) {
				res->float_list_val[i] = f1->float_list_val[i] * f2->float_val;
			} else if (op == DIV) {
				float tmp = f2->float_val;
				if (tmp == 0.0) {
					tmp = 1.0;
				}

				res->float_list_val[i] = f1->float_list_val[i] / tmp;
			} else if (op == MOD) {
				res->float_list_val[i] = (int)f1->float_list_val[i] % (int)f2->float_val;
			}

		}
	} else if (f2->type == cFLOAT_LIST && f1->type == cFLOAT) {
		res->type = cFLOAT_LIST;
		res->size = f2->size;
		res->float_list_val = malloc(f2->size * sizeof(float));

		for (int i = 0; i < f2->size; ++i) {
			if (op == ADD) {
				res->float_list_val[i] = f1->float_val + f2->float_list_val[i];
			} else if (op == SUB) {
				res->float_list_val[i] = f1->float_val - f2->float_list_val[i];
			} else if (op == MUL) {
				res->float_list_val[i] = f1->float_val * f2->float_list_val[i];
			} else if (op == DIV) {
				float tmp = f2->float_list_val[i];
				if (tmp == 0.0) {
					tmp = 1.0;
				}

				res->float_list_val[i] = f1->float_val / tmp;
			} else if (op == MOD) {
				res->float_list_val[i] = (int)f1->float_val % (int)f2->float_list_val[i];
			}
		}
	}

	return res;
}

mType * inv(mType * f1) {
	mType * result = malloc(sizeof(mType));

	if (f1->type == cFLOAT) {
		result->type = cFLOAT;
		result->float_val = f1->float_val;
	} else {
		result->type = cFLOAT_LIST;
		result->size = f1->size;
		result->float_list_val = malloc(f1->size * sizeof(float));
		for (int i = 0; i < f1->size; ++i) {
			result->float_list_val[i] = f1->float_list_val[f1->size - i - 1];
		}
	}
	return result;
}

mType * dup(mType * f1) {
	mType * result = malloc(sizeof(mType));

	if (f1->type == cFLOAT) {
		result->type = cFLOAT;
		result->float_val = f1->float_val;
	} else {
		result->type = cFLOAT_LIST;
		result->size = f1->size;
		result->float_list_val = malloc(f1->size * sizeof(float));
		for (int i = 0; i < f1->size; ++i) {
			result->float_list_val[i] = f1->float_list_val[i];
		}
	}
	return result;
}

float * parseValue(char * input) {
	int inputLen = strlen(input);
	int lastPosition = 0;
	mType * res = NULL;
	initStacks(1024);

	int elemCount = 0;
	char ** parts = split(input, '|', &elemCount);

	for (int i = 0; i < elemCount; ++i) {
		char * param = parts[i];

		if (param == NULL || strcmp(param, "") == 0) {
			continue;
		}

		if (param[0] == '[') {
			mType * tmp = parseList(param);
			pushObj(tmp);

			free(param);
			continue;
		}

		int paramLen = strlen(param);
		int hasCommas = 0;
		for (int j = 0; j < paramLen; ++j) {
			if (param[j] == ',') {
				hasCommas = 1;
				break;
			}
		}

		if (hasCommas == 1) {
			int nrCommas = 0;
			char ** commaParts = split(param, ',', &nrCommas);

			if (strcmp(commaParts[0], "file") == 0) {
				mType * tmp = file(commaParts[1]);
				pushObj(tmp);
			}

			for (int k = 0; k < nrCommas; ++k) {
				free(commaParts[k]);
			}
			free(commaParts);
		} else {

			if (isdigit(param[0]) || (strlen(param) > 1 && isdigit(param[1]) && param[0] == '-')) {
				float val = string2float(param);

				mType * tmp = malloc(sizeof(mType));
				tmp->type = cFLOAT;
				tmp->float_val = val;
				pushObj(tmp);
			} else if (param[0] == '=') {
				mType * tmp = popObj();

				mType * newtmp = deepCopy(tmp);

				vars[param[1] % maxVars] = newtmp;
				pushObj(tmp);
			} else if (param[0] == ':') {
				mType * tmp = vars[param[1] % maxVars];

				if (tmp == NULL) {
					continue;
				}

				mType * newTmp = deepCopy(tmp);
				pushObj(newTmp);
			} else if (strcmp(param, "+") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(ADD, f1, f2);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "-") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(SUB, f2, f1);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "*") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(MUL, f1, f2);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "/") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(DIV, f2, f1);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "%") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(MOD, f2, f1);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "inv") == 0) {
				mType * f1 = popObj();

				res = inv(f1);
				pushObj(res);

				destroyObj(f1);
			} else if (strcmp(param, "dup") == 0) {
				mType * f1 = popObj();
				mType * df1 = dup(f1);

				pushObj(f1);
				pushObj(df1);
			}
		}

		free(param);
	}

	free(parts);

	res = popObj();

	float * result = malloc(BUFFER_LEN * sizeof(float));

	for (int i = 0; i < BUFFER_LEN; ++i) {
		result[i] = 0.0;
	}

	if (res->type == cFLOAT) {
		result[0] = res->float_val;
	} else if (res->type == cFLOAT_LIST) {
		memcpy(result, res->float_list_val, res->size * sizeof(float));
	}

	destroyStacks();

	return result;
}

int main(int argc, char **argv) {
	char * test = "file,gigi.txt|dup|inv|+";
	test = "[1 10 100]|inv";
	test = "[1 2 3]|[1 10 100]|+|inv";
	test = "file,gigi.txt|[1 10 100]|+|inv|dup|+|8.7|+";
	test = "file,gigi.txt|[1 10 100]|+|inv|dup|+|-2.4|+";
	test = "file,gigi.txt|[1 10 100]|+|inv|dup|+";
	test = "[10 10 10]|dup|[1 10 100]|+|inv|-|dup|+";
	test = "[10 10 10]|dup|[1 10 100]|*|+|5|/|5|%|inv|10|*";
	test = "10|=a|3|+|:a|5|-|+";
	test = "10|=a|3|+|:a|5|-|-";

	float * res = parseValue(test);

	for (int i = 0; i < BUFFER_LEN; ++i) {
		printf("rez %f\n", res[i]);
	}

	exit(0);
}
