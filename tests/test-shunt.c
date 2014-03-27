
// gcc -std=c99 test-shunt.c -o test-shunt

/* The authors of this work have released all rights to it and placed it
in the public domain under the Creative Commons CC0 1.0 waiver
(http://creativecommons.org/publicdomain/zero/1.0/).

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Retrieved from: http://en.literateprograms.org/Shunting_yard_algorithm_(C)?oldid=18970
*/


#include<stdlib.h>
#include<stdio.h>
#include<ctype.h>

#include "../utils.h"

#define MAXOPSTACK 64
#define MAXNUMSTACK 64

float eval_uminus(float a1, float a2)
{
	return -a1;
}
float eval_exp(float a1, float a2)
{
	return a2<0 ? 0 : (a2==0?1:a1*eval_exp(a1, a2-1));
}
float eval_mul(float a1, float a2)
{
	return a1*a2;
}
float eval_div(float a1, float a2)
{
	if(!a2) {
		fprintf(stderr, "ERROR: Division by zero\n");
		exit(EXIT_FAILURE);
	}
	return a1/a2;
}
float eval_mod(float a1, float a2)
{
	if(!a2) {
		fprintf(stderr, "ERROR: Division by zero\n");
		exit(EXIT_FAILURE);
	}
	return (int)a1 % (int)a2;
}
float eval_add(float a1, float a2)
{
	return a1+a2;
}
float eval_sub(float a1, float a2)
{
	return a1-a2;
}


enum {ASSOC_NONE=0, ASSOC_LEFT, ASSOC_RIGHT};

struct op_s {
	char op;
	int prec;
	int assoc;
	int unary;
	float (*eval)(float a1, float a2);
} ops[]={
	{'_', 10, ASSOC_RIGHT, 1, eval_uminus},
	{'^', 9, ASSOC_RIGHT, 0, eval_exp},
	{'*', 8, ASSOC_LEFT, 0, eval_mul},
	{'/', 8, ASSOC_LEFT, 0, eval_div},
	{'%', 8, ASSOC_LEFT, 0, eval_mod},
	{'+', 5, ASSOC_LEFT, 0, eval_add},
	{'-', 5, ASSOC_LEFT, 0, eval_sub},
	{'(', 0, ASSOC_NONE, 0, NULL},
	{')', 0, ASSOC_NONE, 0, NULL}
};

struct op_s *getop(char ch)
{
	int i;
	for(i=0; i<sizeof ops/sizeof ops[0]; ++i) {
		if(ops[i].op==ch) return ops+i;
	}
	return NULL;
}

struct op_s *opstack[MAXOPSTACK];
int nopstack=0;

float numstack[MAXNUMSTACK];
int nnumstack=0;

void push_opstack(struct op_s *op)
{
	if(nopstack>MAXOPSTACK-1) {
		fprintf(stderr, "ERROR: Operator stack overflow\n");
		exit(EXIT_FAILURE);
	}
	opstack[nopstack++]=op;
}

struct op_s *pop_opstack()
{
	if(!nopstack) {
		fprintf(stderr, "ERROR: Operator stack empty\n");
		exit(EXIT_FAILURE);
	}
	return opstack[--nopstack];
}

void push_numstack(float num)
{
	if(nnumstack>MAXNUMSTACK-1) {
		fprintf(stderr, "ERROR: Number stack overflow\n");
		exit(EXIT_FAILURE);
	}
	numstack[nnumstack++]=num;
}

float pop_numstack()
{
	if(!nnumstack) {
		fprintf(stderr, "ERROR: Number stack empty\n");
		exit(EXIT_FAILURE);
	}
	return numstack[--nnumstack];
}


void shunt_op(struct op_s *op)
{
	struct op_s *pop;
	float n1, n2;
	if(op->op=='(') {
		push_opstack(op);
		return;
	} else if(op->op==')') {
		while(nopstack>0 && opstack[nopstack-1]->op!='(') {
			pop=pop_opstack();
			n1=pop_numstack();
			if(pop->unary) push_numstack(pop->eval(n1, 0));
			else {
				n2=pop_numstack();
				push_numstack(pop->eval(n2, n1));
			}
		}
		if(!(pop=pop_opstack()) || pop->op!='(') {
			fprintf(stderr, "ERROR: Stack error. No matching \'(\'\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	if(op->assoc==ASSOC_RIGHT) {
		while(nopstack && op->prec<opstack[nopstack-1]->prec) {
			pop=pop_opstack();
			n1=pop_numstack();
			if(pop->unary) push_numstack(pop->eval(n1, 0));
			else {
				n2=pop_numstack();
				push_numstack(pop->eval(n2, n1));
			}
		}
	} else {

		while(nopstack && op->prec<=opstack[nopstack-1]->prec) {
			pop=pop_opstack();
			n1=pop_numstack();
			if(pop->unary) push_numstack(pop->eval(n1, 0));
			else {
				n2=pop_numstack();
				push_numstack(pop->eval(n2, n1));
			}
		}
	}
	push_opstack(op);
}


int main(int argc, char *argv[])
{
	char *expr;
	char *tstart=NULL;
	struct op_s startop={'X', 0, ASSOC_NONE, 0, NULL};	/* Dummy operator to mark start */
	struct op_s *op=NULL;
	float n1, n2;
	struct op_s *lastop=&startop;

//	if(argc<2) {
//		fprintf(stderr, "Usage: %s <expression>\n", argv[0]);
//		return EXIT_FAILURE;
//	}


	expr = "(3+2)*sin(10,250,56)+file(\"sound/ana are mere.txt\",150)";
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

	for (int i = 0; i < expLen; ++i) {
		if (startFunc == -1 && isalpha(expr[i]) )  { //&& !isspace(expr[i]) && expr[i] != '(' && expr[i] != ')') {
			startFunc = i;
		} else if (startFunc != -1 && expr[i] == '(') {
			if (nrParens == 0) {
				posFirstParen = i;
			}
			nrParens++;

			funcName = malloc((i - startFunc + 1) * sizeof(char));
			memcpy(funcName, expr + startFunc, (i - startFunc));
			funcName[i - startFunc] = '\0';

			lastInterestingPos = i+1;

		} else if (startFunc != -1 && expr[i] == ',') {

			char * param = malloc((i - lastInterestingPos + 1) * sizeof(char));
			memcpy(param, expr + lastInterestingPos, (i - lastInterestingPos));
			param[i - lastInterestingPos] = '\0';

			lastInterestingPos = i+1;

			rawParams[currentParam] = param;
			currentParam++;

		} else if (startFunc != -1 && expr[i] == ')') {
			nrParens--;

			char * param = malloc((i - lastInterestingPos + 1) * sizeof(char));
			memcpy(param, expr + lastInterestingPos, (i - lastInterestingPos));
			param[i - lastInterestingPos] = '\0';

			lastInterestingPos = i+1;

			rawParams[currentParam] = param;
			currentParam++;

			if (nrParens == 0) {
				char * funcSig = malloc((i - startFunc + 2) * sizeof(char));
				memcpy(funcSig, expr + startFunc, (i - startFunc + 1));
				funcSig[i - startFunc + 1] = '\0';

				for (int j = 0; j < currentParam; ++j) {
					char * curRawParam = rawParams[j];

					if (curRawParam[0] == '"') {
						solvedParams[j] = curRawParam;
						continue;
					} else {

					}

				}

				startFunc = -1;
				endFunc = -1;
				nrParens = 0;
				posFirstParen = -1;
				currentParam = 0;
			}
		}
	}

	return 0;

	expr = "2+3.5-(3.1415 * 2 * 250/1500)";

	for(; *expr; ++expr) {
		if(!tstart) {

			if((op=getop(*expr))) {
				if(lastop && (lastop==&startop || lastop->op!=')')) {
					if(op->op=='-') op=getop('_');
					else if(op->op!='(') {
						fprintf(stderr, "ERROR: Illegal use of binary operator (%c)\n", op->op);
						exit(EXIT_FAILURE);
					}
				}
				shunt_op(op);
				lastop=op;
			} else if(isdigit(*expr)) tstart=expr;
			else if(!isspace(*expr) && *expr != '.') {
				int nrParens = 0;

				fprintf(stderr, "ERROR: Syntax error %s \n", expr);
//				return EXIT_FAILURE;
			}
		} else {
			if(isspace(*expr)) {
				push_numstack(atof(tstart));
				tstart=NULL;
				lastop=NULL;
			} else if((op=getop(*expr))) {
				push_numstack(atof(tstart));
				tstart=NULL;
				shunt_op(op);
				lastop=op;
			} else if(!isdigit(*expr) && *expr != '.') {
				fprintf(stderr, "ERROR: Syntax error 22 %s \n", expr);
//				return EXIT_FAILURE;
			}
		}
	}
	if(tstart) {
		push_numstack(atof(tstart));
	}

	while(nopstack) {
		op=pop_opstack();
		n1=pop_numstack();
		if(op->unary) push_numstack(op->eval(n1, 0));
		else {
			n2=pop_numstack();
			push_numstack(op->eval(n2, n1));
		}
	}
	if(nnumstack!=1) {
		fprintf(stderr, "ERROR: Number stack has %d elements after evaluation. Should be 1.\n", nnumstack);
		return EXIT_FAILURE;
	}
	printf("%f\n", numstack[0]);

	return EXIT_SUCCESS;
}

