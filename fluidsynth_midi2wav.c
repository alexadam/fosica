/**
 * how to compile fluidsynth:
 *
 * cd fluidsynth source folder
 * mkdir build
 * cd build
 * cmake ..
 * make
 *
 * rename src/libfluidsynth.so.5.2.1 to libcustom.so
 *
 * gcc -L/home/alex/Desktop/fluidsynth-1.1.6/build/src -lcustom -o test e.c
 *
 */

/*

gcc -std=c99 -o test fluidsynth_midi2wav.c -L/usr/lib64 -lfluidsynth -Wl,-rpath -Wl,/usr/lib64

*/

#include <stdio.h>
#include <stdlib.h>
#include <fluidsynth.h>

typedef struct {
	int channel;
	int key;
	int velocity;
} midievent;

int gen(midievent * midievents, int nrOfEvents, int durationMillis, int samplingRate, char * fileName, char * soundFontPath);
int gen(midievent * midievents, int nrOfEvents, int durationMillis, int samplingRate, char * fileName, char * soundFontPath) {
	fluid_settings_t* settings;
	fluid_synth_t* synth = NULL;

	settings = new_fluid_settings();

	if (settings == NULL) {
		fprintf(stderr, "Failed to create the settings\n");
		goto cleanup;
	}

	/* Create the synthesizer */
	synth = new_fluid_synth(settings);
	if (synth == NULL) {
		fprintf(stderr, "Failed to create the synthesizer\n");
		goto cleanup;
	}

	fluid_settings_setstr(settings, "audio.driver", "file");
	fluid_settings_setstr(settings, "audio.file.type", "wav");
	fluid_settings_setnum(settings, "synth.sample-rate", samplingRate);
	fluid_settings_setstr(settings, "audio.file.format", "s16");
	fluid_settings_setstr(settings, "audio.file.endian", "cpu");
	fluid_settings_setstr(settings, "audio.file.name", fileName);
	fluid_settings_setint(settings, "audio.period-size", 16383);
	fluid_settings_setint(settings, "audio.realtime-prio", 99);

	if (fluid_synth_sfload(synth, soundFontPath, 1) == -1) {
		fprintf(stderr, "Failed to load the SoundFont\n");
		goto cleanup;
	}

    new_fluid_audio_driver(settings, synth);

    for (int i = 0; i < nrOfEvents; i++) {
    	fluid_synth_noteon(synth, midievents[i].channel, midievents[i].key, midievents[i].velocity);
    }

    usleep(durationMillis * 1000);

    return 1;

	cleanup:

	if (synth) {
		delete_fluid_synth(synth);
	}
	if (settings) {
		delete_fluid_settings(settings);
	}

	return 0;

}

int main() {
	midievent ev1;
	ev1.channel = 0;
	ev1.key = 60;
	ev1.velocity = 100;

	midievent events[1] = {ev1};

	gen(events, 1, 2000, 44100, "gigi.wav", "/home/alex/Desktop/sf/sf.sf2");
}
