#include "cache.h"
#include "../utils/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

CACHE * initCache(int initialSize) {
	CACHE * cache = malloc(sizeof(CACHED_OBJ));

	cache->capacity = initialSize;
	cache->index = 0;
	cache->objs = malloc(initialSize * sizeof(CACHED_OBJ *));

	for (int i = 0; i < initialSize; ++i) {
		cache->objs[i] = malloc(sizeof(CACHED_OBJ));
	}

	return cache;
}

void destroyCache(CACHE * cache) {
	for (int i = 0; i < cache->capacity; ++i) {
		if (cache->objs[i]->data != NULL) {
			free(cache->objs[i]->data);
		}
		free(cache->objs[i]);
	}
	free(cache);
}

void * getCachedObj(CACHE * cache, char * name) {
	int cName = hash(name);
	return getCachedObjById(cache, cName);
}

void * getCachedObjById(CACHE * cache, int id) {
	for (int i = 0; i < cache->index; ++i) {
		if (cache->objs[i]->id == id) {
			return cache->objs[i]->data;
		}
	}

	return NULL;
}

void * putObjInCache(CACHE * cache, void * obj, size_t objSize, char * name) {
	int cName = hash(name);
	return putObjInCacheById(cache, obj, objSize, cName);
}

void * putObjInCacheById(CACHE * cache, void * obj, size_t objSize, int id) {

	//override existing element
	for (int i = 0; i < cache->index; ++i) {
		if (cache->objs[i]->id == id) {
			free(cache->objs[i]->data);
			free(cache->objs[i]);

			cache->objs[i] = malloc(sizeof(CACHED_OBJ));

			cache->objs[i]->data = malloc(objSize);
			memcpy(cache->objs[i]->data, obj, objSize);
			cache->objs[i]->id = id;

			void * rez = cache->objs[i]->data;

			return rez;
		}
	}

	//resize
	if (cache->index == cache->capacity) {
		cache->objs = realloc(cache->objs, (cache->capacity + 10) * sizeof(CACHED_OBJ *));

		if (!cache->objs) {
			printf("ERROR - cannot realloc\n");
			return NULL;
		}

		for (int i = cache->index; i < (cache->index + 10); ++i) {
			cache->objs[i] = malloc(sizeof(CACHED_OBJ));
			cache->objs[i]->data = NULL;
		}

		cache->capacity += 10;

	}

	//add new element
	cache->objs[cache->index]->data = malloc(objSize);
	memcpy(cache->objs[cache->index]->data, obj, objSize);
	cache->objs[cache->index]->id = id;

	void * rez = cache->objs[cache->index]->data;
	cache->index++;

	return rez;
}
