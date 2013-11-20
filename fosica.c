#include <portaudio.h>
#include <sndfile.h>
#include <samplerate.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>

#define FRAMES_PER_BUFFER  (64)


float genSin(int frequency, int currentSample, int sampleRate);
float genSin(int frequency, int currentSample, int sampleRate) {

	if (sampleRate == 0)
		return 0;

	return sin(2 * 3.1415 * frequency * currentSample / sampleRate);
}

float * genSinS(int frequency, int milliSeconds, int sampleRate);
float * genSinS(int frequency, int milliSeconds, int sampleRate) {

	if (milliSeconds <= 0 || sampleRate <= 0 || frequency <= 0)
		return 0;

	int totalFrames = (int)milliSeconds/1000 * sampleRate;

	float * data = (float *) malloc(totalFrames * sizeof(float));

	for (int i = 0; i < totalFrames; i++) {
		data[i] = genSin(frequency, i, sampleRate);
	}

	return data;
}

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
paTestData;

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = data->data[data->left_phase];  /* left */

        if (data->nrOfChannels == 2) *out++ = data->data[data->right_phase];  /* right */

        data->left_phase += 1;
        if( data->left_phase >= data->dataLength ) data->left_phase -= data->dataLength;

        data->right_phase += 1;
        if( data->right_phase >= data->dataLength ) data->right_phase -= data->dataLength;
    }

    return paContinue;
}

static void StreamFinished( void* userData )
{
   paTestData *data = (paTestData *) userData;
   printf( "Stream Completed: \n");
}

paTestData getData(char * fileName);
paTestData getData(char * fileName) {
	SNDFILE *sf;

	SF_INFO info;
	int num_channels;
	int num, num_items;
	float *buf;
	int f, sr, c;
	int i, j;
	FILE *out;

	paTestData testData;

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

paTestData resample(paTestData * data, int newSamplingRate);
paTestData resample(paTestData * data, int newSamplingRate) {

	float ratio = newSamplingRate / data->samplingRate;

	paTestData resultData;
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



paTestData changeNrOfChannels(paTestData * data, int newNrChannels, long startPoint);
paTestData changeNrOfChannels(paTestData * data, int newNrChannels, long startPoint) {
	paTestData resultData;

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

		printf("WW  QQQQQ WEESDKSKDHSJDH ");

		resultData.nrOfFrames = data->nrOfFrames;
		resultData.nrOfChannels = newNrChannels;
		resultData.samplingRate = data->samplingRate;
		resultData.left_phase = data->left_phase;
		resultData.right_phase = data->right_phase;
		resultData.dataLength = data->dataLength * 2;
	}

	return resultData;
}

paTestData mix(const paTestData * data1, const paTestData * data2);
paTestData mix(const paTestData * data1, const paTestData * data2) {
	paTestData resultData;
	paTestData * maxData;
	paTestData * minData;

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
				resultData.data[i] = (maxData->data[i] + minData->data[i]) / 2;
			} else {
				resultData.data[i] = maxData->data[i];
			}
		}

	}

	resultData.nrOfFrames = maxData->nrOfFrames;
	resultData.nrOfChannels = maxData->nrOfChannels;
	resultData.samplingRate = maxData->samplingRate;
	resultData.left_phase = maxData->left_phase;
	resultData.right_phase = maxData->right_phase;
	resultData.dataLength = maxData->dataLength;

	return resultData;
}

void writeDataToFile(paTestData * data, char * fileName);
void writeDataToFile(paTestData * data, char * fileName) {
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

int main()
    {


    paTestData testData1 = getData("tom.wav");
    paTestData testData2 = getData("shaker.wav");
    paTestData testData3 = getData("test2.wav");
    paTestData testData4 = getData("test1.wav");

    paTestData testData31 = changeNrOfChannels(&testData3, 2, 0);
    paTestData testData41 = changeNrOfChannels(&testData4, 2, 0);

    paTestData testData42 = resample(&testData4, 44100);
    paTestData testData32 = resample(&testData31, 44100);


//    paTestData testData = mix(&testData1, &testData2);
//    paTestData testData = mix(&testData31, &testData41);
    paTestData testData = mix(&testData31, &testData41);

    writeDataToFile(&testData, "gygy.wav");

    exit(1);

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
    outputParameters.channelCount = testData.nrOfChannels;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              testData.samplingRate, // * testData.nrOfChannels,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &testData );
    if( err != paNoError ) goto error;

    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    printf("Play for %d seconds.\n", 1);
    Pa_Sleep( 10 * 1000 );

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    Pa_Terminate();
    printf("Test finished.\n");

    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;


 }
