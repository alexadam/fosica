/**
 gcc -std=c99 -Wall -o term term.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	while (1) {

		char c = getchar();

		if (c == 3) exit(0);

		printf("%c", c);

	}

}
