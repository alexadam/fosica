
#ifndef CACHE_H_
#define CACHE_H_

#define CACHE_OBJ_INCR 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
	void * data;
	int id;
} CACHED_OBJ;

typedef struct {
	CACHED_OBJ ** objs;
	int index;
	int capacity;
} CACHE;

CACHE * initCache(int initialSize);

void destroyCache(CACHE * cache);

void * getCachedObj(CACHE * cache, char * name);

void * getCachedObjById(CACHE * cache, int id);

void * putObjInCache(CACHE * cache, void * obj, size_t objSize, char * name);

void * putObjInCacheById(CACHE * cache, void * obj, size_t objSize, int id);

#endif /* CACHE_H_ */
