#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

float sinGen(int index, int freq, unsigned long int lengthInSamples) {
	if (lengthInSamples == 0) {
		lengthInSamples = 1;
	}

	return sin(6.283 * freq * index / lengthInSamples);
}

void sinGenArray(float * data, int freq, int samplingRate, unsigned long int lengthInSamples) {
    if (samplingRate == 0)
        return;

    for (int i = 0; i< lengthInSamples; i++) {
        data[i] = sin(2 * 3.1415 * freq * i / samplingRate);
    }
}

void silenceGenArray(float * data, unsigned long int lengthInSamples) {
    for (int i = 0; i< lengthInSamples; i++) {
        data[i] = 0.0;
    }
}

float * sinGenA(int freq, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples) {
//	return sinGenPhase(freq, 0, startIndex, nrOfSamples, totalNrOfSamples);

	float * res = malloc(nrOfSamples * sizeof(float));

		for (int i = 0; i < nrOfSamples; ++i) {
			res[i] = sin((6.283 * freq * (startIndex + i) )/ totalNrOfSamples);
		}

		return res;
}

float * sinGenPhase(int freq, int phase, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples) {
	float * res = malloc(nrOfSamples * sizeof(float));

	for (int i = 0; i < nrOfSamples; ++i) {
		res[i] = sin((6.283 * freq * (startIndex + i) + phase * 3.1415 / 180)/ totalNrOfSamples);
	}

	return res;
}
