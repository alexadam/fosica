#ifndef EFFECTS_H_
#define EFFECTS_H_

float sinGen(int index, int freq, unsigned long int lengthInSamples);

void sinGenArray(float * data, int freq, int samplingRate, unsigned long int lengthInSamples);

float * sinGenA(int freq, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples);

float * sinGenPhase(int freq, int phase, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples);

void silenceGenArray(float * data, unsigned long int lengthInSamples);

#endif /* EFFECTS_H_ */
