#ifndef EFFECTS_H_
#define EFFECTS_H_

float sinGen(int index, int freq, unsigned long int lengthInSamples);

void sinGenArray(float * data, int freq, int samplingRate, unsigned long int lengthInSamples);

void silenceGenArray(float * data, unsigned long int lengthInSamples);

#endif /* EFFECTS_H_ */
