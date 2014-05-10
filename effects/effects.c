#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <samplerate.h>
#include <sndfile.h>
#include <portaudio.h>
#include <jansson.h>

#include "../utils/utils.h"
#include "../utils/noiz_utils.h"
#include "../cache/cache.h"
#include "effects.h"

F_PTR getFWrapper(char * name) {

	if (strcmp(name, "repeat") == 0)
		return f_repeat;
	if (strcmp(name, "file") == 0)
		return f_file;
	if (strcmp(name, "mix") == 0)
		return f_mix;
	if (strcmp(name, "channel") == 0)
		return f_channel;
	if (strcmp(name, "sinOsc") == 0)
		return f_sin_osc;
	if (strcmp(name, "triOsc") == 0)
		return f_tri_osc;
	if (strcmp(name, "sqrOsc") == 0)
		return f_sqr_osc;
	if (strcmp(name, "sawOsc") == 0)
		return f_saw_osc;
	if (strcmp(name, "exp") == 0)
		return f_exp;
	if (strcmp(name, "reverb") == 0)
		return f_reverb;
	if (strcmp(name, "ce") == 0)
		return f_custom_envelope;
	if (strcmp(name, "f1") == 0)
		return f_lowpass;
	if (strcmp(name, "fm") == 0)
		return f_fm;
	if (strcmp(name, "mul") == 0)
		return f_mul;
	if (strcmp(name, "div") == 0)
		return f_div;
	if (strcmp(name, "add") == 0)
		return f_add;
	if (strcmp(name, "sub") == 0)
		return f_sub;

	return NULL;
}



//float sinGen(int index, int freq, unsigned long int lengthInSamples) {
//	if (lengthInSamples == 0) {
//		lengthInSamples = 1;
//	}
//
//	return sin(6.283 * freq * index / lengthInSamples);
//}
//
//void sinGenArray(float * data, int freq, int samplingRate, unsigned long int lengthInSamples) {
//    if (samplingRate == 0)
//        return;
//
//    for (int i = 0; i< lengthInSamples; i++) {
//        data[i] = sin(2 * 3.1415 * freq * i / samplingRate);
//    }
//}
//
//void silenceGenArray(float * data, unsigned long int lengthInSamples) {
//    for (int i = 0; i< lengthInSamples; i++) {
//        data[i] = 0.0;
//    }
//}
//
//float * sinGenA(int freq, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples) {
////	return sinGenPhase(freq, 0, startIndex, nrOfSamples, totalNrOfSamples);
//
//	float * res = malloc(nrOfSamples * sizeof(float));
//
//		for (int i = 0; i < nrOfSamples; ++i) {
//			res[i] = sin((6.283 * freq * (startIndex + i) )/ totalNrOfSamples);
//		}
//
//		return res;
//}
//
//float * sinGenPhase(int freq, int phase, unsigned long int startIndex, unsigned long int nrOfSamples, unsigned long int totalNrOfSamples) {
//	float * res = malloc(nrOfSamples * sizeof(float));
//
//	for (int i = 0; i < nrOfSamples; ++i) {
//		res[i] = sin((6.283 * freq * (startIndex + i) + phase * 3.1415 / 180)/ totalNrOfSamples);
//	}
//
//	return res;
//}
//
//void f_repeat(int index, int bufferLen, int repeatAtIndex, int nrOfSeqs, int seqLen, int nrOfPoints, int * points, float * input, float * output ) {
//
//}

void f_mix(FUNCTION_DATA * function_data) {

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	for (int i = 0; i < input_fb->size; ++i) {
		input_fb->functions[i]->f_ptr(input_fb->functions[i]->f_data);
	}

	for (int i = 0; i < function_data->globalData->bufferLen; ++i) {
		function_data->output[i] = 0.0;
		for (int j = 0; j < input_fb->size; ++j) {
			function_data->output[i] += input_fb->functions[j]->f_data->output[i] * 0.8; //TODO
		}
	}
}

CACHE * localCache = NULL;

sndData * getCachedSnd(char * name, char * saveName, int reverse, int samplingRate) {

	if (localCache == NULL) {
		localCache = initCache(10);
	}

	void* co;
	co = getCachedObj(localCache, saveName);

	if (co == NULL) {
		sndData snd;
		snd = readFile(name);

		if (reverse == 1) {
			for (int i = 0; i < snd.dataLength / 2; ++i) {
				int ri = snd.dataLength - i - 1;
				float tmp = snd.data[i];
				snd.data[i] = snd.data[ri];
				snd.data[ri] = tmp;
			}
		}

		if (samplingRate != 0 && samplingRate != snd.samplingRate) {
			snd = resample(&snd, samplingRate);
		}

		co = putObjInCache(localCache, &snd, sizeof(snd), saveName);
	}

	return co;
}

void f_repeat(FUNCTION_DATA * function_data) {
	// repeatIndex, maxSequences, tempo, nrOfPoints, [points]

	if (function_data->paramSize != 5) {
		printf("Wrong params - f_repeat");
		return;
	}

	int repeatIndex = function_data->params[0]->intVal;
	int totalNrOfSeqs = function_data->params[1]->intVal;
	int tempo = function_data->params[2]->intVal;
	int nrOfPoints = function_data->params[3]->intVal;
	int * points = function_data->params[4]->intListVal;
	int samplingRate = function_data->globalData->samplingRate; // TODO ??? framesPerSecond must be multiple of channel nr + what happens if inputdata.dataLength is even/ not multiple of nr channels
	int framesPerSeq = (60.0 / tempo) * samplingRate;
	int totalFrames = totalNrOfSeqs * framesPerSeq;
	int repeat = totalFrames;
	int currentSeq = -1;

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	int lastComputedSeq = -1;

	float bb = 0.0;

	for (int i = 0; i < function_data->globalData->bufferLen; i += 1) {

//		int ch = i % data->nrOfChannels;

		int mainIndex = function_data->globalData->index + i;
		int trackIndex = mainIndex % repeat;

		currentSeq = (int)trackIndex / framesPerSeq;
		int foundSeq = 0;

		for (int var = 0; var < nrOfPoints; var += 2) {

			if (currentSeq >= points[var] && currentSeq < points[var + 1]) {
				foundSeq = 1;

				int sequenceIndex = (trackIndex - (points[var] * framesPerSeq));

				if (currentSeq != lastComputedSeq) {
					input_fb->functions[0]->f_data->globalData->index = sequenceIndex;
					input_fb->functions[0]->f_ptr(input_fb->functions[0]->f_data);
					lastComputedSeq = currentSeq;
				}

				//sin(2 * 3.1415 * 5 * i / function_data->globalData->bufferLen) * 0.6;
				function_data->output[i] = input_fb->functions[0]->f_data->output[i];

				break;
			}
		}

		if (foundSeq == 0) {
			function_data->output[i] = 0.0;
		}

	}

}

void f_file(FUNCTION_DATA * function_data) {
	// name, reverse, samplingRate

	if (function_data->paramSize != 3) {
		printf("Wrong params - f_file");
		return;
	}

	char * fileName = function_data->params[0]->charVal;
	char * samplingRateStr = int2string(function_data->params[1]->intVal);
	char * reverseStr = int2string(function_data->params[2]->intVal);

	char * newFileName = localconcat(localconcat(fileName, samplingRateStr), reverseStr);

	float bb = 0.0;

	sndData* csnd;
	csnd = getCachedSnd(fileName, newFileName, function_data->params[1]->intVal, function_data->params[2]->intVal);

	if (csnd == NULL) {
		return;
	}

	for (int i = 0; i < function_data->globalData->bufferLen; ++i) {
		int t_index = function_data->globalData->index + i;

		if (t_index >= csnd->dataLength) {
			function_data->output[i] = 0.0;
		} else {
			function_data->output[i] = csnd->data[t_index];
		}
	}

//	free(fileName);
//	free(samplingRateStr);
//	free(reverseStr);

}

void f_channel(FUNCTION_DATA * function_data) {
	// channel -> ex: channel,0,<in (one input); 0 -> left channel; 1 -> right channel; other nr -> both channels

	if (function_data->paramSize != 1) {
		printf("Wrong params - f_channel");
		return;
	}

	int channel = function_data->params[0]->intVal;

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	input_fb->functions[0]->f_ptr(input_fb->functions[0]->f_data);

	for (int i = 0; i < function_data->globalData->bufferLen; i += 1) {
		int ch = i % function_data->globalData->nrOfChannels;
		if (ch == channel) {
			function_data->output[i] = 0.0;
		} else {
			function_data->output[i] = input_fb->functions[0]->f_data->output[i];
		}
	}
}

void f_sin_osc(FUNCTION_DATA * function_data) {
	// freq, phase, period

	if (function_data->paramSize != 3) {
		printf("Wrong params - f_sin_osc\n");
		return;
	}

	int freq = function_data->params[0]->intVal;
	int phase = function_data->params[1]->intVal;
	int period_frames = function_data->params[2]->intVal;
	int startIndex = function_data->globalData->index;

	if (period_frames == 0) {
		period_frames = function_data->globalData->samplingRate;
	}

	period_frames *= function_data->globalData->nrOfChannels;

	for (int i = 0; i < (int)function_data->globalData->bufferLen; i++) {
		float sv = sin((6.283 * freq * (startIndex + i) + phase * 3.1415 / 180)/ period_frames);

		function_data->output[i] = sv;
	}
}

void f_tri_osc(FUNCTION_DATA * function_data) {
	// freq, period

	if (function_data->paramSize != 2) {
		printf("Wrong params - f_tri_osc");
		return;
	}

	int freq = function_data->params[0]->intVal;
	int period_frames = function_data->params[1]->intVal;
	int startIndex = function_data->globalData->index;

	if (period_frames == 0) {
		period_frames = function_data->globalData->samplingRate;
	}

	period_frames *= function_data->globalData->nrOfChannels;

	int per = period_frames / freq;
	int hper = per / 2;

	for (int i = 0; i < function_data->globalData->bufferLen; i++) {
		int ni = startIndex + i;
//		float sv = (float)(hper - abs((startIndex + ni) % (2 * hper) - hper)) / hper; //(float)(hper - abs((startIndex + ni) % (2 * hper) - per)) / hper;// (float)((startIndex + ni) % (hper)) / hper;
//		float sv = (float)2 * ((float)(startIndex + ni)/hper - floor(0.5 + (startIndex + ni)/hper));
//		float sv = (float)2/hper * ((startIndex + ni) - hper * floor((float)(startIndex + ni)/hper + 0.5)) / ((float)(startIndex + ni)/hper + 0.5); //spiral

		if (ni % per < hper) {
			function_data->output[i] =  4 * (float)(ni % per)/per - 1;
		} else {
			function_data->output[i] = 3 - 4 * (float)(ni % per)/per;
		}
	}
}

void f_sqr_osc(FUNCTION_DATA * function_data) {
	// freq, period

	if (function_data->paramSize != 2) {
		printf("Wrong params - f_sqr_osc");
		return;
	}

	int freq = function_data->params[0]->intVal;
	int period_frames = function_data->params[1]->intVal;
	int startIndex = function_data->globalData->index;

	if (period_frames == 0) {
		period_frames = function_data->globalData->samplingRate;
	}

	period_frames *= function_data->globalData->nrOfChannels;

	int per = period_frames / freq;
	int hper = per / 2;

	for (int i = 0; i < function_data->globalData->bufferLen; i++) {
		int ni = startIndex + i;

		float sv = 0.0;

		if (ni % per < hper) {
			function_data->output[i] = 1.0;
		} else {
			function_data->output[i] = 0.0;
		}
	}
}

void f_saw_osc(FUNCTION_DATA * function_data) {
	// freq, period

	if (function_data->paramSize != 2) {
		printf("Wrong params - f_saw_osc");
		return;
	}

	int freq = function_data->params[0]->intVal;
	int period_frames = function_data->params[1]->intVal;
	int startIndex = function_data->globalData->index;

	if (period_frames == 0) {
		period_frames = function_data->globalData->samplingRate;
	}

	period_frames *= function_data->globalData->nrOfChannels;

	int per = period_frames / freq;
	int hper = per / 2;

	for (int i = 0; i < function_data->globalData->bufferLen; i++) {
		int ni = startIndex + i;
		function_data->output[i] = 2 * (float) (ni % per) / per -1;
	}
}

void f_exp(FUNCTION_DATA * function_data) {
	// freq, totalFrames -> if totalFrames == 0 -> totalFrames = global frame rate

	if (function_data->paramSize != 0) {
		printf("Wrong params - f_exp");
		return;
	}

	int freq = function_data->params[0]->intVal;
	int period_frames = function_data->params[1]->intVal;
	int startIndex = function_data->globalData->index;

	if (period_frames == 0) {
		period_frames = function_data->globalData->samplingRate;
	}

	int per = period_frames / freq;
	int hper = per / 2;

	for (int i = 0; i < function_data->globalData->bufferLen; i++) {
		int ni = startIndex + i;
		function_data->output[i] = 1 / exp(ni);
	}
}

void f_fm(FUNCTION_DATA * function_data) {
	// frequency modulation
	// freq_carrier, freq_carried, period

	if (function_data->paramSize != 3) {
		printf("Wrong params - f_sin_osc");
		return;
	}

	int freq_carrier = function_data->params[0]->intVal;
	int freq_carried = function_data->params[1]->intVal;
	int period_frames = function_data->params[2]->intVal;
	int startIndex = function_data->globalData->index;

	if (period_frames == 0) {
		period_frames = function_data->globalData->samplingRate;
	}

	period_frames *= function_data->globalData->nrOfChannels;

	for (int i = 0; i < function_data->globalData->bufferLen; i++) {
		function_data->output[i] = sin(6.283 * freq_carrier * (startIndex + i) / period_frames + sin(6.283 * freq_carried * (startIndex + i) / period_frames));
	}
}

void f_reverb(FUNCTION_DATA * function_data) {
	// delay, decay

	if (function_data->paramSize != 2) {
		printf("Wrong params - f_reverb\n");
		return;
	}

	int delay = function_data->params[0]->intVal;
	float decay = function_data->params[1]->floatVal;
	int startIndex = function_data->globalData->index;

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	for (int i = 0; i < input_fb->size; ++i) {
		input_fb->functions[i]->f_ptr(input_fb->functions[i]->f_data);
	}

	for (int i = 0; i < function_data->globalData->bufferLen - delay; i++) {
		if (i < delay) {
			function_data->output[i] = input_fb->functions[0]->f_data->output[i];
		} else 	if (i + delay < function_data->globalData->bufferLen) {
			function_data->output[i] = function_data->output[i - delay] * decay;
		}
	}
}

void f_lowpass(FUNCTION_DATA * function_data) {
	// ?

	int cutOffFreq = 2000;
	int period_frames = 0; //function_data->params[1]->intVal;
	int startIndex = function_data->globalData->index;

	if (period_frames == 0) {
		period_frames = function_data->globalData->samplingRate;
	}

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	for (int i = 0; i < input_fb->size; ++i) {
		input_fb->functions[i]->f_ptr(input_fb->functions[i]->f_data);
	}

	for (int i = 0; i < function_data->globalData->bufferLen; i++) {
		if (i < 2) {
			function_data->output[i] = input_fb->functions[0]->f_data->output[i];
			continue;
		}



		float ang_freq = sin(6.283 * cutOffFreq * (startIndex + i)/ period_frames);
		float qual_factor = 1.0 / sqrt(2);
		float tmp = 1.0 / qual_factor;
		float beta = (float)((1 - (tmp / 2) * sin(ang_freq)) / (1 + (tmp / 2) * sin(ang_freq))) / 2;
		float gamma = (0.5 + beta) * cos(ang_freq);

//		printf("WWW %f %f %f %f\n", ang_freq, qual_factor, tmp, gamma);

		//low pass coef
		float a0 = (0.5 + beta - gamma) / 2;
		float a1 = 0.5 + beta - gamma;
		float a2 = a0;
		float b1 = -1 * gamma;
		float b2 = 1 * beta;

//		printf("QQQ %f %f %f %f\n", a1, a2, b1, b2);

		//high pass coef
//		float a0 = (0.5 + beta - gamma) / 2;
//		float a1 = -(0.5 + beta - gamma);
//		float a2 = a0;
//		float b1 = -2 * gamma;
//		float b2 = 2 * beta;

		function_data->output[i] = a0 * input_fb->functions[0]->f_data->output[i] +
				a1 * input_fb->functions[0]->f_data->output[i-1] +
				a2 * input_fb->functions[0]->f_data->output[i-2] -
				b1 * function_data->output[i - 1] -
				b2 * function_data->output[i - 2];

		function_data->output[i] *= 0.5;
	}
}

void f_custom_envelope(FUNCTION_DATA * function_data) {
	//freq, period, nrPoints, [x1 y1 x2 y2 x3 y3 ....] -> float array with wave coordinates

	if (function_data->paramSize != 4) {
		printf("Wrong params - f_ce\n");
		return;
	}

	int freq = function_data->params[0]->intVal;
	int samplingRate = function_data->params[1]->intVal;
	int nrOfPoints = function_data->params[2]->intVal;
	float * points = function_data->params[3]->floatListVal;

	if (samplingRate == 0) {
		samplingRate = function_data->globalData->samplingRate; // TODO ??? framesPerSecond must be multiple of channel nr + what happens if inputdata.dataLength is even/ not multiple of nr channels
	}

	samplingRate *= function_data->globalData->nrOfChannels;

	int framesPerEnvelope = samplingRate / freq;
	float unitsPerSample = 1.0 / framesPerEnvelope;

	for (int i = 0; i < function_data->globalData->bufferLen; i += 1) {

		int mainIndex = function_data->globalData->index + i;
		int envelopeIndex = mainIndex % framesPerEnvelope;
		float currentX = unitsPerSample * envelopeIndex;
		int foundSeq = 0;

		for (int var = 0; var < nrOfPoints; var += 2) {

			if (currentX >= points[var] && currentX < points[var + 2]) {
				foundSeq = 1;

				float dY = points[var + 3] - points[var + 1];
				float dX = points[var + 2] - points[var];
				float dCX = currentX - points[var];
				float rez = points[var + 1] + dY * dCX / dX;

				function_data->output[i] = rez;

				break;
			}
		}

		if (foundSeq == 0) {
			function_data->output[i] = 0.0;
		}

	}
}

void f_mul(FUNCTION_DATA * function_data) {
	// mul,<i1 i2 ... in

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	for (int i = 0; i < input_fb->size; ++i) {
		input_fb->functions[i]->f_ptr(input_fb->functions[i]->f_data);
	}

	for (int i = 0; i < function_data->globalData->bufferLen; ++i) {
		function_data->output[i] = 1.0;
		for (int j = 0; j < input_fb->size; ++j) {
			function_data->output[i] *= input_fb->functions[j]->f_data->output[i];
		}
	}
}

void f_add(FUNCTION_DATA * function_data) {
	// add,<i1 i2 ... in

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	for (int i = 0; i < input_fb->size; ++i) {
		input_fb->functions[i]->f_ptr(input_fb->functions[i]->f_data);
	}

	for (int i = 0; i < function_data->globalData->bufferLen; ++i) {
		function_data->output[i] = 0.0;
		for (int j = 0; j < input_fb->size; ++j) {
			function_data->output[i] += input_fb->functions[j]->f_data->output[i];
		}
	}
}

void f_sub(FUNCTION_DATA * function_data) {
	// sub,<i1 i2 ... in

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	for (int i = 0; i < input_fb->size; ++i) {
		input_fb->functions[i]->f_ptr(input_fb->functions[i]->f_data);
	}

	for (int i = 0; i < function_data->globalData->bufferLen; ++i) {
		function_data->output[i] = input_fb->functions[0]->f_data->output[i];
		for (int j = 1; j < input_fb->size; ++j) {
			function_data->output[i] -= input_fb->functions[j]->f_data->output[i];
		}
	}
}

void f_div(FUNCTION_DATA * function_data) {
	// div,<i1 i2 ... in

	FUNCTION_BUFFERS * input_fb = (FUNCTION_BUFFERS *) function_data->input;

	for (int i = 0; i < input_fb->size; ++i) {
		input_fb->functions[i]->f_ptr(input_fb->functions[i]->f_data);
	}

	for (int i = 0; i < function_data->globalData->bufferLen; ++i) {
		function_data->output[i] = input_fb->functions[0]->f_data->output[i];
		for (int j = 1; j < input_fb->size; ++j) {
			float tmp = input_fb->functions[j]->f_data->output[i];
			if (tmp == 0.0) {
				tmp = 1.0;
			}
			function_data->output[i] /= tmp;
		}
	}
}

void f_at_frame(FUNCTION_DATA * function_data) {
	// startFrame, endFrame -> ex: at_frame,1780043,666542,<i1  (single input)
	//TODO

}
