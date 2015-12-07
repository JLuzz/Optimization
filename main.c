#include <stdio.h>
#include <stdlib.h>

struct waveData{

	int valid;

	char chunkID[4];
	int chunkSize;
	char format[4];

	char subChunk1ID[4];
	int subChunk1Size;
	short audioFormat;
	short numChannels;
	int sampleRate;
	int byteRate;
	short blockAlign;
	short bitsPerSample;

	int channelSize;
	char subChunk2ID[4];
	int subChunk2Size;

	short* data;

};

void printWave(struct waveData waveD)
{
	printf("\n============= HEADER INFO =============\n");
	printf(" chunkID:%s\n", waveD.chunkID);
	printf(" chunkSize:%d\n", waveD.chunkSize);
	printf(" format:%s\n", waveD.format);
	printf(" subChunk1ID:%s\n", waveD.subChunk1ID);
	printf(" subChunk1Size:%d\n", waveD.subChunk1Size);
	printf(" audioFormat:%d\n", waveD.audioFormat);
	printf(" numChannels:%d\n", waveD.numChannels);
	printf(" sampleRate:%d\n", waveD.sampleRate);
	printf(" byteRate:%d\n", waveD.byteRate);
	printf(" blockAlign:%d\n", waveD.blockAlign);
	printf(" bitsPerSample:%d\n", waveD.bitsPerSample);
	printf(" subChunk2ID:%s\n", waveD.subChunk2ID);
	printf(" subChunk2Size:%d\n", waveD.subChunk2Size);
}

struct waveData loadWave(char* filename)
{
	FILE* in = fopen(filename, "rb");
	struct waveData wave;

	if (in != NULL)
	{

		printf("Reading %s...\n",filename);

		fread(wave.chunkID, 1, 4, in);
		fread(&wave.chunkSize, 1, 4, in);
		fread(wave.format, 1, 4, in);

		//sub chunk 1
		fread(wave.subChunk1ID, 1, 4, in);
		fread(&wave.subChunk1Size, 1, 4, in);
		fread(&wave.audioFormat, 1, 2, in);
		fread(&wave.numChannels, 1, 2, in);
		fread(&wave.sampleRate, 1, 4, in);
		fread(&wave.byteRate, 1, 4, in);
		fread(&wave.blockAlign, 1, 2, in);
		fread(&wave.bitsPerSample, 1, 2, in);

		//read extra bytes
		if(wave.subChunk1Size == 18)
		{
			short empty;
			fread(&empty, 1, 2, in);
		}

		//sub chunk 2
		fread(wave.subChunk2ID, 1, 4, in);
		fread(&wave.subChunk2Size, 1, 4, in);

		//read data
		int bytesPerSample = wave.bitsPerSample/8;
		int numSamples = wave.subChunk2Size / bytesPerSample;
		wave.data = (short*) malloc(sizeof(short) * numSamples);

		//fread(data, 1, bytesPerSample*numSamples, in);

		int i=0;
		short sample=0;
		while(fread(&sample, 1, bytesPerSample, in) == bytesPerSample)
		{
			wave.data[i++] = sample;
			sample = 0;
		}

		fclose(in);
		printf("Closing %s...\n",filename);
	}
	else
	{
		printf("Can't open file\n");
		wave.valid = 0;
		return wave;
	}

	wave.valid = 1;
	return wave;
}

int saveWave(struct waveData wave, char* filename)
{
	FILE* out = fopen(filename, "wb");

	if (out != NULL)
	{
		printf("\nWriting %s...\n",filename);

		fwrite(wave.chunkID, 1, 4, out);
		fwrite(&wave.chunkSize, 1, 4, out);
		fwrite(wave.format, 1, 4, out);

		//sub chunk 1
		fwrite(wave.subChunk1ID, 1, 4, out);
		fwrite(&wave.subChunk1Size, 1, 4, out);
		fwrite(&wave.audioFormat, 1, 2, out);
		fwrite(&wave.numChannels, 1, 2, out);
		fwrite(&wave.sampleRate, 1, 4, out);
		fwrite(&wave.byteRate, 1, 4, out);
		fwrite(&wave.blockAlign, 1, 2, out);
		fwrite(&wave.bitsPerSample, 1, 2, out);

		//read extra bytes
		if(wave.subChunk1Size == 18)
		{
			short empty = 0;
			fwrite(&empty, 1, 2, out);
		}

		//sub chunk 2
		fwrite(wave.subChunk2ID, 1, 4, out);
		fwrite(&wave.subChunk2Size, 1, 4, out);

		//read data
		int bytesPerSample = wave.bitsPerSample / 8;
		int sampleCount =  wave.subChunk2Size / bytesPerSample;
/*
		//impulse response - echo
		int IRSize = 6;
		float IR[IRSize];
		IR[0] = 0.0;
		IR[1] = 1.0;
		IR[2] = 1.0;
		IR[3] = 1.0;
		IR[4] = 1.0;
		IR[5] = 0.0;

		//write the data
		float* newData = (float*) malloc(sizeof(float) * (sampleCount + IRSize - 1));
		float maxSample = -1;
		float MAX_VAL = 32767.f;	//FIXME: find based on bits per sample

		for(int i=0; i<sampleCount; ++i)
		{

			//convolve
			for(int j=0; j<IRSize; ++j)
				newData[i+j] += ((float) wave.data[i] / MAX_VAL) * IR[j];

			//Keep track of max value for scaling
			if(i==0)
				maxSample = newData[0];
			else if(newData[i] > maxSample)
				maxSample = newData[i];
		}

		//scale and re write the data
		for(int i=0; i<sampleCount + IRSize - 1; ++i)
		{
			newData[i] = (newData[i] / maxSample) ;
			short sample = (short) (newData[i] * MAX_VAL);
			fwrite(&sample, 1, bytesPerSample, out);
		}

		//clean up
		free(newData);*/
		fwrite(wave.data, 1, bytesPerSample*sampleCount, out);
		fclose(out);
		printf("Closing %s...\n",filename);
	}
	else
	{
		printf("Can't open file\n");
		return 0;
	}
	return 1;
}

/*****************************************************************************
*
*    Function:     convolve
*
*    Description:  Convolves two signals, producing an output signal.
*                  The convolution is done in the time domain using the
*                  "Input Side Algorithm" (see Smith, p. 112-115).
*
*    Parameters:   x[] is the signal to be convolved
*                  N is the number of samples in the vector x[]
*                  h[] is the impulse response, which is convolved with x[]
*                  M is the number of samples in the vector h[]
*                  y[] is the output signal, the result of the convolution
*                  P is the number of samples in the vector y[].  P must
*                       equal N + M - 1
*
*****************************************************************************/
/*
void convolve(float x[], int N, float h[], int M, float y[], int P)
{
  int n, m;

  //  Make sure the output buffer is the right size: P = N + M - 1
  if (P != (N + M - 1)) {
    printf("Output signal vector is the wrong size\n");
    printf("It is %-d, but should be %-d\n", P, (N + M - 1));
    printf("Aborting convolution\n");
    return;
  }

  //  Clear the output buffer y[] to all zero values
  for (n = 0; n < P; n++)
    y[n] = 0.0;

  //  Do the convolution
  //  Outer loop:  process each input value x[n] in turn
  for (n = 0; n < N; n++) {
  //    Inner loop:  process x[n] with each sample of h[]
    for (m = 0; m < M; m++)
      y[n+m] += x[n] * h[m];
  }
}*/

int main(int argc, char* argv[])
{
	char* inSoundWav = argv[1];
	char*	IRwav = argv[2];
	char* outSoundWav = argv[3];

	struct waveData soundIn = loadWave(inSoundWav);
	struct waveData IRSound = loadWave(IRwav);

	if(soundIn.valid)
		printWave(soundIn);

	//convolve();

	saveWave(soundIn, outSoundWav);
	free(soundIn.data);
}
