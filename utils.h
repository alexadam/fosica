#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifndef UTILS_H_
#define UTILS_H_

char * localconcat(char *s1, char *s2);

char * trimwhitespace(char *str);

char * readFileToBuffer(char * fileName);

char * substring(const char * str, size_t begin, size_t len);

char * str_replace( const char *string, const char *substr, const char *replacement );

char ** str_split(char* a_str, const char a_delim, int * elemCount);

int string2int(char * input);

char * int2string(int input);

char * ulint2string(unsigned long int input);

long string2long(char * input);

float string2float(char * input);

#endif /* UTILS_H_ */
