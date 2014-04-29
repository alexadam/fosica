
#ifndef PARSER_H_
#define PARSER_H_

//float parseValue(char * input);

//float * parseValue(char * input, int bufferLen);

float getValue(char * input, int bufferLen);

float * parseValue(char * input, int sequenceIndex, int abufferLen);

#endif /* PARSER_H_ */
