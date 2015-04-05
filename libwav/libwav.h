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
#pragma warning(disable: 4996)

#include	<iostream>
#include	<fstream>
#include	<cstdio>
#include	<cstdint>
#include	<string>
#include	<cmath>

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#ifndef byte
#define byte BYTE
#endif

static const uint16_t WAVE_FORMAT_PCM = 1;	//ACCEPT
static const uint16_t WAVE_FORMAT_IEEE_FLOAT = 3;	//DIE
static const uint16_t WAVE_FORMAT_ALAW = 6;	//DIE
static const uint16_t WAVE_FORMAT_MULAW = 7;	//DIE
static const uint16_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;	//ACCEPT

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

static const char WAVE_MEDIASUBTYPE_PCM[16]
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
	int nBytes;
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
	struct DFTResult
	{
		//index
		int k;

		//DFT: X(k)
		double real;
		double imag;	//negative already	DFTResult->imag * (i)

		//Analysis
		double freq;	//x-axis
		double mag;		//y-axis
		double dbmag;	//spec mag in db
	};

	DFTransform(memblock* memory, int nSamples, int nChannels, int nSamplesPerSecond, int bytesPerSecond);

	~DFTransform(){};

	bool hasNext();
	DFTResult* next();

private:
	memblock* memory;
	int k;
	int nSamples;
	int nChannels;
	int nSamplesPerSecond;
	int bytesPerSecond;

	DFTResult result;

};


//raw PCM data is a continuous sampling of sound amplitudes
class LIBWAV_API Wave 
{
public:
	Wave(std::string filename);

	Wave(byte raw[], int length);
	~Wave();

	WAVE_H* getH(){ return h; }
	
	byte* get_data_p()
	{
		return ((byte*)data) + sizeof(WAVE_CHUNK);
	}


	struct DFTResult
	{
		int cur;
		int real;
		int imag;
	};

	memblock* next();
	memblock* next(int nBlocks);
	memblock* next(int nBlocks, DFTransform* dft)
	{
		memblock& m = *next(nBlocks);
		dft = DFT(m);
		return &m;
	}

	DFTransform* DFT()
	{
		memblock m;
		memset(&m, 0, sizeof(memblock));
		m.p = (uintptr_t)get_data_p();
		m.nBytes = data->ckSize;
		return DFT(m);
	}

	DFTransform* DFT(memblock& m)
	{
		return new DFTransform(&m, m.nBytes / (h->wBitsPerSample / 8 * h->nChannels), h->nChannels, h->nSamplesPerSec, h->wBitsPerSample / 8);
	}

private:
	void Wave_base_constructor(byte raw[]);	//unsafe
	WAVE_H* h;
	memblock mem;

protected:
	byte* raw = nullptr;
	WAVE_CHUNK* data;
	byte* p_data = nullptr;
};
