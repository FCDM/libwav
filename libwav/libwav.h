// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBWAV_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBWAV_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

// Copyright © 2007 Charles Chen

#ifdef LIBWAV_EXPORTS
#define LIBWAV_API __declspec(dllexport)
#else
#define LIBWAV_API __declspec(dllimport)
#ifdef _DEBUG	//vs
#pragma comment(lib, "../Debug/libwav.lib")
#else
#pragma comment(lib, "../Release/libwav.lib")
#endif
#endif

#pragma warning(disable: 4996)	//unsafe fopen
#define __WASAPI_INCLUDED__

#include	<iostream>
#include	<fstream>
#include	<cstdio>
#include	<cstdint>
#include	<string>
#include	<cmath>
#include	<functional>
#include	<limits>
#include	<deque>


//WASAPI
#ifdef __WASAPI_INCLUDED__
#include	<Mmdeviceapi.h>
#include	<Audioclient.h>
#include	<Audiopolicy.h>


#else	// !__WASAPI_INCLUDED__
static const uint16_t WAVE_FORMAT_PCM = 1;	//ACCEPT
static const uint16_t WAVE_FORMAT_IEEE_FLOAT = 3;	//DIE
static const uint16_t WAVE_FORMAT_ALAW = 6;	//DIE
static const uint16_t WAVE_FORMAT_MULAW = 7;	//DIE
static const uint16_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;	//ACCEPT


#endif


#include	<complex>
#include	<valarray>
#include	<vector>


/** Degrees to Radian **/
#define degreesToRadians( degrees ) ( ( degrees ) / 180.0 * M_PI )

/** Radians to Degrees **/
#define radiansToDegrees( radians ) ( ( radians ) * ( 180.0 / M_PI ) )


#define FREEIF_NONNULL(ptr) {\
	if (ptr != nullptr) free(ptr);\
	ptr = nullptr;\
								}

//WASAPI
#define RELEASEIF_NONNULL(ptr) {\
	if (ptr != nullptr) ptr->Release();\
	ptr = nullptr;\
				}

#define DELETEIF_NONNULL(ptr) {\
	if (ptr != nullptr) delete ptr;\
	ptr = nullptr;\
		}



#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef byte
#define byte BYTE
#endif


/*
24-bit integer type
*/
struct int24
{
	signed int data : 24;
};

/*
Chunks after WAVE_H
*/
struct WAVE_CHUNK
{
	char ckID[4];
	uint32_t ckSize;
};

/*
The leading structure of a standard WAVE file, discribes general details of the sound
*/
struct WAVE_H
{
	char RIFFTag[4];
	uint32_t fileLength;

	char WAVETag[4];

	//format chunk, ALWAYS PRESENT
	char fmt_Tag[4];
	uint32_t fmtSize;

	//common fields
	//note that bps=bytes per sample
	uint16_t wFormatTag;
	uint16_t nChannels;	//nCH
	uint16_t nSamplesPerSec;  //fps
	uint32_t nAvgBytesPerSec;	//fps*bps*nCH
	uint16_t nBlockAlign;   //bps*nCH				//process this much for each sample
	uint16_t wBitsPerSample;	//bps = wBitsPerSample/8

};

/*
Standard PCM-Wave
*/
struct WAVE_H_PCM
{
	WAVE_H header;
	//figure out the data chunk first
	WAVE_CHUNK* chunks;
};

//uuid
static const unsigned char WAVE_MEDIASUBTYPE_PCM[16]
{
	0x00, 0x00, 0x00, 0x01,
		0x00, 0x00,
		0x00, 0x10,
		0x80,
		0x00,
		0x00,
		0xAA,
		0x00,
		0x38,
		0x9B,
		0x71
};

/*
WAVE_FORMAT_EXTENSIBLE type Wave file's extra details of the sound
*/
struct WAVE_H_EXTENDED
{
	WAVE_H header;
	uint16_t cbSizeExtension;	//22
	uint16_t wValidBitsPerSample;	//at most = 8*bps
	uint32_t dwChannelMask;	//speaker mask

	// 00000001-0000-0010-8000-00AA00389B71            MEDIASUBTYPE_PCM 0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71)
	char SubFormat[16];

	
	WAVE_CHUNK* chunks;
};

/*
an area of memory with a pointer and a length
*/
struct memblock
{
	uintptr_t p;
	unsigned int nBytes;
};

/*
Stereo Support
always using int32_t for data transferring

getters: int32_t, extra-high-bits are zeros
setters: int32_t, extra-high-bits are ignored

please go through function comments first
*/
class LIBWAV_API Stereo
{
public:
	Stereo(WAVE_H* h, memblock single) : Stereo(1, h->nSamplesPerSec, h->wBitsPerSample / 8, single, single){}
	Stereo(WAVE_H* h, memblock left, memblock right) : Stereo(2, h->nSamplesPerSec, h->wBitsPerSample / 8, left, right){}

	Stereo(int nChannels, int nSamplesPerSec, int bytesPerSample, memblock left, memblock right)
	{
		this->nChannels = nChannels;
		this->bytesPerSample = bytesPerSample;
		this->left = left;
		this->right = right;
		if (nChannels == 2) this->right.p += bytesPerSample;	//stereo ptr fix
		this->nSamples = left.nBytes / (bytesPerSample * nChannels);
		this->nSamplesPerSec = nSamplesPerSec;
	}
	
	//return true if the fetch is valid
	//if failed, DO NOT USE GETTERS OR SETTERS
	bool next(){ return ++index < nSamples; }	//use do-while
	bool prev(){ return --index >= 0 && index < nSamples; }
	void reset(){ index = 0; }
	uint64_t getIndex() const { return index; }
	uint64_t getnSamples() const { return nSamples; }
	int getnSamplesPerSec() const { return nSamplesPerSec; }

	//getters
	int32_t getLeft(){ return get(left); }
	int32_t getRight(){ return get(right); }
	int32_t getAvg(){ return (getLeft() + getRight()) / 2; }

	//setters: val will be trunc. to fill the size, returns the actual value put in
	int32_t setLeft(int32_t val){ return set(left, val); }
	int32_t setRight(int32_t val){ return set(right, val); }
	int32_t setAvg(int32_t val){ return (setLeft(val) + setRight(val)) / 2; }

	//max value for signal
	int32_t maxAmp() const;

	/*
	Scales the given sample value into a percentage of the volume
	*/
	double scale(int32_t data) const
	{
		return (double)data / maxAmp();
	}

protected:
	int32_t get(memblock& memory);
	int32_t set(memblock& memory, int32_t val);

protected:
	uint64_t index = 0;
	uint64_t nSamples;
	int bytesPerSample;
	int nChannels = 2;
	memblock left;
	memblock right;
	int nSamplesPerSec;
};


/*
DFT

X(k) = foreach(n:0->N-1)x(n)cos(2pi*k*n/N) - foreach(n:0->N-1)x(n)sin(2pi*k*n/N) * i

//in
k: index of sample
x(n): amplitude
N: nSamples

//out
X(k): coefficient
P(k): power
F(k): frequency


Result:
F(k) = k * nSamplesPerSecond(fs) / nSamples(N)
P(k) = Real(X(k))**2 + Imag(X(k))**2

*/
class LIBWAV_API DFTransform
{
public:
	struct DFTChannelResult
	{
		//index
		int k;

		//DFT: X(k)
		double real;
		double imag;	//negative already	DFTResult->imag * (i)

		//Analysis
		double freq;	//x-axis, this jumps in a constant value
		double mag;		//y-axis
		double angle;	//phase in rad

		double dbmag;	//spec mag in db
	};

	struct DFTResult
	{
		DFTChannelResult left;
		DFTChannelResult right;
		DFTChannelResult stereo;
	};

	DFTransform(Stereo& stereo, int nSamplesPerSecond)
	{
		this->stereo = &stereo;
		this->nSamplesPerSecond = nSamplesPerSecond;
	}
	~DFTransform(){ delete stereo; };

	/*
	iteration
	if hasNext(), call next()
	DFTChannelResult.k ++
	*/
	virtual bool hasNext() const;
	enum nextResult{ LEFT, RIGHT, STEREO, ALL };
	virtual DFTResult* next(){ return next(ALL); };
	virtual DFTResult* next(nextResult type);


	uint64_t getnSamples() const { return stereo->getnSamples(); }

protected:
	Stereo* stereo;
	int nSamplesPerSecond;

	int k = 0;
	DFTResult result;

	virtual void DFT(DFTChannelResult& r, std::function<double(void)> stereo_fetch);


};


/*
FFT - Real
See DFTransform
Note: Cannot change the next() type once started iteration
Note: Cannot specify ALL for next(), only LEFT, RIGHT, STEREO
*/
class LIBWAV_API FFTransform : public DFTransform
{
public:
	FFTransform(Stereo& stereo, int nSamplesPerSecond) : DFTransform(stereo, nSamplesPerSecond)
	{
		nSamples = (int)stereo.getnSamples();
	}

	~FFTransform()
	{

	}


	//Core FFTransform API, static access
	static void ComplexFFT(std::valarray<std::complex<double>>& x);
	static void ComplexFFT(double real[], double imag[], int nSamples, double outputr[], double outputi[]);


	/*
	See DFTransform
	*/
	virtual bool hasNext() const
	{
		if (real == nullptr) return true;
		return k < nSamples;
	}

	enum nextResult{ LEFT, RIGHT, STEREO };
	virtual DFTResult* next(){ return next(STEREO); };
	virtual DFTResult* next(nextResult type);	//only taking the first input


protected:
	nextResult type;
	double* real = nullptr;
	double* imag = nullptr;

	double* outputr = nullptr;
	double* outputi = nullptr;

	int nSamples;

	void FFTinit(nextResult type);
	void fillResult(DFTChannelResult r);

};


//raw PCM data is a continuous sampling of sound amplitudes
class LIBWAV_API Wave 
{
public:
	/* Generates an invalid Wave Object */
	Wave(){}

	/* File IO load the specified Wave File, invalid Wave file or length will throw an exception */
	Wave(std::string filename);

	/* Length is provided only for securing possible access violation, construct a wav file by a pointer to its memory address, INPLACE */
	Wave(byte raw[], int length);
	~Wave();

	/* Return the leading structure of Wave */
	WAVE_H* getH(){ return h; }
	
	/* Get the pointer at the first byte for actual data */
	uintptr_t get_data_p(){ return (uintptr_t)((byte*)data) + sizeof(WAVE_CHUNK); }
	int get_data_size() { return data->ckSize; }

	/* Check if Wave already finished its iteration */
	bool hasNext()
	{
		return hasNext(1);
	}

	/* Check if nBlocks is still available */
	bool hasNext(int nBlocks)
	{
		if (p_data == nullptr) return true;
		if (mem.p == 0) return true;
		int nBytes = nBlocks * h->nBlockAlign;
		if (mem.p + mem.nBytes + nBytes >= get_data_p() + get_data_size()) return false;
		return true;
	}

	/* Fetch the next chunk of blocks of samples from the wave source, INPLACE access */
	memblock* next();
	memblock* next(int nBlocks);
	memblock* getLastNextResult() {return &mem;}	//get the last next() result, the current block
	
	/* Construct a DFTransform/FFTransform object, YOU ARE RESPONSIBLE FOR DELETING IT: delete p; */
	DFTransform* DFT(memblock& m){ return new DFTransform(*getStereoObject(m), h->nSamplesPerSec); }
	FFTransform* FFT(memblock& m){ return new FFTransform(*getStereoObject(m), h->nSamplesPerSec); }

	/* Construct a Stereo object to wrap around memblocks, Wave-dependent */
	Stereo* getStereoObject(memblock& m)
	{
		switch (h->nChannels)
		{
		case 1:
			return new Stereo(h, m);
		case 2:
			return new Stereo(h, m, m);
		default:
			throw std::exception();
		}
	}

	//Triangle = Bartlett
	enum DFTWindowType
	{
		Rectangular, Triangle, Hamming, Hanning, Blackman, BlackmanHarris
	};

	//INPLACE Windowing, manipulations to the input samples
	memblock& DFTWindow(memblock& m, DFTWindowType type)
	{
		double N = m.nBytes / (h->wBitsPerSample / 8 * h->nChannels);
		switch (type)
		{
		case Rectangular:
			break;
		case Triangle:
			DFTWindowTransform(m, [&](int index){return 1.0f - abs((index - ((N - 1.0f) / 2.0f)) / ((N - 1.0f) / 2.0f)); });
			break;
		case Hamming:
			DFTWindowTransform(m, [&](int index){return 0.54f - (0.46f * cos(index / N)); });
			break;
		case Hanning:
			DFTWindowTransform(m, [&](int index){return 0.5f * (1.0f - cos(index / N)); });
			break;
		case Blackman:
			DFTWindowTransform(m, [&](int index){return 0.42f - (0.5f * cos(index / N)) + (0.08 * cos(2.0 * index / N)); });
			break;
		case BlackmanHarris:
			DFTWindowTransform(m, [&](int index){return 0.35875f - (0.48829f * cos(1.0 * index / N)) + (0.14128f * cos(2.0 * index / N)) - (0.01168f * cos(3.0 * index / N)); });
			break;
		default:
			;
		}
		return m;
	}

	/* Fetch nSamplesPerSec from header */
	int nSamplesPerSec() { return h->nSamplesPerSec; }


	/*
	Detect the BPM in the given block of memory

	incl. start, not incl. end
	very inefficient
	
	memblock: which data block you want to test
	for (int testbpm = startBPM; testbpm < endBPM; testbpm += stepBPM){};

	*/
	int detectBPM(memblock& m, int startBPM, int endBPM, int stepBPM);

	/* Build a WAVEFORMATEX structure, used by WASAPI */
	WAVEFORMATEX getWaveFormatEx()
	{
		WAVEFORMATEX wave;
		memcpy(&wave, &h->wFormatTag, sizeof(wave));
		wave.cbSize = 0;
		return wave;
	}

	/* Build a WAVEFORMATEXTENSIBLE structure, used by WASAPI */
	WAVEFORMATEXTENSIBLE getWaveFormatExtensible()
	{
		WAVEFORMATEX w = getWaveFormatEx();
		w.cbSize = 22;
		WAVEFORMATEXTENSIBLE ext;
		memcpy(&ext, &w, sizeof(WAVEFORMATEX));
		ext.Samples.wValidBitsPerSample = ((WAVE_H_EXTENDED*)h)->wValidBitsPerSample;
		//ext.Samples.wSamplesPerBlock = 0;
		//ext.Samples.wReserved = 0;
		ext.dwChannelMask = ((WAVE_H_EXTENDED*)h)->dwChannelMask;
		memcpy(&(ext.SubFormat), &(((WAVE_H_EXTENDED*)h)->SubFormat[0]), 16);
		return ext;
	}
	
	/* Check if this Wave is an extended wave*/
	bool isExtendedWave()
	{
		return h->wFormatTag == WAVE_FORMAT_EXTENSIBLE;
	}

	/* Reset the iteration status, re-start iteration, PLEASE BE CAREFUL OF INPLACE ACCESSES*/
	void reset()
	{
		p_data = nullptr;
	}

	/* Determine the alg. correlations between two memblocks(internally uses as Stereo) */
	double correlation(memblock& mem1, memblock& mem2)
	{
		double result = 0;
		Stereo* stereo1 = getStereoObject(mem1);
		Stereo* stereo2 = getStereoObject(mem2);
		uint64_t length = stereo1->getnSamples() > stereo2->getnSamples() ? stereo2->getnSamples() : stereo1->getnSamples();
		do
		{
			result += stereo1->getAvg() * stereo2->getAvg();
			stereo1->next(); stereo2->next();
		} while (length-- > 0);
		return result;
	}

private:
	WAVE_H* h;
	memblock mem;

protected:	
	/* Parse Actual Wave Header */
	virtual void base_constructor(byte raw[], int length);	//unsafe
	
	/* Perform Windowing Transform according to std::function */
	void DFTWindowTransform(memblock& memory, std::function<double(int)> transform);

	/* detectBPM internals */
	int getTi(int BPM){ return (int)((double)60 / BPM * h->nSamplesPerSec); }
	double BPMc(double* ta, double* tb, double* l, double* j, double* tl, double* tj, int nSamples, int maxAmp, int BPM);

protected:
	/* raw pointer of the wave in the memory */
	byte* raw = nullptr;

	/* data chunk pointer */
	WAVE_CHUNK* data;
	
	/* current iteration pointer */
	byte* p_data = nullptr;
};

/*
StatBeatDetection
Statistical Beat Detection Implementation
Algorithm By Frédéric Patin @ http://archive.gamedev.net/archive/reference/programming/features/beatdetection/index.html

*/
class LIBWAV_API StatBeatDetection
{
public:
	/*
	Constructor
	hnsPrecision: how large is a block of samples to be considered, in time
	hnsBufferDuration: how many blocks in history should be considered for detection 
	*/
	StatBeatDetection(Wave& wave, REFERENCE_TIME hnsPrecision = 750000, REFERENCE_TIME hnsBufferDuration = 10000000)
	{
		this->wave = &wave;
		double nSamplesBuffer = (hnsBufferDuration*pow(10, -7)*wave.nSamplesPerSec());
		double nSamplesPrecision = (hnsPrecision*pow(10, -7)*wave.nSamplesPerSec());
		buffer = new std::deque<double>((int)(nSamplesBuffer / nSamplesPrecision), 1);
		this->nSamplesPrecision = (int)nSamplesPrecision;
	}

	~StatBeatDetection(){ release(); }
	void release(){ DELETEIF_NONNULL(buffer); }

	/* This value is determined by the time the object is constructed */
	virtual int length() const { return wave->get_data_size() / ((wave->getH()->wBitsPerSample / 8)*wave->getH()->nChannels) / nSamplesPrecision; }

	/* Iteration */
	virtual bool hasNext() const { return wave->hasNext(nSamplesPrecision); }
	virtual double next();

	/* Get nSamples in a block of consideration */
	int getPrecision() const { return nSamplesPrecision; }

protected:
	std::deque<double>* buffer = nullptr;
	Wave* wave = nullptr;
	int nSamplesPrecision;

};


/* Windows Audio Session */
namespace WASAPI
{

	class LIBWAV_API Audio
	{
	public:
		Audio(Wave& audioContent, REFERENCE_TIME hnsBufferDuration);	//100 nano seconds

		~Audio()
		{
			Release();
		}

	public:
		/*
		Audio Playback Control
		*/

		/*
		audioContent->next(audio.framesAvailable());
		nBlocks of data the stream could/should load now, shall match the call to audioContent->next(int)
		*/
		int framesAvailable();

		/*
		fill the stream buffer
		this function collects data right from audioContent, it looks at the last call to audioContent->next(int)
		*/
		bool fillBuffer(){ return fillBuffer(audioContent->getLastNextResult()); }

		/*
		fill the stream buffer
		this function uses the given memblock with the audioContent's format information
		*/
		bool fillBuffer(memblock* mem);

	public:
		/*
		Volume Control
		*/

		/*
		get total channel(s) present in this session
		*/
		uint32_t getChannelCount(){ uint32_t t; pAudioStreamVolume->GetChannelCount(&t); return t; }

		/*
		get the percentage volume of the selected channel[0, getChannelCount()), from 0.0 to 1.0
		*/
		float getChannelVolume(uint32_t indxChannel)
		{
			float t;
			pAudioStreamVolume->GetChannelVolume(indxChannel, &t);
			return t;
		}

		/*
		set the percentage volume of the selected channel[0, getChannelCount()), from 0.0 to 1.0
		*/
		void setChannelVolume(uint32_t indxChannel, const float volume)
		{
			pAudioStreamVolume->SetChannelVolume(indxChannel, volume);
		}

	protected:
		void selectDefaultAudioDevice(Wave* audioContent);	//(re) init. WAS interfaces

		void Release();	//Release Allocated Resources

	protected:
		HRESULT hr = S_OK;
		Wave* audioContent;
		REFERENCE_TIME hnsBufferDuration;
		uint32_t bufferFrameCount;

	protected:
		IMMDeviceEnumerator* pEnumerator;
		IMMDevice* pDevice;
		IAudioClient* pAudioClient;

	protected:
		IAudioClock* pAudioClock;
		IAudioRenderClient* pAudioRenderClient;
		IAudioStreamVolume* pAudioStreamVolume;
	};
}


