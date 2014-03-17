#ifndef MIDIUTILS_H_
#define MIDIUTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <fluidsynth.h>

typedef struct {
	int channel;
	int key;
	int velocity;
} midievent;

int eventToWavFluidSynth(midievent * midievents, int nrOfEvents, int durationMillis, int samplingRate, char * fileName, char * soundFontPath);

//int eventToMidiText(midievent * midievents, int nrOfEvents, int durationMillis, char * fileName);
//
//int midiTextToMidi(char * toolName, char * midiTextFileName, char * wavFileName);
//
//int midiToWav(char * midiFileName, char * wavFileName, char * soundFontFileName);

#endif
