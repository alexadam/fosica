#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <samplerate.h>
#include <sndfile.h>
#include <portaudio.h>
#include <jansson.h>

#include "utils/utils.h"
#include "effects/effects.h"
#include "scripting/parser.h"
#include "cache/cache.h"
#include "utils/noiz_utils.h"

//#include "/usr/local/include/portaudio.h"
//#include "libs/portaudio/include/portaudio.h"

/*
	gcc -g -pedantic -Wall -Wextra -lefence -std=c99 -o noiz utils/utils.c utils/noiz_utils.c effects/effects.c cache/cache.c fosica.c  -lpthread -lm -L/usr/local/lib -ljansson -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib

 */

//----------
//IO utils
//---------


//TODO
//static void StreamFinished( void* userData )
//{
//   sndData *data = (sndData *) userData;
//   printf( "Stream Completed: \n");
//}


unsigned long int globalIndex = 0;

SNDFILE * outputFile;
char * outputFileName = "out.wav";
sndData dataBuffer;
const int BUFF_LEN = 4000;// 22000; //FIXME
track * tracks;
int nrOfTracks = 0;

CACHE * che;

int main() {
	che = initCache(10);
    init();
}


int audioCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData );

void createSound(sndData * data);
void * threadContainer(void * arg);
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;

void silenceGenArray(float * data, unsigned long int lengthInSamples) {
    for (int i = 0; i< lengthInSamples; i++) {
        data[i] = 0.0;
    }
}

void init() {

    /**
     *
     * Create Data Buffer and initialize it with "silence"
     *
     */

    dataBuffer.dataLength = BUFF_LEN;
    dataBuffer.left_phase = 0;
    dataBuffer.right_phase = 0;
    dataBuffer.nrOfChannels = 2;
    dataBuffer.nrOfFrames = BUFF_LEN / 2;
    dataBuffer.samplingRate = 44100;
    dataBuffer.data = malloc(BUFF_LEN * sizeof(float));

    silenceGenArray(dataBuffer.data, BUFF_LEN);

    /**
     *
     * open file to save sound
     *
     */

    outputFile = openFileToWrite(&dataBuffer, outputFileName);

    /**
     *
     * PortAudio settings
     *
     */

    PaStreamParameters outputParameters;
           PaStream *stream;
           PaError err;

    err = Pa_Initialize();
        if( err != paNoError ) goto error;

        outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
        if (outputParameters.device == paNoDevice) {
          fprintf(stderr,"Error: No default output device.\n");
          goto error;
        }
        outputParameters.channelCount = dataBuffer.nrOfChannels;       /* stereo output */
        outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
        outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
                  &stream,
                  NULL,
                  &outputParameters,
                  dataBuffer.samplingRate,
                  (BUFF_LEN / 2),
                  paNoFlag,
                  audioCallback,
                  &dataBuffer );

        if( err != paNoError ) goto error;

//        err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
//        if( err != paNoError ) goto error;

        err = Pa_StartStream( stream );
        if( err != paNoError ) goto error;

        printf("Play for %d seconds.\n", 1);
        Pa_Sleep( 100 * 1000 );

        err = Pa_StopStream( stream );
        if( err != paNoError ) goto error;

        err = Pa_CloseStream( stream );
        if( err != paNoError ) goto error;

        Pa_Terminate();
        printf("Test finished.\n");

    error:
        Pa_Terminate();
        fprintf( stderr, "An error occured while using the portaudio stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );

}

int audioCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{

    /**
     *
     * Reset the buffer
     *
     */

	silenceGenArray(dataBuffer.data, BUFF_LEN);

    /**
     *
     * Create the sound
     *
     */
    createSound(&dataBuffer);

    /**
     *
     * write buffer to file
     *
     */
    writeToFile(outputFile, &dataBuffer);

    /**
     *
     * Increment the index
     *
     */
    globalIndex += BUFF_LEN;

    /**
     *
     * TODO: use a double buffer
     *
     */
    sndData *data =  &dataBuffer;

    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo;
    (void) statusFlags;
    (void) inputBuffer;

    int dataLen = framesPerBuffer * data->nrOfChannels;

    for( i=0; i<dataLen; i++ ) {
    	*out++ = data->data[i];
    }

    return paContinue;
}

void createSound(sndData * data) {

	char * fileContent = readFileToBuffer("test.txt");
	float * tmp = getValue(fileContent, globalIndex, BUFF_LEN);

	memcpy(data->data, tmp, BUFF_LEN * sizeof(float));
}

//////////////
////////////
////////////
////////////


//TODO remove
//void soundGenFunction(sndData * data, track * cTrack) {
//
////    int tempo = cTrack->tempo;
////    int totalNrOfSeqs = cTrack->totalNrOfSeqs;
////    int samplingRate = 44104; // TODO ??? framesPerSecond must be multiple of channel nr + what happens if inputdata.dataLength is even/ not multiple of nr channels
////    int framesPerSeq = (60.0 / tempo) * samplingRate;
////    int totalFrames = totalNrOfSeqs * framesPerSeq;
////    int repeat = totalFrames;
////    int currentSeq = -1;
//
////    int lastComputedSeq = -1;
////
////	for (int i = 0; i < data->dataLength; i += 1) {
////
////		int ch = i % data->nrOfChannels;
////
////		unsigned long int mainIndex = globalIndex + i;
////		unsigned long int trackIndex = mainIndex % repeat;
////
////		currentSeq = (int)trackIndex / framesPerSeq;
////
////		int foundSeq = 0;
////
////		for (int var = 0; var < cTrack->nrOfSeqs; ++var) {
////
////			if (currentSeq >= cTrack->sequences[var].start && currentSeq < cTrack->sequences[var].stop) {
////				foundSeq = 1;
////
////				unsigned long sequenceIndex = (trackIndex - (cTrack->sequences[var].start * framesPerSeq));
////
////				//if not computed
////				if (currentSeq != lastComputedSeq) {
////
////					printf("AICI %d %d\n", currentSeq, var);
////
////					float * tmp = parseValue(cTrack->sequences[var].instructions, sequenceIndex, cTrack->sequences[var].bufferLenInSamples);
////					memcpy(cTrack->sequences[var].buffer, tmp, cTrack->sequences[var].bufferLenInSamples * sizeof(float));
////					lastComputedSeq = currentSeq;
////				}
////
//////				if (cTrack->sequences[var].bufferLenInSamples >= BUFF_LEN) {
//////					if (i < cTrack->sequences[var].bufferLenInSamples)
////						data->data[i] = cTrack->sequences[var].buffer[i]; //TODO review
//////				} else {
//////					data->data[i] = cTrack->sequences[var].buffer[sequenceIndex];
//////				}
////
////				break; //TODO do not 'break' but instead mix all vals from each sequence ?
////			}
////		}
////
////		if (foundSeq == 0) {
////			data->data[i] = 0.0; //sin(2 * 3.1415 * 5 * i / BUFF_LEN) * 0.6;
////		}
////
//////		else {
//////			data->data[i] = cTrack->sequences[currentSeq].buffer[i]; //TODO review
//////		}
//
////    }
///**
// * soundsketch
// * soundpattern
// * namjera.com -> meaning, mind, tendency, notion, intension, design (Bosnian)
// * kreirati.com -> design (Croatian)
// * soundmodel.org
// *
// */
//
//}
