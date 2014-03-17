#include <stdio.h>
#include <stdlib.h>

#ifndef UTILS_H_
#define UTILS_H_

char * localconcat(char *s1, char *s2);

char * trimwhitespace(char *str);

char * readFile(char * fileName);

#endif /* UTILS_H_ */
