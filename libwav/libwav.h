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
#include	<typeinfo>

#ifndef byte
#define byte BYTE
#endif

static const uint16_t WAVE_FORMAT_PCM = 1;	//ACCEPT
static const uint16_t WAVE_FORMAT_IEEE_FLOAT = 3;	//DIE
static const uint16_t WAVE_FORMAT_ALAW = 6;	//DIE
static const uint16_t WAVE_FORMAT_MULAW = 7;	//DIE
static const uint16_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;	//ACCEPT



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

class LIBWAV_API Wave 
{
public:
	Wave(std::string filename);

	Wave(byte raw[], int length);

	WAVE_H* getH(){ return h; }
	
	byte* get_data_p()
	{
		return ((byte*)data) + sizeof(WAVE_CHUNK);
	}

	struct memblock
	{
		uintptr_t p;
		int nBytes;
	};

	memblock* next();
	memblock* next(int nBlocks);

private:
	void Wave_base_constructor(byte raw[]);	//unsafe
	WAVE_H* h;
	memblock mem;

protected:
	byte* raw;
	WAVE_CHUNK* data;
	byte* p_data = nullptr;
};

