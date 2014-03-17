#include <stdio.h>
#include <stdlib.h>

#include "../midiutils.h"


/**
 *
 *  gcc -std=c99 -o fluidsynth_test ../midiutils.c midiutils_test.c -L/usr/lib64 -lfluidsynth -Wl,-rpath -Wl,/usr/lib64
 *
 */

int main(int argc, char **argv) {

	/**
	 * Generate wav from midi events
	 */

	midievent ev1;
	ev1.channel = 0;
	ev1.key = 60;
	ev1.velocity = 100;

	midievent ev2;
	ev2.channel = 0;
	ev2.key = 62;
	ev2.velocity = 100;

	midievent ev3;
	ev3.channel = 0;
	ev3.key = 64;
	ev3.velocity = 100;

	midievent events[3] = { ev1, ev2, ev3};

	eventToWavFluidSynth(events, 3, 2000, 44100, "result.wav", "../sf.sf2");



//	/**
//	 * midicomp test events -> text
//	 */
//	eventToMidiText(events, 1, 2000, "miditxt.txt");
//
//	/**
//	 * midicomp text -> midi
//	 */
//	midiTextToMidi("../libs/midicomp -c ", "miditxt.txt", "midi.mid");
//
//	/**
//	 * fluidsynth midi -> wav
//	 */
//	midiToWav("midi.mid", "aaa.wav", "../sf.sf2");
}
