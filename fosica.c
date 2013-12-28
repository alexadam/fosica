#include <math.h>
#include <portaudio.h>
#include <samplerate.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <limits.h>

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

/*
 gcc  -std=c99 -lsndfile -lportaudio -lsamplerate -o fosica fosica.c
 */


//float genSin(int frequency, int currentSample, int sampleRate);
//float genSin(int frequency, int currentSample, int sampleRate) {
//
//	if (sampleRate == 0)
//		return 0;
//
//	return sin(2 * 3.1415 * frequency * currentSample / sampleRate);
//}
//
//float * genSinS(int frequency, int milliSeconds, int sampleRate);
//float * genSinS(int frequency, int milliSeconds, int sampleRate) {
//
//	if (milliSeconds <= 0 || sampleRate <= 0 || frequency <= 0)
//		return 0;
//
//	int totalFrames = (int)milliSeconds/1000 * sampleRate;
//
//	float * data = (float *) malloc(totalFrames * sizeof(float));
//
//	for (int i = 0; i < totalFrames; i++) {
//		data[i] = genSin(frequency, i, sampleRate);
//	}
//
//	return data;
//}

typedef struct
{
	int nrOfFrames;
	int nrOfChannels;
	int samplingRate;
    int left_phase;
    int right_phase;
    int dataLength;
    float * data;
}
sndData;

static void StreamFinished( void* userData )
{
   sndData *data = (sndData *) userData;
   printf( "Stream Completed: \n");
}

sndData getData(char * fileName);
sndData getData(char * fileName) {
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
		printf("Failed to open the file.\n");
		exit(-1);
	}

	/* Print some of the info, and figure out how much data to read. */
	f = info.frames;
	sr = info.samplerate;
	c = info.channels;

	printf("frames=%d\n", f);
	printf("samplerate=%d\n", sr);
	printf("channels=%d\n", c);

	num_items = f * c;

	printf("num_items=%d\n", num_items);
	/* Allocate space for the data to be read, then read it. */
	buf = (float *) malloc(num_items * sizeof(float));

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

sndData resample(sndData * data, int newSamplingRate);
sndData resample(sndData * data, int newSamplingRate) {

	float ratio = newSamplingRate / data->samplingRate;

	sndData resultData;
	resultData.data = (float *) malloc(data->dataLength * sizeof(float) * ratio);
	resultData.dataLength = (int) data->dataLength * ratio;
	resultData.left_phase = 0;
	resultData.right_phase = 0;
	resultData.nrOfChannels = data->nrOfChannels;
	resultData.nrOfFrames = resultData.dataLength / resultData.nrOfChannels;
	resultData.samplingRate = newSamplingRate;

	SRC_STATE *src_state;
	SRC_DATA src_data;
	int error;

	if ((src_state = src_new(SRC_SINC_BEST_QUALITY, data->nrOfChannels, &error))
			== NULL) {
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

static double apply_gain (float * data, long frames, int channels, double max, double gain);
static double apply_gain (float * data, long frames, int channels, double max, double gain)
{
	long k ;

	for (k = 0 ; k < frames * channels ; k++)
	{	data [k] *= gain ;

		if (fabs (data [k]) > max)
			max = fabs (data [k]) ;
		} ;

	return max ;
} /* apply_gain */


sndData changeNrOfChannels(sndData * data, int newNrChannels, long startPoint);
sndData changeNrOfChannels(sndData * data, int newNrChannels, long startPoint) {
	sndData resultData;

	if (startPoint < 0)
		startPoint = 0;

	if (data->nrOfChannels == 2 && newNrChannels == 1) {

		resultData.data = (float *) malloc(data->dataLength / 2 * sizeof(float));

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
		resultData.data = (float *) malloc(data->dataLength * 2 * sizeof(float));

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

void mix2(const sndData * toData,const  sndData * fromData);
void mix2(const sndData * toData,const  sndData * fromData) {

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

sndData mix(const sndData * data1, const sndData * data2);
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

		resultData.data = (float *) malloc(maxData->dataLength * sizeof(float));

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

void writeDataToFile(sndData * data, char * fileName);
void writeDataToFile(sndData * data, char * fileName) {
	SNDFILE		*outfile;
	SF_INFO		sfinfo;
	int			k, readcount;

	sfinfo.samplerate	= data->samplingRate;
	sfinfo.frames		= data->nrOfFrames;
	sfinfo.channels		= data->nrOfChannels;
	sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_16) ; //65541; //SF_FORMAT_WAV;

	if (! (outfile = sf_open (fileName, SFM_WRITE, &sfinfo)))
		{	printf ("Error : could not open file : %s\n", fileName) ;
			puts (sf_strerror (NULL)) ;
			exit (1) ;
			} ;

	sf_write_float (outfile, data->data, data->dataLength) ;
	sf_close (outfile) ;
}

sndData duplicateClean(sndData * data) {
	sndData tmpData;
	tmpData.data = (float *) malloc(data->dataLength * sizeof(float));
	tmpData.dataLength = data->dataLength;
	tmpData.left_phase = data->left_phase;
	tmpData.right_phase = data->right_phase;
	tmpData.nrOfChannels = data->nrOfChannels;
	tmpData.nrOfFrames = data->nrOfFrames;
	tmpData.samplingRate = data->samplingRate;
	return tmpData;
}

//FIXME - examples
//void genSin(sndData * data, int index) {
//
//	int f = index / (BUFF_LEN * 10); //FIXME
//	int freq = 500 * ((f % 13) + 1);
//
//	printf("GenSIn %d %d\n", freq, time(NULL));
//
//	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
//		float fact = 1.0;
////		if (i < 10) {
////			fact = i/10;
////		}
//
//		for (int ch = 0; ch < data->nrOfChannels; ch++) {
//			float val = fact * sin(2 * 3.1415 * freq * (index + i)/ (data->samplingRate));
//			data->data[i+ch] = val;
//		}
//	}
//}

//void playS2(sndData * data, int index) {
//	int repeat = 1000000;
//	int freq = 1000;
//
//	int sp1 = 5000;
//	int ep1 = 40000;
//
//	uLong points[6] = {5000, 40000, 75000, 90000, 220000,250000};
//	int currentSeq = -1;
//	int nrOfPoints = sizeof(points) / sizeof(uLong);
//
////	float * sinBuff = (float * ) malloc(44100 * sizeof(float));
////	sinGen(sinBuff, 1000, 44100, 44100);
//
//	sndData s = getData("sounds/kick.wav");
//
//	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
//		for (int ch = 0; ch < data->nrOfChannels; ch++) {
//			uLong cIndex = (index + i + ch) % repeat;
//
//			for (int p = 0; p < nrOfPoints; p += 2) {
//				if (cIndex >= points[p] && cIndex <= points[p+1]) {
//					currentSeq = p;
//					break;
//				} else {
//					currentSeq = -1;
//				}
//			}
//
//			if (currentSeq > -1) {
//				uLong goodIndex = (cIndex - points[currentSeq]);
//
//				float val = s.data[goodIndex % s.dataLength]; //sin(2 * 3.1415 * freq * (goodIndex) / (data->samplingRate)); //sinBuff[goodIndex];
//
//				data->data[i + ch] = val;
//			} else {
//				data->data[i + ch] = 0.0;
//			}
//
//		}
//	}
//
//	free(s.data);
//}

void sinGen(float * data, int freq, int samplingRate, long lengthInSamples) {
	if (samplingRate == 0)
		return;

	for (int i = 0; i< lengthInSamples; i++) {
		data[i] = sin(2 * 3.1415 * freq * i / samplingRate);
	}
}

void silenceGen(float * data, long lengthInSamples) {
	for (int i = 0; i< lengthInSamples; i++) {
		data[i] = 0.0;
	}
}



int main() {
	init();
}

/////

typedef struct
{
	char * name;
	void (* f)(sndData * data, int index); //TODO change int to long
}
track;

typedef unsigned long uLong;

sndData dataBuffer;
int BUFF_LEN = 4410; //FIXME
long index = 0;
track * tracks;
int nrOfTracks = 0;

void genSin(sndData * data, int index);

void ps1(sndData * data, int index);
void ps2(sndData * data, int index);
void ps3(sndData * data, int index);
void ps4(sndData * data, int index);
void ps5(sndData * data, int index);
void ps6(sndData * data, int index);
void ps7(sndData * data, int index);
void ps8(sndData * data, int index);

void sinGen(float * data, int freq, int samplingRate, long lengthInSamples);
void silenceGen(float * data, long lengthInSamples);

int audioCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData );

void createSound(sndData * data, int index);
void * threadContainer(void * arg);
sndData duplicateClean(sndData * data);
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;

void init();
void init() {

	/**
	 *
	 * Create Tracks
	 *
	 */

	tracks = (track *) malloc(10 * sizeof(track));

	track t1;
	t1.name = "t1";
	t1.f = &ps1;
	tracks[0] = t1;
	nrOfTracks++;

	track t2;
	t2.name = "t2";
	t2.f = &ps2;
	tracks[1] = t2;
	nrOfTracks++;

	track t3;
	t3.name = "t3";
	t3.f = &ps3;
	tracks[2] = t3;
	nrOfTracks++;

	track t4;
	t4.name = "t4";
	t4.f = &ps4;
	tracks[3] = t4;
	nrOfTracks++;

	track t5;
	t5.name = "t5";
	t5.f = &ps5;
	tracks[4] = t5;
	nrOfTracks++;

	track t6;
	t6.name = "t6";
	t6.f = &ps6;
	tracks[5] = t6;
	nrOfTracks++;

	track t7;
	t7.name = "t7";
	t7.f = &ps7;
	tracks[6] = t7;
	nrOfTracks++;

	track t8;
	t8.name = "t8";
	t8.f = &ps8;
	tracks[7] = t8;
	nrOfTracks++;

	/**
	 *
	 * Create Data Buffer and initialize it with "silence"
	 *
	 */

	dataBuffer.dataLength = BUFF_LEN;
	dataBuffer.left_phase = 0;
	dataBuffer.nrOfChannels = 2;
	dataBuffer.nrOfFrames = BUFF_LEN / 2;
	dataBuffer.right_phase = 0;
	dataBuffer.samplingRate = 44100;
	dataBuffer.data = (float *) malloc(BUFF_LEN * sizeof(float));

	silenceGen(dataBuffer.data, BUFF_LEN);

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
	              NULL, /* no input */
	              &outputParameters,
	              dataBuffer.samplingRate * 2,
	              BUFF_LEN,
	              paNoFlag,      /* we won't output out of range samples so don't bother clipping them */
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

	silenceGen(dataBuffer.data, BUFF_LEN);

	/**
	 *
	 * Create the sound
	 *
	 */
	createSound(&dataBuffer, index);

	/**
	 *
	 * Increment the index
	 *
	 */
	index += BUFF_LEN;

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

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = data->data[i];  /* left */

        if (data->nrOfChannels == 2) *out++ = data->data[i];  /* right */

//        data->left_phase += 1;
//        if( data->left_phase >= data->dataLength ) data->left_phase -= data->dataLength;
//
//        data->right_phase += 1;
//        if( data->right_phase >= data->dataLength ) data->right_phase -= data->dataLength;
    }

    return paContinue;
}

typedef struct {
	track * cTrack;
	sndData * data;
} threadArgContainer;

void * threadContainer(void * arg) {
	threadArgContainer * argContainer = (threadArgContainer *) arg;

	sndData tmpData = duplicateClean(argContainer->data);

	track * cTrack = argContainer->cTrack;
	(*cTrack).f(&tmpData, index);

	pthread_mutex_lock(&bufferMutex);
	mix2(argContainer->data, &tmpData);
	pthread_mutex_unlock(&bufferMutex);

	free(tmpData.data);
}

void createSound(sndData * data, int index) {

	pthread_t threads[nrOfTracks];
	threadArgContainer tac[nrOfTracks];

	int ret1 = 0;

	for (int i = 0; i < nrOfTracks; ++i) {
		threadArgContainer * taci = &tac[i];
		taci->cTrack = &tracks[i];
		taci->data = data;

		ret1 = pthread_create(&threads[i], NULL, threadContainer, (void*) taci);
	}

	for (int i = 0; i < nrOfTracks; ++i) {
		pthread_join(threads[i], NULL);
	}

}

/**
 * jss6 = '''{
    "tempo" : 120,
    "maxLen" : 32,
    "h" : "rough-house-attackmagazine/hat.wav",
    "s" : "rough-house-attackmagazine/shaker.wav",
    "ta" : "rough-house-attackmagazine/tamb.wav",
    "c" : "rough-house-attackmagazine/clap.wav",
    "o2" : "rough-house-attackmagazine/organic2.wav",
    "o1" : "rough-house-attackmagazine/organic1.wav",
    "to" : "rough-house-attackmagazine/tom.wav",
    "k" : "rough-house-attackmagazine/kick.wav",
    "patterns" : [
                 ["h", "h", "h", "h", 3, "h", "h", "h", "h", "h", 4, "h", "h", "h", "h", 3, "h", "h", "h", "h", "h", 4],
                 ["s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s", "s"],
                 [4, "ta", 7, "ta", 7, "ta", 7, "ta", 3],
                 [4, "c", 7, "c", 7, "c", "c", 1, "c", 1, "c", 2, "c", 3],
                 [2, "o2", 7, "o2", 7, "o2", 7, "o2", 5],
                 [1, "o1", 5, "o1", 1, "o1", 5, "o1", 1, "o1", 5, "o1", 1, "o1", 5 ,"o1"],
                 [1, "to", 1, "to", 2, "to", 10, "to", 1, "to", 2, "to", 10],
                 ["k", 3, "k", 3, "k", 3, "k", 3, "k", 3, "k", 3, "k", 3, "k", 3]
                 ]

}
'''
 */

int tempo = 240;

void ps1(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / tempo) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t1 %d %d %d", framesPerSeq, totalFrames, index);

	int points[36] = {0,1, 1,2, 2,3, 3,4, 7,8, 8,9, 9,10, 10,11, 11,12, 16,17, 17,18, 18,19, 19,20, 23,24, 24,25, 25,26, 26,27, 27,28};

	int currentSeq = -1;
	int nrOfPoints = sizeof(points) / sizeof(int);

	sndData s = getData("sounds/hat.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				float val = s.data[goodIndex % s.dataLength];

				data->data[i + ch] = val;
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}

void ps2(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / tempo) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t2 %d %d %d", framesPerSeq, totalFrames, index);

	int points[64] = {0,1, 1,2, 2,3, 3,4, 4,5, 5,6, 6,7, 7,8, 8,9, 9,10, 10,11, 11,12, 12,13, 13,14, 14,15, 15,16, 16,17, 17,18, 18,19, 19,20, 20,21, 21,22, 22,23, 23,24, 24,25, 25,26, 26,27, 27,28, 28,29, 29,30, 30,31, 31,32};

	int currentSeq = -1;
	int nrOfPoints = sizeof(points) / sizeof(int);

	sndData s = getData("sounds/shaker.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				float val = s.data[goodIndex % s.dataLength];

				data->data[i + ch] = val;
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}

void ps3(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / (tempo)) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t3 %d %d %d", framesPerSeq, totalFrames, index);

//	int points[8] = {4,5, 13,14, 21,22, 29,30};
	int points[8] = {4, 12,12, 20,20, 28,28, 32};

	int currentSeq = -1;
	int nrOfPoints = 8; //sizeof(points) / sizeof(int);

	sndData s = getData("sounds/tamb.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				float val = s.data[goodIndex % s.dataLength];

				data->data[i + ch] = val;
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}

void ps4(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / (tempo)) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t4 %d %d %d", framesPerSeq, totalFrames, index);

//	int points[14] = {4,12, 12,21, 20,21, 21,22, 23,24, 25,26, 28,29};
	int points[14] = {4,12, 12,20, 20,21, 21,23, 23,25, 25,28, 28,32};

	int currentSeq = -1;
	int nrOfPoints = sizeof(points) / sizeof(int);

	sndData s = getData("sounds/clap.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				float val = s.data[goodIndex % s.dataLength];

				data->data[i + ch] = val;
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}

void ps5(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / (tempo)) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t5 %d %d %d", framesPerSeq, totalFrames, index);

//	int points[10] = {2,3, 10,11, 18,19, 21,22, 26,27};
	int points[10] = {2,10, 10,18, 18,21, 21,26, 26,32};


	int currentSeq = -1;
	int nrOfPoints = sizeof(points) / sizeof(int);

	sndData s = getData("sounds/organic2.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				float val = s.data[goodIndex % s.dataLength];

				data->data[i + ch] = val;
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}


void ps6(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / (tempo)) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t6 %d %d %d", framesPerSeq, totalFrames, index);


//	int points[16] = {1,2, 7,8, 9,15, 15,17, 17,23, 23,25, 25,31, 31,32};
	int points[16] = {1,7, 7,9, 9,15, 15,17, 17,23, 23,25, 25,31, 31,32};


	int currentSeq = -1;
	int nrOfPoints = sizeof(points) / sizeof(int);

	sndData s = getData("sounds/organic1.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				if (goodIndex <= s.dataLength) {
					float val = s.data[goodIndex]; // % s.dataLength
					data->data[i + ch] = val;
				} else {
					data->data[i + ch] = 0.0;
				}
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}

void ps7(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / (tempo)) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t7 %d %d %d", framesPerSeq, totalFrames, index);

//	int points[12] = {1,2, 3,4, 6,7, 17,18, 19,20, 22,23};
	int points[12] = {1,3, 3,6, 6,17, 17,19, 19,22, 22,32};

	int currentSeq = -1;
	int nrOfPoints = sizeof(points) / sizeof(int);

	sndData s = getData("sounds/tom.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				if (goodIndex <= s.dataLength) {
					float val = s.data[goodIndex]; // % s.dataLength
					data->data[i + ch] = val;
				} else {
					data->data[i + ch] = 0.0;
				}
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}

void ps8(sndData * data, int index) {
	int nrOfSeqs = 32;
	int samplingRate = 44100;
	int framesPerSeq = (60.0 / (tempo)) * samplingRate;
	int totalFrames = nrOfSeqs * framesPerSeq;

	int repeat = totalFrames;

	printf("t7 %d %d %d", framesPerSeq, totalFrames, index);

//	int points[16] = {0,1, 4,5, 8,9, 12,13, 16,17, 20,21, 24,25, 28,29};
	int points[16] = {0,4, 4,8, 8,12, 12,16, 16,20, 20,24, 24,28, 28,32};

	int currentSeq = -1;
	int nrOfPoints = sizeof(points) / sizeof(int);

	sndData s = getData("sounds/kick.wav");

	for (int i = 0; i < data->dataLength; i += data->nrOfChannels) {
		for (int ch = 0; ch < data->nrOfChannels; ch++) {
			uLong cIndex = (index + i + ch) % repeat;

			for (int p = 0; p < nrOfPoints; p += 2) {
				if (cIndex >= (points[p] * framesPerSeq) && cIndex <= (points[p+1] * framesPerSeq)) {
					currentSeq = p;
					break;
				} else {
					currentSeq = -1;
				}
			}

			if (currentSeq > -1) {
				uLong goodIndex = (cIndex - (points[currentSeq] * framesPerSeq));

				if (goodIndex <= s.dataLength) {
					float val = s.data[goodIndex]; // % s.dataLength
					data->data[i + ch] = val;
				} else {
					data->data[i + ch] = 0.0;
				}
			} else {
				data->data[i + ch] = 0.0;
			}

		}
	}

	free(s.data);
}
