#include "../cache/cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**

 	gcc -g -pedantic -Wall -Wextra -lefence -std=c99 -o test-cache ../utils/utils.c ../cache/cache.c test-cache.c

 */

typedef struct {
	float a;
	int b;
} AAA;

int main(int argc, char **argv) {

	CACHE * c = initCache(10);

	char * ts = "string 1";
	char * ts2 = "string 2";
	float ts3 = 1234.5678;
	AAA * ts4 = malloc(sizeof(AAA));
	ts4->a = 1.2;
	ts4->b = 3;

	putObjInCacheById(c, ts, (strlen(ts)+1) * sizeof(char), 12);
	putObjInCacheById(c, ts2, (strlen(ts2)+1) * sizeof(char), 8);
	putObjInCacheById(c, &ts3, sizeof(float), 4);
	putObjInCacheById(c, ts4, sizeof(ts4), 1);

	char * ss = getCachedObjById(c, 8);
	AAA * sa = getCachedObjById(c, 1);

	printf("res1 %f %d\n", sa->a, sa->b);
	printf("res2 %s %f\n", ss, *((float *)getCachedObjById(c, 4)));

	float * src = malloc(128 * sizeof(float));

	for (int i = 0; i < 128; ++i) {
		src[i] = (float)i;
		putObjInCacheById(c, &src[i], sizeof(float), i);
	}

	printf("SRC %f\n", *((float *)getCachedObjById(c, 4)));
	printf("SRC %f\n", *((float *)getCachedObjById(c, 40)));
	printf("SRC %f\n", *((float *)getCachedObjById(c, 14)));
	printf("SRC %f\n", *((float *)getCachedObjById(c, 24)));
	printf("SRC %f\n", *((float *)getCachedObjById(c, 74)));

	destroyCache(c);

}
