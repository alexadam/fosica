/*
 * fosica.h
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */

#ifndef FOSICA_H_
#define FOSICA_H_

#include <stdlib.h>

typedef struct
{
    float * data;
    int nrOfFrames;
    int nrOfChannels;
    int samplingRate;
    int left_phase;
    int right_phase;
    int dataLength;
}
sndData;

typedef struct
{
    char * instructions;
    float * buffer;
    int start; //TODO long int
    int stop;
    int lastStart;
    int lastStop;
    int lastHash;
    int bufferLenInSamples;
}
sequence;

typedef struct
{
    sequence * sequences;
    int nrOfSeqs;
    int totalNrOfSeqs;
    int index;
    int tempo;
    int forceStopSound;
}
track;

typedef struct {
    track * cTrack;
    sndData * data;
} threadArgContainer;

typedef struct
{
    sndData snd;
    char * name;
}
cachedSnd;

#endif /* FOSICA_H_ */
