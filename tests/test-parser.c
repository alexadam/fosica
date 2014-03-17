#include <stdio.h>
#include <stdlib.h>
#include "../utils.h"

/**
 *
 *gcc  -std=c99 -o test-parser ../utils.c test-parser.c
 *
 */

typedef struct varray_t {
	void ** memory;
	size_t allocated;
	size_t used;
	int index;
} varray;

varray * varray_init(int inititalSize) {
	varray * array = (varray*) malloc(inititalSize * sizeof(void*));
	array->memory = NULL;
	array->allocated = 0;
	array->used = 0;
	array->index = -1;
	return array;
}

void varray_push(varray * array, void *data) {
	size_t toallocate;
	size_t size = sizeof(void*);
	if ((array->allocated - array->used) < size) {
		toallocate = array->allocated == 0 ? size : (array->allocated * 2);
		array->memory = realloc(array->memory, toallocate);
		array->allocated = toallocate;
	}

	array->memory[++array->index] = data;
	array->used = array->used + size;
}

int varray_length(varray *array) {
	return array->index + 1;
}

void varray_clear(varray *array) {
	int i;
	for (i = 0; i < varray_length(array); i++) {
		array->memory[i] = NULL;
	}
	array->used = 0;
	array->index = -1;
}

void varray_free(varray *array) {
	free(array->memory);
	free(array);
}

void * varray_get(varray *array, int index) {
	if (index < 0 || index > array->index)
		return NULL;

	return array->memory[index];
}

void varray_insert(varray *array, int index, void *data) {
	if (index < 0 || index > array->index)
		return;

	array->memory[index] = data;
}

/**
 *
 *
 */


int main(int argc, char **argv) {

	char * string = readFile("../merge.txt");
	if (string) {

		unsigned long bufLen = strlen(string);
		unsigned long lastPosition = 0;

		for (int i = 0; i < bufLen; ++i) {

			if (string[i] == '\n') {

				char *to = (char*) malloc(i - lastPosition);
				strncpy(to, string + lastPosition, i);

				printf("\nYYYYYYYYY: %s", to);
//				free(to);
				i++;
				lastPosition = i;
			}
		}

		printf("au fost %d\n", bufLen);
		free(string);
	}

	return 0;

}
