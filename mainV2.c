#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
#define SIZE       8
#define PI         3.141592653589793
#define TWO_PI     (2.0 * PI)
#define SWAP(a,b)  tempr=(a);(a)=(b);(b)=tempr

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

	double* data;

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
		wave.data = (double*) malloc(sizeof(double) * numSamples);


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
//is the length divisible by 2? Nope, add a zero. Yup? then we wont go down this road
void add1Zero(double data[], int size)
{
	double* tmp = realloc(data, sizeof(double) * (size+1));
	if (tmp == NULL){
		printf("adding zero FAIL!!!");
	}else{
		data = tmp;
		data[size] = 0.0;
	}
}

// if the data arrays are not equal zero pad one of them
void zeroPad(double data1[], int size1, double data2[], int size2)
{
	if (size1 > size2){
		data2 = realloc(data2, sizeof(double) * size1);
		if(size1 % 2 != 0){
			add1Zero(data1, size1);
			add1Zero(data2, size1);
		}
	}else if(size2 > size1){
		data1 = realloc(data1, sizeof(double) * size2);
		if(size2 % 2 != 0){
			add1Zero(data1, size2);
			add1Zero(data2, size2);
		}
	}else{
		if(size1 %2 != 0){
			add1Zero(data1, size1);
			add1Zero(data2, size1);
		}
	}
}

//  Shiny New Convolution stuff
//  The four1 FFT from Numerical Recipes in C,
//  p. 507 - 508.
//  Note:  changed float data types to double.
//  nn must be a power of 2, and use +1 for
//  isign for an FFT, and -1 for the Inverse FFT.
//  The data is complex, so the array size must be
//  nn*2. This code assumes the array starts
//  at index 1, not 0, so subtract 1 when
//  calling the routine (see main() below).

void four1(double data[], int nn, int isign)
{
    unsigned long n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;

    n = nn << 1;
    j = 1;

    for (i = 1; i < n; i += 2) {
	if (j > i) {
	    SWAP(data[j], data[i]);
	    SWAP(data[j+1], data[i+1]);
	}
	m = nn;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }

    mmax = 2;
    while (n > mmax) {
	istep = mmax << 1;
	theta = isign * (6.28318530717959 / mmax);
	wtemp = sin(0.5 * theta);
	wpr = -2.0 * wtemp * wtemp;
	wpi = sin(theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 1; m < mmax; m += 2) {
	    for (i = m; i <= n; i += istep) {
		j = i + mmax;
		tempr = wr * data[j] - wi * data[j+1];
		tempi = wr * data[j+1] + wi * data[j];
		data[j] = data[i] - tempr;
		data[j+1] = data[i+1] - tempi;
		data[i] += tempr;
		data[i+1] += tempi;
	    }
	    wr = (wtemp = wr) * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
	}
	mmax = istep;
    }
}

int main(int argc, char* argv[])
{
	char* inSoundWav = argv[1];
	char*	IRwav = argv[2];
	char* outSoundWav = argv[3];

	struct waveData IRSound = loadWave(IRwav);
	struct waveData soundIn = loadWave(inSoundWav);

	int INbytesPerSample = soundIn.bitsPerSample / 8;
	int INSize = soundIn.subChunk2Size / INbytesPerSample;

	int IRbytesPerSample = IRSound.bitsPerSample / 8;
	int IRSize = IRSound.subChunk2Size / IRbytesPerSample;
/*
	soundOut.subChunk2Size = 2 * (INNumSample + IRNumSample - 1);
	soundOut.chunkSize = 36 + soundOut.subChunk2Size;

	int OUTBytesPerSample = soundOut.bitsPerSample / 8;
	int OUTNumSample = soundOut.subChunk2Size / OUTBytesPerSample;
*/
	zeroPad(soundIn.data, INSize, IRSound.data, IRSize);


	if(soundIn.valid)
		printWave(soundIn);
	if(IRSound.valid)
		printWave(IRSound);

	free(soundIn.data);
	free(IRSound.data);
}
