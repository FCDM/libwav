// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBWAV_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBWAV_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

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
//#define __WASAPI_INCLUDED__

#include	<iostream>
#include	<fstream>
#include	<cstdio>
#include	<cstdint>
#include	<string>
#include	<cmath>
#include	<functional>
#include	<limits>

//WASAPI
#ifdef __WASAPI_INCLUDED__
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



#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef byte
#define byte BYTE
#endif


struct int24
{
	signed int data : 24;
};


struct WAVE_CHUNK
{
	char ckID[4];
	uint32_t ckSize;
};

struct WAVE_H
{
	char RIFFTag[4];
	uint32_t fileLength;

	char WAVETag[4];
	//format chunk
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

struct WAVE_H_PCM
{
	WAVE_H header;
	//figure out the data chunk first
	WAVE_CHUNK* chunks;
};

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
	Stereo(WAVE_H* h, memblock single) : Stereo(1, h->wBitsPerSample / 8, single, single){}
	Stereo(WAVE_H* h, memblock left, memblock right) : Stereo(2, h->wBitsPerSample / 8, left, right){}

	Stereo(int nChannels, int bytesPerSample, memblock left, memblock right)
	{
		this->nChannels = nChannels;
		this->bytesPerSample = bytesPerSample;
		this->left = left;
		this->right = right;
		if (nChannels == 2) this->right.p += bytesPerSample;	//stereo ptr fix
		this->nSamples = left.nBytes / (bytesPerSample * nChannels);
	}
	
	//return true if the fetch is valid
	//if failed, DO NOT USE GETTERS OR SETTERS
	bool next(){ return ++index < nSamples; }	//use do-while
	bool prev(){ return --index >= 0 && index < nSamples; }
	void reset(){ index = 0; }
	uint64_t getIndex() const { return index; }
	uint64_t getnSamples() const { return nSamples; }


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

protected:
	int32_t get(memblock& memory);
	int32_t set(memblock& memory, int32_t val);
	uint64_t index = 0;
	uint64_t nSamples;
	int bytesPerSample;
	int nChannels = 2;
	memblock left;
	memblock right;

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
		double freq;	//x-axis
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


///*
//ComplexDFT
//
//X(k) = foreach(n:0->N-1)x(n)cos(2pi*k*n/N) - foreach(n:0->N-1)x(n)sin(2pi*k*n/N) * i
//
//let x(n) = a+bi, let theta = 2pi*k*n/N
//X(k) = foreach(n:0->N-1)(a*cos(theta) + b*sin(theta)) - i * foreach(n:0->N-1)(a*sin(theta) - b*cos(theta))
//
//when b = 0, ComplexDFT = DFT
//
//*/
//class ComplexDFTransform
//{
//public:
//	typedef DFTransform::DFTResult ComplexDFTResult;
//	typedef DFTransform::DFTChannelResult ComplexDFTChannelResult;
//
//	ComplexDFTransform(double real[], double imag[], int nSamples)
//	{
//		this->real = real;
//		this->imag = imag;
//		this->nSamples = nSamples;
//	}
//
//	bool hasNext() const
//	{
//		return k < nSamples;
//	}
//
//	ComplexDFTChannelResult* next()
//	{
//		if (!hasNext()) return nullptr;
//		memset(&result, 0, sizeof(result));
//		double theta;
//
//		for (int n = 0; n < nSamples; n++)
//		{
//			double a = real[n];
//			double b = imag[n];
//			theta = (2 * M_PI * k * n / nSamples);
//
//			result.real += a*cos(theta) + b*sin(theta);
//			result.imag -= a*sin(theta) - b*cos(theta);
//		}
//
//		k++;
//		return &result;
//	}
//
//protected:
//	double* real;
//	double* imag;
//	int nSamples;
//
//	int k = 0;
//	ComplexDFTChannelResult result;
//};


//raw PCM data is a continuous sampling of sound amplitudes
class LIBWAV_API Wave 
{
public:
	Wave(std::string filename);

	Wave(byte raw[], int length);
	~Wave();

	WAVE_H* getH(){ return h; }
	
	byte* get_data_p(){ return ((byte*)data) + sizeof(WAVE_CHUNK); }
	int get_data_size() { return data->ckSize; }

	memblock* next();
	memblock* next(int nBlocks);

	DFTransform* DFT(memblock& m){ return new DFTransform(*getStereoObject(m), h->nSamplesPerSec); }
	FFTransform* FFT(memblock& m){ return new FFTransform(*getStereoObject(m), h->nSamplesPerSec); }

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

	//this function WILL change the original sound data
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


	/*
	Detect the BPM in the given block of memory

	incl. start, not incl. end
	very inefficient
	
	memblock: which data block you want to test
	for (int testbpm = startBPM; testbpm < endBPM; testbpm += stepBPM){};

	*/
	int detectBPM(memblock& m, int startBPM, int endBPM, int stepBPM);



private:
	void Wave_base_constructor(byte raw[]);	//unsafe
	WAVE_H* h;
	memblock mem;

	void DFTWindowTransform(memblock& memory, std::function<double(int)> transform);

	int getTi(int BPM){ return (int)((double)60 / BPM * h->nSamplesPerSec); }
	double BPMc(double* ta, double* tb, double* l, double* j, double* tl, double* tj, int nSamples, int maxAmp, int BPM);

protected:
	byte* raw = nullptr;
	WAVE_CHUNK* data;
	byte* p_data = nullptr;
};

/** Degrees to Radian **/
#define degreesToRadians( degrees ) ( ( degrees ) / 180.0 * M_PI )

/** Radians to Degrees **/
#define radiansToDegrees( radians ) ( ( radians ) * ( 180.0 / M_PI ) )

