
// gcc -g -std=c99 stacklan.c ../utils.c ../parsers/stack_parser.c -lmatheval -o stacklan


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <matheval.h>
#include "../utils.h"
#include "../parser.h"

int main(int argc, char *argv[])
{
	int len = 3;
	float expectedResults[] = {1016.260010,
								1015.260010,
								0.0};
	char* exprs[] = {"2|3|+|5|*|'ana are mere'|10|b|-|12|13|15.45|a|+|-1.26|-",
			"2|3|+|5|*|'ana are mere'|'ana are mere'|11|b|b|-|12|13|15.45|a|+|-1.26|-",
			"+"};


	for (int i = 0; i < len; ++i) {
		float res = parseValue(exprs[i]);
		if (res != expectedResults[i]) {
			printf("TEST %d %f FAILED !\n", i, res);
		} else {
			printf("TEST %d passed !\n", i);
		}
	}

}


