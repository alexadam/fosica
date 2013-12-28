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
gcc -std=c99 -lfluidsynth -o test fluidsynth_callback.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <fluidsynth.h>

/* The structure with the effects data. This example simply applies a
 * linear gain the to synthesizer output. */
struct fx_data_t {
	fluid_synth_t* synth;
	float gain;
} fx_data_t;

/* This function implements the callback function of the audio driver
 * (see new_fluid_audio_driver2 below). The data argument is a pointer
 * to your private data structure. 'len' is the number of samples in
 * the buffers. 'nin' and 'nout' are the number of input and output
 * audio buffers. 'in' and 'out' are an array of float buffers with
 * the samples. The audio driver fills the 'in' buffers the incoming
 * audio. The 'out' buffers should be filled by your function. The
 * 'out' buffers will be sent to the audio output of the sound card.
 *
 * IMPORTANT NOTE: The API was designed to be generic but none of the
 * audio drivers currently handles audio input. Either 'nin' will be
 * zero, or the buffers will be filled with zero samples.
 */

int fx_function(void* data, int len, int nin, float** in, int nout, float** out) {
	struct fx_data_t* fx_data = (struct fx_data_t*) data;
	int i, k;
	float* out_i;

	/* Call the synthesizer to fill the output buffers with its
	 * audio output. */
	if (fluid_synth_process(fx_data->synth, len, nin, in, nout, out) != 0) {
		/* Some error occured. Very unlikely to happen, though. */
		return -1;
	}

	/* Apply your effects here. In this example, the gain is
	 * applied to all the output buffers. */
	for (i = 0; i < nout; i++) {
		out_i = out[i];
		for (k = 0; k < len; k++) {
			out_i[k] *= 1.0; //fx_data->gain;
		}
	}

	return 0;
}

void sequ(fluid_synth_t * synth);
void sequ(fluid_synth_t * synth) {
	while (1) {
		fluid_synth_noteon(synth, 0, 60, 100);
		fluid_synth_noteon(synth, 0, 63, 100);
		fluid_synth_noteon(synth, 0, 65, 100);
		sleep(1);
	}
}

int main(int argc, char** argv) {
	fluid_settings_t* settings;
	fluid_synth_t* synth = NULL;
	fluid_audio_driver_t* adriver = NULL;
	struct fx_data_t fx_data;

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

	fluid_settings_setstr(settings, "audio.driver", "alsa");

	fluid_settings_setstr(settings, "audio.file.type", "wav");
	fluid_settings_setnum(settings, "synth.sample-rate", 44100);
	fluid_settings_setstr(settings, "audio.file.format", "s16");
	fluid_settings_setstr(settings, "audio.file.endian", "cpu");
	fluid_settings_setstr(settings, "audio.file.name", "a.wav");
	fluid_settings_setint(settings, "audio.period-size", 16383);
	fluid_settings_setint(settings, "audio.realtime-prio", 99);

	/* Load the soundfont */
	if (fluid_synth_sfload(synth, "/home/alex/Desktop/sf/sf.sf2", 1) == -1) {
		fprintf(stderr, "Failed to load the SoundFont\n");
		goto cleanup;
	}

	/* Fill in the data of the effects unit */
	fx_data.synth = synth;
	fx_data.gain = 1;

	adriver = new_fluid_audio_driver2(settings, fx_function, (void*) &fx_data);
	if (adriver == NULL) {
		fprintf(stderr, "Failed to create the audio driver\n");
		goto cleanup;
	}

	sequ(synth);

	printf("Press \"Enter\" to stop: ");
	fgetc(stdin);
	printf("done\n");

	cleanup:

	if (adriver) {
		delete_fluid_audio_driver(adriver);
	}
	if (synth) {
		delete_fluid_synth(synth);
	}
	if (settings) {
		delete_fluid_settings(settings);
	}

}
