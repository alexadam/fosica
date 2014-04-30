#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <samplerate.h>
#include <sndfile.h>
#include <portaudio.h>
#include <jansson.h>

#include "noiz_utils.h"

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

