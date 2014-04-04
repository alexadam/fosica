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
    int start; //TODO long int
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

	if (nrOfTracks > 0) {
		for (int i = 0; i < nrOfTracks; ++i) {
			if (tracks[i].sequences) {
				for (int j = 0; j < tracks[i].nrOfSeqs; ++j) {
					free(tracks[i].sequences[j].instructions);
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

	char * string = readFileToBuffer("merge.txt");

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
				lastTrack->sequences[cSequenceIndex].start = string2int(cLine);

			} else if (lastEvent == eventStart) {
				lastEvent = eventStop;
				lastTrack->sequences[cSequenceIndex].stop = string2int(cLine);

			} else if (lastEvent == eventStop) {
				lastEvent = eventInstr;

				lastTrack->sequences[cSequenceIndex].instructions = malloc((1 + strlen(cLine)) * sizeof(char));
				strcpy (lastTrack->sequences[cSequenceIndex].instructions, cLine);

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

float getValueFromFile(char * fileName, unsigned long int position) {

	cachedSnd * csnd = getCachedSnd(fileName);

	if (!csnd) {
		return 0.0;
	}

	if (position >= csnd->snd.dataLength) {
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
