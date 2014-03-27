
// gcc -g -std=c99 mega.c ../utils.c -lmatheval -o mega


#include <stdlib.h>
#include <stdio.h>
#include<ctype.h>
#include <string.h>
#include <assert.h>
#include <matheval.h>
#include "../utils.h"

float a(int v1, int v2, float v3) {
	return 1000.0;
}

float b(float v3) {
	return 10.0;
}

float c() {
	return 100.0;
}


float mathEval(char * expr) {

//	void * f = evaluator_create(expr);
//	assert(f);
//
//	double res = evaluator_evaluate_x(f, 0);
//	evaluator_destroy(f);
	float res = 100;
	return (float)res;
}

float evalExpr(char * expr);

char * replaceFunc(char * expr) {
	int expLen = strlen(expr);

	int startFunc = -1;
	int endFunc = -1;
	int nrParens = 0;
	int posFirstParen = -1;
	int lastInterestingPos = 0;
	char * funcName = NULL;
	char * rawParams[10];
	char * solvedParams[10];
	int currentParam = 0;

	char ** replaceMe = (char **)malloc(10 * sizeof(char*));
	char ** replaceWith = (char **)malloc(10 * sizeof(char*));
	int crepl = 0;

	for (int i = 0; i < expLen; ++i) {
		if (startFunc == -1 && isalpha(expr[i]) )  {
			startFunc = i;
		} else if (startFunc != -1 && expr[i] == '(') {
			if (nrParens == 0) {
				posFirstParen = i;

				funcName = malloc((i - startFunc + 1) * sizeof(char));
				memcpy(funcName, expr + startFunc, (i - startFunc));
				funcName[i - startFunc] = '\0';

				lastInterestingPos = i+1;
			}
			nrParens++;

		} else if (startFunc != -1 && expr[i] == ',') {

			if (nrParens > 1) {
				continue;
			}

			if (i - lastInterestingPos > 0) {
				char * param = malloc((i - lastInterestingPos + 1) * sizeof(char));
				memcpy(param, expr + lastInterestingPos, (i - lastInterestingPos));
				param[i - lastInterestingPos] = '\0';

				lastInterestingPos = i+1;

				rawParams[currentParam] = param;
				currentParam++;
			}

		} else if (startFunc != -1 && expr[i] == ')') {
			nrParens--;

			if (nrParens == 0) {
				if (i - lastInterestingPos > 0) {
					char * param = malloc((i - lastInterestingPos + 1) * sizeof(char));
					memcpy(param, expr + lastInterestingPos, (i - lastInterestingPos));
					param[i - lastInterestingPos] = '\0';

					lastInterestingPos = i+1;

					rawParams[currentParam] = param;
					currentParam++;
				}

				char * funcSig = malloc((i - startFunc + 2) * sizeof(char));
				memcpy(funcSig, expr + startFunc, (i - startFunc + 1));
				funcSig[i - startFunc + 1] = '\0';

				for (int j = 0; j < currentParam; ++j) {
					char * curRawParam = rawParams[j];

					if (curRawParam[0] == '"') {
						solvedParams[j] = curRawParam;
						continue;
					}

					float tmp = evalExpr(curRawParam);
					solvedParams[j] =  float2string(tmp);
				}

				if (strcmp(funcName, "a") == 0) {
					float result = (float) a(string2int(solvedParams[0]), string2int(solvedParams[1]), string2float(solvedParams[2]));
					char * res = float2string(result);

					replaceMe[crepl] = funcSig;
					replaceWith[crepl] = res;

					crepl++;
				} else if (strcmp(funcName, "b") == 0) {
					float result = (float) b(string2float(solvedParams[0]));
					char * res = float2string(result);

					replaceMe[crepl] = funcSig;
					replaceWith[crepl] = res;

					crepl++;
				} else if (strcmp(funcName, "c") == 0) {
					float result = (float) c();
					char * res = float2string(result);

					printf("p %p\n", &res);

					replaceMe[crepl] = funcSig;
					replaceWith[crepl] = res;

					crepl++;
				}

				startFunc = -1;
				free(funcName);
				endFunc = -1;
				nrParens = 0;
				posFirstParen = -1;
				currentParam = 0;
				lastInterestingPos = 0;

				for (int y = 0; y < currentParam; ++y) {
					free(rawParams[y]);
					free(solvedParams[y]);
				}
//				free(rawParams); TODO malloc them ?
//				free(solvedParams);
			}
		}
	}

	for (int i = 0; i < crepl; ++i) {
		expr = str_replace(expr, replaceMe[i], replaceWith[i]);
	}

	for (int y = 0; y < crepl; ++y) {
		printf("ajung aici\n");
		free(replaceMe[y]);
		printf("p rw %p\n", &replaceWith[y]);
		free(replaceWith[y]);
	}
//	free(replaceMe);
//	free(replaceWith);

	return expr;
}

float evalExpr(char * expr) {
	char * oldp = expr;
	expr = replaceFunc(expr);

	if (expr != oldp) {
		free(oldp);
	}

	float res = mathEval(expr);
	free(expr);
	return res;
}

float eval(char * input) {

	char * tmp1 =  ulint2string(1000);
	char * tmp2 = int2string(100);

	char * instructions = str_replace(input, "#globalIndex", tmp1);
	char * oldp = instructions;
	instructions = str_replace(instructions, "#tempo", tmp2);

	free(oldp);
	free(tmp1);
	free(tmp2);

	float res = evalExpr(instructions);
	free(instructions);

	return res;
}

int main(int argc, char *argv[])
{
	for (int var = 0; var < 1000; ++var) {
//		sleep(1);

		char *expr = "(3+2)*a(10,b(b(10)),56)+b(99)+c()";

		float res = eval(expr);

		printf("REZ = %f\n", res);

	}

}


