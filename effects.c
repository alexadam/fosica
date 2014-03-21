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
