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

	char * string = readFileToBuffer("../merge.txt");

		if (string) {

			int trackHeader = 1;
			int eventStart = 2;
			int eventStop = 3;
			int eventInstr = 4;
			int lastEvent = -1;

			unsigned long bufLen = strlen(string);
			unsigned long lastPosition = 0;

			for (int i = 0; i < bufLen; ++i) {

				if (string[i] == '\n' || i == (bufLen - 1)) {

					char * to = (char * ) malloc(i - lastPosition);
					to = substring(string, lastPosition, ((i == (bufLen - 1)) ? i - lastPosition + 1 : i - lastPosition));

					if (strstr(to, "@track")) {
						lastEvent = trackHeader;
						printf("NEW Track \n");
					} else if (lastEvent == trackHeader	|| lastEvent == eventInstr) {
						lastEvent = eventStart;
						long found = strtol(&string[lastPosition], NULL, 0);
						printf("AM gasit %d\n", found);
					} else if (lastEvent == eventStart) {
						lastEvent = eventStop;
						long found = strtol(&string[lastPosition], NULL, 0);
						printf("AM gasit %d\n", found);
					} else if (lastEvent == eventStop) {
						lastEvent = eventInstr;
						printf("INSTRUCTIUNE %s\n", to);
					}

					free(to);
					lastPosition = i+1;
				}
			}

			printf("au fost %d\n", bufLen);
			free(string);
		}

}
