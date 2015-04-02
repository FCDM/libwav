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
#pragma comment(lib, "../Debug/libwav.lib")
#endif

#pragma warning(disable: 4996)
#include	<iostream>
#include	<fstream>
#include	<cstdio>
#include	<cstdint>
#include	<string>
#include	<typeinfo>

#ifndef byte
#define byte char
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
	//data
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
	WAVE_CHUNK data;	//bps*fps*nBlocks [+padding to become even]
};

struct WAVE_H_EXTENDED
{
	WAVE_H header;
	uint16_t cbSizeExtension;	//22
	uint16_t wValidBitsPerSample;	//at most = 8*bps
	uint32_t dwChannelMask;	//speaker mask
	char SubFormat[16];	//GUID	//furthur parsing required
	
	WAVE_CHUNK fact;	//nCH*nBlocks
	WAVE_CHUNK data;	//bps*fps*nBlocks [+padding to become even]
};

class LIBWAV_API Wave 
{
public:
	Wave(std::string filename);

	Wave(byte raw[], int length);

	

	WAVE_H* h;
private:
	void Wave_base_constructor(byte raw[]);	//unsafe
	

protected:
	byte* raw;
	WAVE_CHUNK* data;

};
