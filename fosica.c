#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <samplerate.h>
#include <sndfile.h>
#include <portaudio.h>
#include <jansson.h>

#include "utils.h"
#include "effects.h"

//#include "/usr/local/include/portaudio.h"
//#include "libs/portaudio/include/portaudio.h"

/*
 	gcc  -std=c99 -o fosica utils.c effects.c fosica.c  -lpthread -lm -L/usr/local/lib -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib

 */

//--------
// root
//--------

//-------
// sndFile utils
//-------

typedef struct
{
    float * data;
    int nrOfFrames;
    int nrOfChannels;
    int samplingRate;
    int left_phase;
    int right_phase;
    int dataLength;
}
sndData;

typedef struct
{
    char * instructions;
    int start;
    int stop;
}
sequence;

typedef struct
{
    sequence * sequences;
    int nrOfSeqs;
    int totalNrOfSeqs;
    int index;
    int tempo;
    int forceStopSound;
}
track;

typedef struct {
    track * cTrack;
    sndData * data;
} threadArgContainer;

typedef struct
{
    sndData snd;
    char * name;
}
cachedSnd;

sndData resample(sndData * data, int newSamplingRate);
double apply_gain(float * data, long frames, int channels, double max, double gain);
sndData changeNrOfChannels(sndData * data, int newNrChannels, long startPoint);
void mix2(const sndData * toData, const  sndData * fromData);
sndData mix(const sndData * data1, const sndData * data2);
sndData duplicateClean(sndData * data);

sndData resample(sndData * data, int newSamplingRate) {

    float ratio = newSamplingRate / data->samplingRate;

    sndData resultData;
    resultData.data = malloc(data->dataLength * sizeof(float) * ratio);
    resultData.dataLength = (int) data->dataLength * ratio;
    resultData.left_phase = 0;
    resultData.right_phase = 0;
    resultData.nrOfChannels = data->nrOfChannels;
    resultData.nrOfFrames = resultData.dataLength / resultData.nrOfChannels;
    resultData.samplingRate = newSamplingRate;

    SRC_STATE *src_state;
    SRC_DATA src_data;
    int error;

    if ((src_state = src_new(SRC_SINC_BEST_QUALITY, data->nrOfChannels, &error)) == NULL) {
        printf("\n\nError : src_new() failed : %s.\n\n", src_strerror(error));
        exit(1);
    };

    src_data.end_of_input = 1;

    src_data.input_frames = data->dataLength / data->nrOfChannels;
    src_data.data_in = data->data;

    src_data.src_ratio = ratio;

    src_data.data_out = resultData.data;
    src_data.output_frames = src_data.input_frames * src_data.src_ratio;

    if ((error = src_process(src_state, &src_data))) {
        printf("\nError AA : %s\n", src_strerror(error));
        exit(1);
    };

    //apply_gain (src_data.data_out, testData.maxData, testData.channels, 1.0, 1.0) ;

    return resultData;
}

double apply_gain(float * data, long frames, int channels, double max, double gain)
{
    long k ;

    for (k = 0 ; k < frames * channels ; k++)
    {    data [k] *= gain ;

        if (fabs (data [k]) > max)
            max = fabs (data [k]) ;
        } ;

    return max ;
}

sndData duplicateClean(sndData * data) {
    sndData tmpData;
    tmpData.data = malloc(data->dataLength * sizeof(float));
    tmpData.dataLength = data->dataLength;
    tmpData.left_phase = data->left_phase;
    tmpData.right_phase = data->right_phase;
    tmpData.nrOfChannels = data->nrOfChannels;
    tmpData.nrOfFrames = data->nrOfFrames;
    tmpData.samplingRate = data->samplingRate;
    return tmpData;
}

sndData changeNrOfChannels(sndData * data, int newNrChannels, long startPoint) {
    sndData resultData;

    if (startPoint < 0)
        startPoint = 0;

    if (data->nrOfChannels == 2 && newNrChannels == 1) {

        resultData.data = malloc((data->dataLength / 2) * sizeof(float));

        for (int i = startPoint; i < data->dataLength; i += 2) {
            resultData.data[i / 2] = data->data[i];
        }

        resultData.nrOfFrames = data->nrOfFrames / 2;
        resultData.nrOfChannels = newNrChannels;
        resultData.samplingRate = data->samplingRate;
        resultData.left_phase = data->left_phase;
        resultData.right_phase = data->right_phase;
        resultData.dataLength = (int) data->dataLength / 2;
    } else if (data->nrOfChannels == 1 && newNrChannels == 2) {
        resultData.data = malloc(data->dataLength * 2 * sizeof(float));

        for (int i = startPoint; i < data->dataLength; i++) {
            resultData.data[i * 2] = data->data[i];
            resultData.data[i * 2 + 1] = data->data[i];
        }

        resultData.nrOfFrames = data->nrOfFrames;
        resultData.nrOfChannels = newNrChannels;
        resultData.samplingRate = data->samplingRate;
        resultData.left_phase = data->left_phase;
        resultData.right_phase = data->right_phase;
        resultData.dataLength = data->dataLength * 2;
    }

    return resultData;
}

/**
 *  mix fromData with toData; saves result in toData
 */
void mix2(const sndData * toData, const  sndData * fromData) {

    //toData.legth must be equal to fromData

    float max = 0.0;

    for (int i = 0; i < toData->dataLength; i++) {
        float tmp = toData->data[i] + fromData->data[i];
        if (tmp > max) {
            max = tmp;
        }
    }

    if (toData->nrOfChannels == fromData->nrOfChannels) {
        for (int i = 0; i < toData->dataLength; i++) {
            if ((float) toData->data[i] == 0.0) {
                toData->data[i] = fromData->data[i];
            } else if ((float) fromData->data[i] == 0.0) {
                continue;
            } else {
                float ssum = (toData->data[i] + fromData->data[i]);
                toData->data[i] = (ssum / 2) * 1.85;
            }
        }
    }
}

sndData mix(const sndData * data1, const sndData * data2) {

    sndData resultData;
    sndData * maxData;
    sndData * minData;

    if (data1->nrOfChannels == data2->nrOfChannels) {
        if (data1->dataLength >= data2->dataLength) {
            maxData = data1;
            minData = data2;
        } else {
            maxData = data2;
            minData = data1;
        }

        resultData.data = malloc(maxData->dataLength * sizeof(float));

        for (int i = 0; i < maxData->dataLength; i++) {
            if (i < minData->dataLength) {
                if ((float)maxData->data[i] == 0.0) {
                    resultData.data[i] = minData->data[i];
                } else if ((float)minData->data[i] == 0.0) {
                    resultData.data[i] = maxData->data[i];
                } else {
                    resultData.data[i] = (maxData->data[i] + minData->data[i]) / 2;
                }
            } else {
                resultData.data[i] = maxData->data[i];
            }
        }

    }

    resultData.nrOfFrames = maxData->nrOfFrames; //TODO - create "duplicate headers" function
    resultData.nrOfChannels = maxData->nrOfChannels;
    resultData.samplingRate = maxData->samplingRate;
    resultData.left_phase = maxData->left_phase;
    resultData.right_phase = maxData->right_phase;
    resultData.dataLength = maxData->dataLength;

    return resultData;
}

//----------
//IO utils
//---------

sndData readFile(char * fileName);
void writeDataToFile(sndData * data, char * fileName);
SNDFILE * openFileToWrite(sndData * data, char * fileName);
void writeToFile(SNDFILE *outfile, sndData * data);
void closeFile(SNDFILE *outfile);

sndData readFile(char * fileName) {
    SNDFILE *sf;

    SF_INFO info;
    int num_channels;
    int num, num_items;
    float *buf;
    int f, sr, c;
    int i, j;
    FILE *out;

    sndData testData;

    /* Open the WAV file. */
    info.format = 0;

    sf = sf_open(fileName, SFM_READ, &info);

    if (sf == NULL) {
        printf("Failed to open the file. %s\n", fileName);
        exit(-1);
    }

    f = info.frames;
    sr = info.samplerate;
    c = info.channels;

    num_items = f * c;

    buf = malloc(num_items * sizeof(float));

    num = sf_read_float(sf, buf, num_items);

    testData.data = buf;
    testData.dataLength = num_items;
    testData.left_phase = testData.right_phase = 0;
    testData.nrOfChannels = c;
    testData.samplingRate = sr;
    testData.nrOfFrames = f;

    sf_close(sf);

    return testData;
}

static void StreamFinished( void* userData )
{
   sndData *data = (sndData *) userData;
   printf( "Stream Completed: \n");
}

/**
 * open file; write data; close file
 * equivalent to openFileToWrite + writeToFile + closeFile
 */
void writeDataToFile(sndData * data, char * fileName) {
    SNDFILE        *outfile;
    SF_INFO        sfinfo;
    int            k, readcount;

    sfinfo.samplerate    = data->samplingRate;
    sfinfo.frames        = data->nrOfFrames;
    sfinfo.channels        = data->nrOfChannels;
    sfinfo.format        = (SF_FORMAT_WAV | SF_FORMAT_PCM_24) ; //65541; //SF_FORMAT_WAV;

    if (! (outfile = sf_open (fileName, SFM_WRITE, &sfinfo)))
        {    printf ("Error : could not open file : %s\n", fileName) ;
            puts (sf_strerror (NULL)) ;
            exit (1) ;
            } ;

    sf_write_float (outfile, data->data, data->dataLength) ;
    sf_close (outfile) ;
}

SNDFILE * openFileToWrite(sndData * data, char * fileName) {
    SF_INFO sfinfo;
    int k, readcount;

    sfinfo.samplerate = data->samplingRate;
    sfinfo.frames = data->nrOfFrames;
    sfinfo.channels = data->nrOfChannels;
    sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_24); //65541; //SF_FORMAT_WAV; //SF_FORMAT_FLOAT SF_FORMAT_PCM_U8  SF_FORMAT_PCM_16  SF_FORMAT_PCM_24  SF_FORMAT_PCM_32  SF_FORMAT_FLOAT  SF_FORMAT_DOUBLE

    return sf_open(fileName, SFM_WRITE, &sfinfo);
}

void writeToFile(SNDFILE *outfile, sndData * data) {
    sf_write_float(outfile, data->data, data->dataLength);
}

void closeFile(SNDFILE *outfile) {
    sf_close(outfile);
}












unsigned long globalIndex = 0;

SNDFILE * outputFile;
char * outputFileName = "out.wav";
sndData dataBuffer;
int BUFF_LEN = 4410; //FIXME
track * tracks;
int nrOfTracks = 0;
cachedSnd * cachedSnds;
int nrOfCachedSnds = 0;

int main() {
    init();
}



void soundGenFunction(sndData * data, track * cTrack);

int audioCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData );

void createSound(sndData * data);
void * threadContainer(void * arg);
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;

void init() {

	cachedSnds = malloc(10 * sizeof(cachedSnd));
	nrOfCachedSnds = 0;

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

        err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
        if( err != paNoError ) goto error;

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

    createTracks();

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

void createTracks() {

	for (int i = 0; i < nrOfTracks; ++i) {
		if (tracks[i].sequences) {
			for (int j = 0; j < tracks[i].nrOfSeqs; ++j) {
				free(tracks[i].sequences[j].instructions);
			}
			free(tracks[i].sequences);
		}
	}

	free(tracks);
	nrOfTracks = 0;

	tracks = malloc(10 * sizeof(track));

	/****
	 *
	 *
	 *
	 *
	 *
	 */

	track * lastTrack;
	int lastSequenceIndex = 0;

	char * text = readFileToBuffer("merge.json");

	if (!text) {
		return;
	}

	json_t *root;
	json_error_t error;

	root = json_loads(text, 0, &error);
	free(text);

	if(!root) {
		fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
		return;
	}

	json_t * jsonTracks = json_object_get(root, "tracks");

	if(!json_is_array(jsonTracks)) {
		fprintf(stderr, "error: root is not an array\n");
		json_decref(root);
		return;
	}

	for(int i = 0; i < json_array_size(jsonTracks); i++) {
		json_t * jsonTrack = json_array_get(jsonTracks, i);
		if (!json_is_object(jsonTrack)) {
			fprintf(stderr, "error: track %d is not an object\n", i + 1);
			json_decref(root);
			continue;
		}

		json_t * trackName = json_object_get(jsonTrack, "trackName");
		if (!json_is_string(trackName)) {
			fprintf(stderr, "error: trackName %d: is not a string\n", i + 1);
			json_decref(root);
			continue;
		}

		char * trackNameText = json_string_value(trackName);

		track t1;
		t1.index = nrOfTracks;
		t1.tempo = 240;
		t1.nrOfSeqs = 7;
		t1.sequences = malloc(t1.nrOfSeqs * sizeof(sequence));
		t1.totalNrOfSeqs = 32;
		t1.forceStopSound = 0;
		tracks[nrOfTracks] = t1;
		nrOfTracks++;

		lastTrack = &t1;
		lastSequenceIndex = 0;

		json_t * jsonSequences = json_object_get(jsonTrack, "sequences");
		if (!json_is_array(jsonSequences)) {
			fprintf(stderr, "error: commit %d: sequences is not an array\n", i + 1);
			json_decref(root);
			continue;
		}

		for (int j = 0; j < json_array_size(jsonSequences); j++) {
			json_t * jsonSequence = json_array_get(jsonSequences, j);

			if (!json_is_object(jsonSequence)) {
				fprintf(stderr, "error: sequence data %d is not an object\n", j + 1);
				json_decref(root);
				continue;
			}

			json_t * seqStart = json_object_get(jsonSequence, "start");
			if (!json_is_integer(seqStart)) {
				fprintf(stderr, "error: commit %d: seqStart is not a int\n", j + 1);
				json_decref(root);
				continue;
			}

			int seqStartInt = json_integer_value(seqStart);
			lastTrack->sequences[lastSequenceIndex].start = seqStartInt;

			json_t * seqStop = json_object_get(jsonSequence, "stop");
			if (!json_is_integer(seqStop)) {
				fprintf(stderr, "error: commit %d: seqStop is not a int\n", j + 1);
				json_decref(root);
				continue;
			}

			int seqStopInt = json_integer_value(seqStop);
			lastTrack->sequences[lastSequenceIndex].stop = seqStopInt;

			json_t * seqInstr = json_object_get(jsonSequence, "instructions");
			if (!json_is_string(seqInstr)) {
				fprintf(stderr, "error: commit %d: seqInstr is not a string\n", j + 1);
				json_decref(root);
				continue;
			}

			char * seqInstrText = json_string_value(seqInstr);

			lastTrack->sequences[lastSequenceIndex].instructions = malloc((1 + strlen(seqInstrText)) * sizeof(char));
			strcpy (lastTrack->sequences[lastSequenceIndex].instructions, seqInstrText);

			lastSequenceIndex++;
		}

	}

	/**
	 *char * string = readFileToBuffer("merge.txt");

	if (string) {

		int trackHeader = 1;
		int eventStart = 2;
		int eventStop = 3;
		int eventInstr = 4;
		int lastEvent = -1;

		size_t bufLen = strlen(string);
		size_t lastPosition = 0;
		track * lastTrack;
		int lastSequenceIndex = 0;

		for (int i = 0; i < bufLen; ++i) {

			if (string[i] == '\n' || i == (bufLen - 1)) {

				char * to = malloc((i - lastPosition + 1) * sizeof(char));
				to = substring(string, lastPosition, ((i == (bufLen - 1)) ? i - lastPosition + 1 : i - lastPosition));

				if (strstr(to, "@track")) {
					lastEvent = trackHeader;

				    track t1;
				    t1.index = nrOfTracks;
				    t1.tempo = 240;
				    t1.nrOfSeqs = 7;
				    t1.sequences = malloc(t1.nrOfSeqs * sizeof(sequence));
				    t1.totalNrOfSeqs = 32;
				    t1.forceStopSound = 1;
				   	jsonTracks[nrOfTracks] = t1;
				   	nrOfTracks++;

				   	lastTrack = &t1;
				   	lastSequenceIndex = 0;

				} else if (lastEvent == trackHeader || lastEvent == eventInstr) {
					lastEvent = eventStart;
					long found = strtol(&string[lastPosition], NULL, 0);

					lastTrack->sequences[lastSequenceIndex].start = (int)found;

				} else if (lastEvent == eventStart) {
					lastEvent = eventStop;
					long found = strtol(&string[lastPosition], NULL, 0);

					lastTrack->sequences[lastSequenceIndex].stop = (int) found;

				} else if (lastEvent == eventStop) {
					lastEvent = eventInstr;

					lastTrack->sequences[lastSequenceIndex].instructions = malloc((1 + strlen(to)) * sizeof(char));
					strcpy (lastTrack->sequences[lastSequenceIndex].instructions, to);

					lastSequenceIndex++;
				}

				free(to);
				lastPosition = i+1;
			}
		}

		free(string);
	}
	 *
	 *
	 *
	 */


//	    track t1;
//	    t1.index = nrOfTracks;
//	    t1.tempo = 240;
//	    t1.nrOfSeqs = 5;
//	    t1.sequences = (sequence *)malloc(t1.nrOfSeqs * sizeof(sequence));
//	    t1.totalNrOfSeqs = 32;
//
//	    for (int var = 0; var < t1.nrOfSeqs; ++var) {
//			t1.sequences[var].start = var;
//			t1.sequences[var].stop = var + 1;
//	    	t1.sequences[var].instructions = "sounds/hat.wav";
//		}
//
//	    t1.forceStopSound = 1;
//	    tracks[nrOfTracks] = t1;
//	    nrOfTracks++;

	//    track t2;
	//    t2.index = nrOfTracks;
	//    t2.tempo = 240;
	//    t2.nrOfSeqs = 32;
	//    t2.instructions = "sounds/shaker.wav";
	//    int points2[64] = {0,1, 1,2, 2,3, 3,4, 4,5, 5,6, 6,7, 7,8, 8,9, 9,10, 10,11, 11,12, 12,13, 13,14, 14,15, 15,16, 16,17, 17,18, 18,19, 19,20, 20,21, 21,22, 22,23, 23,24, 24,25, 25,26, 26,27, 27,28, 28,29, 29,30, 30,31, 31,32};
	//    t2.points = points2;
	//    t2.nrOfPoints = sizeof(points2) / sizeof(int);
	//    t2.forceStopSound = 0;
	//    tracks[nrOfTracks] = t2;
	//    nrOfTracks++;
	//
	//    track t3;
	//    t3.index = nrOfTracks;
	//    t3.tempo = 240;
	//    t3.nrOfSeqs = 32;
	//    t3.instructions = "sounds/tamb.wav";
	//    int points3[8] = {4, 12,12, 20,20, 28,28, 32};
	//    t3.points = points3;
	//    t3.nrOfPoints = sizeof(points3) / sizeof(int);
	//    t3.forceStopSound = 0;
	//    tracks[nrOfTracks] = t3;
	//    nrOfTracks++;
	//
	//    track t4;
	//    t4.index = nrOfTracks;
	//    t4.tempo = 240;
	//    t4.nrOfSeqs = 32;
	//    t4.instructions = "sounds/clap.wav";
	//    int points4[14] = {4,12, 12,20, 20,21, 21,23, 23,25, 25,28, 28,32};
	//    t4.points = points4;
	//    t4.nrOfPoints = sizeof(points4) / sizeof(int);
	//    t4.forceStopSound = 0;
	//    tracks[nrOfTracks] = t4;
	//    nrOfTracks++;
	//
	//    track t5;
	//    t5.index = nrOfTracks;
	//    t5.tempo = 240;
	//    t5.nrOfSeqs = 32;
	//    t5.instructions = "sounds/organic2.wav";
	//    int points5[10] = {2,10, 10,18, 18,21, 21,26, 26,32};
	//    t5.points = points5;
	//    t5.nrOfPoints = sizeof(points5) / sizeof(int);
	//    t5.forceStopSound = 0;
	//    tracks[nrOfTracks] = t5;
	//    nrOfTracks++;
	//
	//    track t6;
	//    t6.index = nrOfTracks;
	//    t6.tempo = 240;
	//    t6.nrOfSeqs = 32;
	//    t6.instructions = "sounds/organic1.wav";
	//    int points6[16] = {1,7, 7,9, 9,15, 15,17, 17,23, 23,25, 25,31, 31,32};
	//    t6.points = points6;
	//    t6.nrOfPoints = sizeof(points6) / sizeof(int);
	//    t6.forceStopSound = 1;
	//    tracks[nrOfTracks] = t6;
	//    nrOfTracks++;
	//
	//    track t7;
	//    t7.index = nrOfTracks;
	//    t7.tempo = 240;
	//    t7.nrOfSeqs = 32;
	//    t7.instructions = "sounds/tom.wav";
	//    int points7[12] = {1,3, 3,6, 6,17, 17,19, 19,22, 22,32};
	//    t7.points = points7;
	//    t7.nrOfPoints = sizeof(points7) / sizeof(int);
	//    t7.forceStopSound = 1;
	//    tracks[nrOfTracks] = t7;
	//    nrOfTracks++;
	//
	//    track t8;
	//    t8.index = nrOfTracks;
	//    t8.tempo = 240;
	//    t8.nrOfSeqs = 32;
	//    t8.instructions = "sounds/kick.wav";
	//    int points8[16] = {0,4, 4,8, 8,12, 12,16, 16,20, 20,24, 24,28, 28,32};
	//    t8.points = points8;
	//    t8.nrOfPoints = sizeof(points8) / sizeof(int);
	//    t8.forceStopSound = 1;
	//    tracks[nrOfTracks] = t8;
	//    nrOfTracks++;
}

void * threadContainer(void * arg) {
    threadArgContainer * argContainer = (threadArgContainer *) arg;

    sndData tmpData = duplicateClean(argContainer->data);

    track * cTrack = argContainer->cTrack;
    soundGenFunction(&tmpData, cTrack);

    pthread_mutex_lock(&bufferMutex);
    mix2(argContainer->data, &tmpData);
    pthread_mutex_unlock(&bufferMutex);

    free(tmpData.data);

    return NULL;
}

void createSound(sndData * data) {

    pthread_t * threads = malloc(nrOfTracks * sizeof(pthread_t));
    threadArgContainer * tac = malloc(nrOfTracks * sizeof(threadArgContainer));

    int retCode = 0;

    for (int i = 0; i < nrOfTracks; ++i) {
        tac[i].cTrack = &tracks[i];
        tac[i].data = data;

        retCode = pthread_create(&threads[i], NULL, threadContainer, (void*) &tac[i]);
    }

    for (int i = 0; i < nrOfTracks; ++i) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(tac);
}

cachedSnd * getCachedSnd(char * name) {

	for (int i2 = 0; i2 < nrOfCachedSnds; ++i2) {
		if (strcmp(cachedSnds[i2].name, name) == 0) {
			return &cachedSnds[i2];
		}
	}

	cachedSnd cs;
	cs.name = malloc((1 + strlen(name)) * sizeof(char));
	strcpy(cs.name, name);

	cs.snd = readFile(name);

	cachedSnds[nrOfCachedSnds] = cs;
	nrOfCachedSnds++;

	return &cachedSnds[nrOfCachedSnds - 1];
}

char ** split(const char * input, char sep, int * elemCoount) {

	char ** result = NULL;
	int inputLen = strlen(input);
	int lastPosition = 0;

	for (int i = 0; i < inputLen; ++i) {
		if (input[i] == sep) {
			(*elemCoount)++;
		}
	}

	(*elemCoount)++;

	int curElem = 0;

	result = (char ** )malloc((*elemCoount) * sizeof(char *));

	for (int i = 0; i < inputLen; ++i) {
		if (input[i] == sep || i == inputLen - 1) {


			int newLen = ((i == (inputLen - 1)) ? i - lastPosition + 2 : i - lastPosition + 1);

			result[curElem] = malloc(newLen * sizeof(char));

			memcpy (result[curElem] , input + lastPosition, newLen-1);
			result[curElem][newLen - 1] = '\0';

			lastPosition = i+1;
			curElem++;
		}
	}

	return result;
}

float getValueFromFile(char * fileName, unsigned long int position) {

	cachedSnd * csnd = getCachedSnd(fileName);

	if (!csnd) {
		return 0.0;
	}

	if (position >= csnd->snd.dataLength) {
		printf("NO SOUND");
		return 0.0;
	}

	return csnd->snd.data[position];
}

float getValueFromFunction(char * functionSignature, unsigned long mainIndex, int tempo, int nrOfSeq, int samplingRate) {

	int lenObj = strlen(functionSignature);
	int has = 0;

	for (int i = 0; i < lenObj; ++i) {
		if (functionSignature[i] == ',') {
			has = 1;
			break;
		}
	}

	if (!has) {
		return 0.0;
	}

	int elemCount = 0;
	float retVal = 0.0;

	char ** tokens = split(functionSignature, ',', &elemCount);

	if (strcmp(*tokens, "file") == 0) {
		retVal = getValueFromFile(tokens[1], mainIndex);
	} else if (strcmp(*tokens, "sin") == 0) {
		char * freqChar = tokens[1];
		int freq = string2int(freqChar);

		char * lengthInSamplesChar = tokens[2];
		int lengthInSamples = string2int(lengthInSamplesChar);

		free(freqChar);
		free(lengthInSamplesChar);
		retVal =  sinGen(mainIndex, freq, lengthInSamples);
	}

	for (int i = 0; i < elemCount; ++i) {
		free(tokens[i]);
	}
	free(tokens);

	return retVal;
}

float dummyParser(char * input, unsigned long int mainIndex, int tempo, int nrOfSeq, int samplingRate) {

	char * instructions = NULL;
	char * tmp1 =  ulint2string(mainIndex);
	char * tmp2 = int2string(tempo);


	instructions = str_replace(input, "#globalIndex", tmp1);
	char * oldp = instructions;
	instructions = str_replace(instructions, "#tempo", tmp2);

	free(oldp);
	free(tmp1);
	free(tmp2);

	int lenObj = strlen(instructions);
	int has = 0;

	for (int i = 0; i < lenObj; ++i) {
		if (instructions[i] == '|') {
			has = 1;
			break;
		}
	}

	int elemCount = 0;
	char ** tokens = NULL;

	if (!has) {
		tokens = instructions;
		elemCount = 1;
	} else {
		tokens = split(instructions, '|', &elemCount);
	}

	if (!tokens) {
		return 0.0;
	}

	float * valuesStack = malloc(elemCount * sizeof(float));

	for (int i = 0; i < elemCount; ++i) {
		char * token = (tokens + i);

		if (strcmp(token, "*") == 0) {
			valuesStack[i] = valuesStack[i - 2] * valuesStack[i - 1];
		} else if (strcmp(token, "+") == 0) {
			valuesStack[i] = valuesStack[i - 2] + valuesStack[i - 1];
		} else if (strcmp(token, "-") == 0) {
			valuesStack[i] = valuesStack[i - 2] - valuesStack[i - 1];
		} else if (strcmp(token, "/") == 0) {
			if (valuesStack[i - 1] == 0.0) {
				valuesStack[i] = valuesStack[i - 2];
			} else {
				valuesStack[i] = valuesStack[i - 2] / valuesStack[i - 1];
			}
		} else {
			int hasCommas = 0;
			int elemLen = strlen(token);

			for (int j = 0; j < elemLen; ++j) {
				if (*(token + j) == ',') {
					hasCommas = 1;
					break;
				}
			}

			if (!hasCommas) {
				//is number
				valuesStack[i] = strtof(*(tokens + i), NULL);
			} else {
				//is function
				char * loc = malloc(sizeof(char) * (strlen(token) + 1 )); //TODO
				strcpy(loc, token);
				valuesStack[i] = getValueFromFunction(token, mainIndex, tempo, nrOfSeq, samplingRate);
				free(loc);
			}
		}

	}

	float result = valuesStack[elemCount - 1];

	free(valuesStack);
	free(instructions);
//	for (int i = 0; i < elemCount; ++i) {
//		free(tokens[i]);
//	}
//	free(tokens);

	return result;
}

float getValue(char * instructions, unsigned long int mainIndex, int tempo, int nrOfSeq, int samplingRate) {
	return dummyParser(instructions, mainIndex, tempo, nrOfSeq, samplingRate);
}

void soundGenFunction(sndData * data, track * cTrack) {

    int tempo = cTrack->tempo;
    int totalNrOfSeqs = cTrack->totalNrOfSeqs;
    int samplingRate = 44104; // TODO ??? framesPerSecond must be multiple of channel nr + what happens if inputdata.dataLength is even/ not multiple of nr channels
    int framesPerSeq = (60.0 / tempo) * samplingRate;
    int totalFrames = totalNrOfSeqs * framesPerSeq;
    int repeat = totalFrames;
    int currentSeq = -1;

    int foundCachedSound = 0;

	for (int i = 0; i < data->dataLength; i += 1) {

		int ch = i % data->nrOfChannels;

		unsigned long int mainIndex = globalIndex + i;
		unsigned long int trackIndex = mainIndex % repeat;

		currentSeq = (int)trackIndex / framesPerSeq;

		int foundSeq = 0;

		for (int var = 0; var < cTrack->nrOfSeqs; ++var) {

			if (currentSeq >= cTrack->sequences[var].start && currentSeq < cTrack->sequences[var].stop) {
				foundSeq = 1;

				unsigned long sequenceIndex = (trackIndex - (cTrack->sequences[var].start * framesPerSeq));

				float val = 0.0;

//				if (cTrack->forceStopSound == 0) {
//					val = inputData.data[trackIndex % inputData.dataLength];
//				} else if (goodIndex < inputData.dataLength) {
//					val = getValueFromFile(cTrack->sequences[var].instructions, goodIndex);
					val = getValue(cTrack->sequences[var].instructions, sequenceIndex, tempo, totalNrOfSeqs, samplingRate);
//				}

				data->data[i] = val;

				break; //TODO do not 'break' but instead mix all val's from each sequence ?
			}
		}

		if (foundSeq == 0) {
			data->data[i] = 0.0;
		}

    }


}
