#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "midiutils.h"

int eventToWavFluidSynth(midievent * midievents, int nrOfEvents, int durationMillis, int samplingRate, char * fileName, char * soundFontPath) {
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

//int eventToMidiText(midievent * midievents, int nrOfEvents, int durationMillis, char * fileName) {
//
//	FILE *f = fopen(fileName, "w");
//	if (f == NULL)
//	{
//	    printf("Error opening file!\n");
//	    return 1;
//	}
//
////	fprintf(f, "MFile 1 1 384\nMTrk\n0 Meta SeqName \"Track 1\"\n0 Tempo 500000\n0 TimeSig 4/4 32 99\n");
//	fprintf(f, "MFile 1 1 384\nMTrk\n");
//
//
//	for (int i = 0; i < nrOfEvents; i++) {
//		fprintf(f, "0 On ch=%d n=%d v=%d\n", midievents[i].channel, midievents[i].key, midievents[i].velocity);
//		fprintf(f, "2000 Off ch=%d n=%d v=%d\n", midievents[i].channel, midievents[i].key, midievents[i].velocity);
//	}
//
//	fprintf(f, "\nTrkEnd\n");
////	fprintf(f, "%d Meta TrkEnd\nTrkEnd\n", durationMillis);
//
//
//	return 0;
//}
//
//char * localconcat(char *s1, char *s2)
//{
//    size_t len1 = strlen(s1);
//    size_t len2 = strlen(s2);
//    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
//    //in real code you would check for errors in malloc here
//    memcpy(result, s1, len1);
//    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
//    return result;
//}
//
//int midiTextToMidi(char * toolName, char * midiTextFileName, char * wavFileName) {
//	return system(localconcat(toolName, localconcat(midiTextFileName, localconcat(" ", wavFileName))));
//}
//
//int midiToWav(char * midiFileName, char * wavFileName, char * soundFontFileName) {
//	//fluidsynth -F out.wav /usr/share/sounds/sf2/FluidR3_GM.sf2 myfile.mid
//	return system(localconcat("fluidsynth -F ", localconcat(wavFileName, localconcat(" ", localconcat(soundFontFileName, localconcat(" ", midiFileName))))));
//}
