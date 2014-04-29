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
#include "fosica.h"
#include "scripting/parser.h"

//#include "/usr/local/include/portaudio.h"
//#include "libs/portaudio/include/portaudio.h"

/*
	gcc -g -pedantic -Wall -Wextra -lefence -std=c99 -o noiz utils/utils.c effects/effects.c noiz.c  -lpthread -lm -L/usr/local/lib -ljansson -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib

 */



//--------
// root
//--------
float * parseValue(char * input, int sequenceIndex, int abufferLen);


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












unsigned long int globalIndex = 0;

SNDFILE * outputFile;
char * outputFileName = "out.wav";
sndData dataBuffer;
const int BUFF_LEN = 4000;// 22000; //FIXME
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

///////
//////
//////

const int RING_BUFF_SIZE = 80000; //TODO multiple of BUFF_LEN

typedef struct {
	float * data;
	int start;
	int end;
	int capacity;
} RingBuff;

typedef struct {
	RingBuff *  bufferData;
	char * instructions;
	int lastHash;
} cFunc;

void addToBuffer(RingBuff * buffer, float val) {
	buffer->data[buffer->end] = val;
	buffer->end += 1;
	buffer->end = buffer->end % buffer->capacity;
}

float getFromBuffer(RingBuff * buffer) {
	float ret = buffer->data[buffer->start];
	buffer->start += 1;
	buffer->start = buffer->start % buffer->capacity;
	return ret;
}

void resetBuffer(RingBuff * buffer) {
	buffer->start = 0;
	buffer->end = 0;
}

void file(char * name, int index, RingBuff * out, int bufferLen) {
	cachedSnd* csnd;
	csnd = getCachedSnd(name);

	for (int i = 0; i < bufferLen; ++i) {
		if (index + i >= csnd->snd.dataLength) {
			addToBuffer(out, 0.0);
		} else {
			addToBuffer(out, csnd->snd.data[index + i]);
		}
	}
}

void eval(cFunc ** chain, int nextFunc, int index, int bufferLen) {

	int currentHash = hash(chain[nextFunc]->instructions);

	if (currentHash != chain[nextFunc]->instructions) {
		resetBuffer(chain[nextFunc]->bufferData);
	}

	int diff = chain[nextFunc]->bufferData->end - chain[nextFunc]->bufferData->start;

	if (diff < 0) {
		diff = chain[nextFunc]->bufferData->capacity - diff;
	}

	if (diff >= bufferLen) {
		return;
	}

	int localIndex = index;
	int uuu = 0;

	while (diff < bufferLen && uuu<2000000) {

		uuu++;

		int elemCount = 0;
		char ** parts = split(chain[nextFunc]->instructions, ',', &elemCount);

		if (strcmp(parts[0], "file") == 0) {
			file(parts[1], localIndex, chain[nextFunc]->bufferData, bufferLen);
		}

		localIndex += bufferLen;

		diff = chain[nextFunc]->bufferData->end - chain[nextFunc]->bufferData->start;

		if (diff < 0) {
			diff = chain[nextFunc]->bufferData->capacity - diff;
		}

		for (int i = 0; i < elemCount; ++i) {
			free(parts[i]);
		}
		free(parts);

	}
}

cFunc ** parse(char * fileContent) {
	int elemCount = 0;
	char ** parts = split(fileContent, '|', &elemCount);

	cFunc ** chain = malloc(elemCount * sizeof(cFunc *));

	for (int i = 0; i < elemCount; ++i) {
		chain[i] = malloc(sizeof(cFunc));
		chain[i]->instructions = malloc(sizeof(char) * (strlen(parts[i]) + 1));
		strcpy(chain[i]->instructions, parts[i]);
		chain[i]->lastHash = hash(parts[i]);
		chain[i]->bufferData = malloc(sizeof(RingBuff));
		chain[i]->bufferData->start = 0;
		chain[i]->bufferData->end = 0;
		chain[i]->bufferData->capacity = RING_BUFF_SIZE;
		chain[i]->bufferData->data = malloc(chain[i]->bufferData->capacity * sizeof(float));
	}

	for (int i = 0; i < elemCount; ++i) {
		free(parts[i]);
	}
	free(parts);

	return chain;
}

int lastHash = 0;
cFunc ** chain = NULL;

void createSound(sndData * data) {

	char * fileContent = readFileToBuffer("test.txt");
	int currentHash = hash(fileContent);

	if (fileContent && currentHash != lastHash) {
		lastHash = currentHash;
		chain = parse(fileContent);
	}

	eval(chain, 0, globalIndex, BUFF_LEN);

	for (int i = 0; i < BUFF_LEN; ++i) {
		data->data[i] = getFromBuffer(chain[0]->bufferData);
	}

}

//////////////
////////////
////////////
////////////



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
