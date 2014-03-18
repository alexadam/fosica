#include <stdio.h>
#include <stdlib.h>

#ifndef UTILS_H_
#define UTILS_H_

char * localconcat(char *s1, char *s2);

char * trimwhitespace(char *str);

char * readFileToBuffer(char * fileName);

char * substring(const char * str, size_t begin, size_t len);

#endif /* UTILS_H_ */
