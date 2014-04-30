
#ifndef NOIZ_UTILS_H_
#define NOIZ_UTILS_H_


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
    float * buffer;
    int start; //TODO long int
    int stop;
    int lastStart;
    int lastStop;
    int lastHash;
    int bufferLenInSamples;
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

sndData resample(sndData * data, int newSamplingRate);

double apply_gain(float * data, long frames, int channels, double max, double gain);

sndData changeNrOfChannels(sndData * data, int newNrChannels, long startPoint);

void mix2(const sndData * toData, const  sndData * fromData);

sndData mix(const sndData * data1, const sndData * data2);

sndData duplicateClean(sndData * data);

sndData readFile(char * fileName);

void writeDataToFile(sndData * data, char * fileName);

SNDFILE * openFileToWrite(sndData * data, char * fileName);

void writeToFile(SNDFILE *outfile, sndData * data);

void closeFile(SNDFILE *outfile);

#endif /* NOIZ_UTILS_H_ */
