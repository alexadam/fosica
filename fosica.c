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
#include "fosica.h"
#include "parser.h"

//#include "/usr/local/include/portaudio.h"
//#include "libs/portaudio/include/portaudio.h"

/*
 	gcc  -std=c99 -o fosica utils.c effects.c fosica.c  -lpthread -lm -L/usr/local/lib -lportaudio -lsndfile -lsamplerate -Wl,-rpath -Wl,/usr/local/lib

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












unsigned long globalIndex = 0;

SNDFILE * outputFile;
char * outputFileName = "out.wav";
sndData dataBuffer;
int BUFF_LEN = 4410;// 22000; //FIXME
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

	char * string = readFileToBuffer("merge.txt");

	if (nrOfTracks > 0) {
		for (int i = 0; i < nrOfTracks; ++i) {
			if (tracks[i].sequences) {
				for (int j = 0; j < tracks[i].nrOfSeqs; ++j) {
					free(tracks[i].sequences[j].instructions);
					free(tracks[i].sequences[j].buffer);

					tracks[i].sequences[j].lastHash = 0;
					tracks[i].sequences[j].lastStart = 0;
					tracks[i].sequences[j].lastStop = 0;
				}
				free(tracks[i].sequences);
			}
		}

		free(tracks);
	}

	nrOfTracks = 0;


	/**

	<nr Of Tracks>
	track
	<tempo>
	<force stop sound>
	<total sequences>
	<nr of sequence>
	<sequence start>
	<sequence stop>
	<sequence instruction>
	<sequence start>
	<sequence stop>
	<sequence instruction>
	.
	.
	.
	.


	 */

	if (string) {

		int trackHeader = 1;
		int eventStart = 2;
		int eventStop = 3;
		int eventInstr = 4;
		int lastEvent = -1;

		track * lastTrack;
		int nrOfLines = 0;
		int cTrackIndex = 0;
		int cSequenceIndex = 0;

		char ** parts = split(string, '\n', &nrOfLines);

		for (int i = 0; i < nrOfLines; ++i) {

			char * cLine = parts[i];

			if (i == 0) {
				nrOfTracks = string2int(cLine);
				tracks = malloc(nrOfTracks * sizeof(track));
			} else if (strcmp(cLine, "track") == 0) {
				lastEvent = trackHeader;

				int tempo = string2int(parts[i+1]);
				int forceStopSound = string2int(parts[i+2]);
				int totalSeqs = string2int(parts[i+3]);
				int currentSeqs = string2int(parts[i+4]);

				i = i+4;

				track t1;
				t1.index = cTrackIndex;
				t1.tempo = tempo;
				t1.forceStopSound = forceStopSound;
				t1.totalNrOfSeqs = totalSeqs;
				t1.nrOfSeqs = currentSeqs;
				t1.sequences = malloc(t1.nrOfSeqs * sizeof(sequence));
				tracks[cTrackIndex] = t1;
				cTrackIndex++;

				lastTrack = &t1;
				cSequenceIndex = 0;

			} else if (lastEvent == trackHeader || lastEvent == eventInstr) {
				lastEvent = eventStart;
				lastTrack->sequences[cSequenceIndex].lastStart = lastTrack->sequences[cSequenceIndex].start;
				lastTrack->sequences[cSequenceIndex].start = string2int(cLine);

			} else if (lastEvent == eventStart) {
				lastEvent = eventStop;
				lastTrack->sequences[cSequenceIndex].lastStop = lastTrack->sequences[cSequenceIndex].stop;
				lastTrack->sequences[cSequenceIndex].stop = string2int(cLine);

			} else if (lastEvent == eventStop) {
				lastEvent = eventInstr;

				lastTrack->sequences[cSequenceIndex].instructions = malloc((1 + strlen(cLine)) * sizeof(char));
				strcpy (lastTrack->sequences[cSequenceIndex].instructions, cLine);

				int tempo = lastTrack->tempo;
				int totalNrOfSeqs = lastTrack->totalNrOfSeqs;
				int samplingRate = 44104; // TODO ??? framesPerSecond must be multiple of channel nr + what happens if inputdata.dataLength is even/ not multiple of nr channels
				int framesPerSeq = (60.0 / tempo) * samplingRate;
				int totalFrames = totalNrOfSeqs * framesPerSeq;
				int trackIndex = globalIndex % totalFrames;
				int sequenceIndex = (trackIndex - (lastTrack->sequences[cSequenceIndex].start * framesPerSeq));
				int totalFramesPerSeq = (lastTrack->sequences[cSequenceIndex].stop - lastTrack->sequences[cSequenceIndex].start) * framesPerSeq;

				lastTrack->sequences[cSequenceIndex].bufferLenInSamples = BUFF_LEN > totalFramesPerSeq ? totalFramesPerSeq : BUFF_LEN; //TODO review
				lastTrack->sequences[cSequenceIndex].buffer = malloc(lastTrack->sequences[cSequenceIndex].bufferLenInSamples * sizeof(float));

				cSequenceIndex++;
			}

			free(cLine);
		}

		free(string);
		free(parts);
	}
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



//////////////
////////////
////////////
////////////

int localBufferLen = 0;

typedef enum {cFLOAT, cSTRING, cFLOAT_LIST} ALL_TYPES;

typedef struct
{
    ALL_TYPES type;
    int size;

    union
    {
    	float float_val;
    	char * string_val;
    	float * float_list_val;
    };

} mType;

typedef struct
{
	mType ** data;
    int index;
    int capacity;
} objStack;

objStack oStack;
mType ** vars;

int maxVars = 30;

typedef enum {ADD, SUB, MUL, DIV, MOD, POW, EQ, NOT, NOTEQ, GT, GTE, LT, LTE, AND, OR} OPS;

void initStacks(int capacity) {
	oStack.data = malloc(capacity * sizeof(mType *));
	oStack.index = 0;
	oStack.capacity = capacity;

	vars = malloc(maxVars * sizeof(mType *));

	for (int i = 0; i < maxVars; ++i) {
		vars[i] = NULL;
	}
}

void destroyStacks() {
	for (int i = 0; i < oStack.index; ++i) {
		destroyObj(oStack.data[i]);
	}
	free(oStack.data);
}

void destroyObj(mType * obj) {
	if (obj->type == cFLOAT_LIST) {
		free(obj->float_list_val);
	}

	free(obj);
}

void pushObj(mType * val) {
	oStack.data[oStack.index] = val;
	oStack.index++;
	oStack.index = oStack.index % oStack.capacity;
}

mType * popObj() {
	oStack.index--;
	oStack.index = oStack.index % oStack.capacity;
	return oStack.data[oStack.index];
}

mType * deepCopy(mType * source) {
	mType * res = malloc(sizeof(mType));
	res->type = source->type;

	if (source->type == cFLOAT) {
		res->float_val = source->float_val;
	} else if (source->type == cFLOAT_LIST) {
		res->size = source->size;
		res->float_list_val = malloc(res->size * sizeof(float));
		memcpy(res->float_list_val, source->float_list_val, res->size * sizeof(float));
	}

	return res;
}

mType * createListObj(float * input, int size) {
	mType * res = malloc(sizeof(mType));

	res->type = cFLOAT_LIST;
	res->size = size;
	res->float_list_val = malloc(size * sizeof(float));

	memcpy(res->float_list_val, input, size * sizeof(float));

	return res;
}

mType * parseList(char * listStr) {
	mType * listElem = malloc(sizeof(mType));
	listElem->type = cFLOAT_LIST;

	int slen = strlen(listStr);
	char * sub = substring(listStr, 1, slen - 2);
	int elemCount = 0;
	char ** listParts = split(sub, ' ', &elemCount);

	listElem->size = elemCount;
	listElem->float_list_val = malloc(elemCount * sizeof(float));

	for (int i = 0; i < elemCount; ++i) {
		listElem->float_list_val[i] = string2float(listParts[i]);
	}

	free(sub);
	for (int i = 0; i < elemCount; ++i) {
		free(listParts[i]);
	}
	free(listParts);

	return listElem;
}

void printFStack() {
	for (int i = 0; i < oStack.index; ++i) {
		mType * tmp = oStack.data[i];

		printf("lista are %p %d %d %d\n", tmp, tmp->size, oStack.index, i);

		if (tmp->type == cFLOAT) {
			printf("nr %f\n",tmp->float_val);
		} else if (tmp->type == cFLOAT_LIST) {
			for (int j = 0; j < tmp->size; ++j) {
				printf("Float list %f\n", tmp->float_list_val[j]);
			}
		}
	}
}

mType * file(char * fileName, int start, int length) {
	mType * result = malloc(sizeof(mType));
	result->type = cFLOAT_LIST;
	result->size = length;

	cachedSnd * csnd = getCachedSnd(fileName);

	result->float_list_val = malloc(length * sizeof(float));

	for (int i = start; i < (start + length); ++i) {
		if (csnd && i < csnd->snd.dataLength) {
			result->float_list_val[i - start] = csnd->snd.data[i];
		} else {
			result->float_list_val[i - start] = 0.0;
		}
	}

	return result;
}

mType * baseOp(OPS op, mType * f1, mType * f2) {
	mType * res = malloc(sizeof(mType));

	if (f1->type == cFLOAT && f2->type == cFLOAT) {
		res->type = cFLOAT;

		if (op == ADD) {
			res->float_val = f1->float_val + f2->float_val;
		} else if (op == SUB) {
			res->float_val = f1->float_val - f2->float_val;
		} else if (op == MUL) {
			res->float_val = f1->float_val * f2->float_val;
		} else if (op == DIV) {
			float tmp = f2->float_val;
			if (tmp == 0.0) {
				tmp = 1.0;
			}

			res->float_val = f1->float_val / tmp;
		} else if (op == MOD) {
			res->float_val = (int)f1->float_val % (int)f2->float_val;
		} else if (op == POW) {
			res->float_val =  pow(f1->float_val, f2->float_val);
		}
	} else if (f1->type == cFLOAT_LIST && f2->type == cFLOAT_LIST) {
		res->type = cFLOAT_LIST;

		res->size = f1->size;
		res->float_list_val = malloc(f1->size * sizeof(float));

		for (int i = 0; i < f1->size; ++i) {
			if (i < f2->size) {

				if (op == ADD) {
					res->float_list_val[i] = f1->float_list_val[i] + f2->float_list_val[i];
				} else if (op == SUB) {
					res->float_list_val[i] = f1->float_list_val[i] - f2->float_list_val[i];
				} else if (op == MUL) {
					res->float_list_val[i] = f1->float_list_val[i] * f2->float_list_val[i];
				} else if (op == DIV) {
					float tmp = f2->float_list_val[i];
					if (tmp == 0.0) {
						tmp = 1.0;
					}

					res->float_list_val[i] = f1->float_list_val[i] / tmp;
				} else if (op == MOD) {
					res->float_list_val[i] = (int)f1->float_list_val[i] % (int)f2->float_list_val[i];
				} else if (op == POW) {
					res->float_list_val[i] =  pow(f1->float_list_val[i], f2->float_list_val[i]);
				}
			}
		}
	} else if (f1->type == cFLOAT_LIST && f2->type == cFLOAT) {
		res->type = cFLOAT_LIST;
		res->size = f1->size;
		res->float_list_val = malloc(f1->size * sizeof(float));

		for (int i = 0; i < f1->size; ++i) {

			if (op == ADD) {
				res->float_list_val[i] = f1->float_list_val[i] + f2->float_val;
			} else if (op == SUB) {
				res->float_list_val[i] = f1->float_list_val[i] - f2->float_val;
			} else if (op == MUL) {
				res->float_list_val[i] = f1->float_list_val[i] * f2->float_val;
			} else if (op == DIV) {
				float tmp = f2->float_val;
				if (tmp == 0.0) {
					tmp = 1.0;
				}

				res->float_list_val[i] = f1->float_list_val[i] / tmp;
			} else if (op == MOD) {
				res->float_list_val[i] = (int)f1->float_list_val[i] % (int)f2->float_val;
			} else if (op == POW) {
				res->float_list_val[i] =  pow(f1->float_list_val[i], f2->float_val);
			}
		}
	} else if (f2->type == cFLOAT_LIST && f1->type == cFLOAT) {
		res->type = cFLOAT_LIST;
		res->size = f2->size;
		res->float_list_val = malloc(f2->size * sizeof(float));

		for (int i = 0; i < f2->size; ++i) {
			if (op == ADD) {
				res->float_list_val[i] = f1->float_val + f2->float_list_val[i];
			} else if (op == SUB) {
				res->float_list_val[i] = f1->float_val - f2->float_list_val[i];
			} else if (op == MUL) {
				res->float_list_val[i] = f1->float_val * f2->float_list_val[i];
			} else if (op == DIV) {
				float tmp = f2->float_list_val[i];
				if (tmp == 0.0) {
					tmp = 1.0;
				}

				res->float_list_val[i] = f1->float_val / tmp;
			} else if (op == MOD) {
				res->float_list_val[i] = (int)f1->float_val % (int)f2->float_list_val[i];
			} else if (op == POW) {
				res->float_list_val[i] =  pow(f1->float_val, f2->float_list_val[i]);
			}
		}
	}

	return res;
}

mType * inv(mType * f1) {
	mType * result = malloc(sizeof(mType));

	if (f1->type == cFLOAT) {
		result->type = cFLOAT;
		result->float_val = f1->float_val;
	} else {
		result->type = cFLOAT_LIST;
		result->size = f1->size;
		result->float_list_val = malloc(f1->size * sizeof(float));
		for (int i = 0; i < f1->size; ++i) {
			result->float_list_val[i] = f1->float_list_val[f1->size - i - 1];
		}
	}
	return result;
}

mType * dup(mType * f1) {
	mType * result = malloc(sizeof(mType));

	if (f1->type == cFLOAT) {
		result->type = cFLOAT;
		result->float_val = f1->float_val;
	} else {
		result->type = cFLOAT_LIST;
		result->size = f1->size;
		result->float_list_val = malloc(f1->size * sizeof(float));
		for (int i = 0; i < f1->size; ++i) {
			result->float_list_val[i] = f1->float_list_val[i];
		}
	}
	return result;
}

float * parseValue(char * input, int sequenceIndex, int abufferLen) {
	localBufferLen = abufferLen;
	int inputLen = strlen(input);
	int lastPosition = 0;
	mType * res = NULL;
	initStacks(1024);

	int elemCount = 0;
	char ** parts = split(input, '|', &elemCount);

	for (int i = 0; i < elemCount; ++i) {
		char * param = parts[i];

		if (param == NULL || strcmp(param, "") == 0) {
			continue;
		}

		if (param[0] == '[') {
			mType * tmp = parseList(param);
			pushObj(tmp);

			free(param);
			continue;
		}

		int paramLen = strlen(param);
		int hasCommas = 0;
		for (int j = 0; j < paramLen; ++j) {
			if (param[j] == ',') {
				hasCommas = 1;
				break;
			}
		}

		if (hasCommas == 1) {
			int nrCommas = 0;
			char ** commaParts = split(param, ',', &nrCommas);

			if (strcmp(commaParts[0], "file") == 0) {
				mType * tmp = file(commaParts[1], sequenceIndex, localBufferLen);
				pushObj(tmp);
			}

			for (int k = 0; k < nrCommas; ++k) {
				free(commaParts[k]);
			}
			free(commaParts);
		} else {

			if (isdigit(param[0]) || (strlen(param) > 1 && isdigit(param[1]) && param[0] == '-')) {
				float val = string2float(param);

				mType * tmp = malloc(sizeof(mType));
				tmp->type = cFLOAT;
				tmp->float_val = val;
				pushObj(tmp);
			} else if (param[0] == '=') {
				mType * tmp = popObj();

				mType * newtmp = deepCopy(tmp);

				vars[param[1] % maxVars] = newtmp;
				pushObj(tmp);
			} else if (param[0] == ':') {
				mType * tmp = vars[param[1] % maxVars];

				if (tmp == NULL) {
					continue;
				}

				mType * newTmp = deepCopy(tmp);
				pushObj(newTmp);
			} else if (strcmp(param, "+") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(ADD, f1, f2);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "-") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(SUB, f2, f1);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "*") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(MUL, f1, f2);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "/") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(DIV, f2, f1);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "%") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(MOD, f2, f1);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "inv") == 0) {
				mType * f1 = popObj();

				res = inv(f1);
				pushObj(res);

				destroyObj(f1);
			} else if (strcmp(param, "pow") == 0) {
				mType * f1 = popObj();
				mType * f2 = popObj();

				mType * tmp = baseOp(POW, f2, f1);
				pushObj(tmp);

				destroyObj(f1);
				destroyObj(f2);
			} else if (strcmp(param, "dup") == 0) {
				mType * f1 = popObj();
				mType * df1 = dup(f1);

				pushObj(f1);
				pushObj(df1);
			} else if (strcmp(param, "sin1") == 0) {
				mType * totalNrOfSamples = popObj();
				mType * freq = popObj();

				float * res = sinGenA((int)freq->float_val, sequenceIndex, localBufferLen, (unsigned long int) totalNrOfSamples->float_val);

				mType * tmp = createListObj(res, localBufferLen);

				pushObj(tmp);

				destroyObj(totalNrOfSamples);
				destroyObj(freq);
			} else if (strcmp(param, "sin2") == 0) {
				mType * phase = popObj();
				mType * totalNrOfSamples = popObj();
				mType * freq = popObj();

				float * res = sinGenPhase((int)freq->float_val, (int)phase, 0, localBufferLen, (unsigned long int) totalNrOfSamples->float_val);

				mType * tmp = createListObj(res, localBufferLen);

				pushObj(tmp);

				destroyObj(totalNrOfSamples);
				destroyObj(freq);
				destroyObj(phase);
			}
		}

		free(param);
	}

	free(parts);

	res = popObj();

	float * result = malloc(localBufferLen * sizeof(float));

	for (int i = 0; i < localBufferLen; ++i) {
		result[i] = 0.0;
	}

	if (res->type == cFLOAT) {
		result[0] = res->float_val;
	} else if (res->type == cFLOAT_LIST) {
		memcpy(result, res->float_list_val, res->size * sizeof(float));
	}

	destroyStacks();

	return result;
}


void soundGenFunction(sndData * data, track * cTrack) {

    int tempo = cTrack->tempo;
    int totalNrOfSeqs = cTrack->totalNrOfSeqs;
    int samplingRate = 44104; // TODO ??? framesPerSecond must be multiple of channel nr + what happens if inputdata.dataLength is even/ not multiple of nr channels
    int framesPerSeq = (60.0 / tempo) * samplingRate;
    int totalFrames = totalNrOfSeqs * framesPerSeq;
    int repeat = totalFrames;
    int currentSeq = -1;

    int lastComputedSeq = -1;

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

				//if not computed
				if (currentSeq != lastComputedSeq) {
					float * tmp = parseValue(cTrack->sequences[var].instructions, sequenceIndex, cTrack->sequences[var].bufferLenInSamples);
					memcpy(cTrack->sequences[var].buffer, tmp, cTrack->sequences[var].bufferLenInSamples * sizeof(float));
					lastComputedSeq = currentSeq;
				}

				if (cTrack->sequences[var].bufferLenInSamples >= BUFF_LEN) {
					if (i < cTrack->sequences[var].bufferLenInSamples)
						data->data[i] = cTrack->sequences[var].buffer[i]; //TODO review
				} else {
					data->data[i] = cTrack->sequences[var].buffer[sequenceIndex];
				}

				break; //TODO do not 'break' but instead mix all vals from each sequence ?
			}
		}

		if (foundSeq == 0) {
			data->data[i] = 0.0;
		}

    }
/**
 * soundsketch
 * soundpattern
 * namjera.com -> meaning, mind, tendency, notion, intension, design (Bosnian)
 * kreirati.com -> design (Croatian)
 * soundmodel.org
 *
 */

}
