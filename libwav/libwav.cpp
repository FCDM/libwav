// libwav.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "libwav.h"

using namespace std;

Wave::Wave(string filename)
{
	FILE *f = fopen(filename.c_str(), "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	raw = new BYTE[fsize];
	fread(raw, 1, fsize, f);
	fclose(f);

	if (fsize > sizeof(WAVE_H)) Wave_base_constructor(raw);
	else throw new exception("length invalid");
}

Wave::~Wave()
{
	if (raw)
	{
		delete raw;
		raw = nullptr;
	}
}

Wave::Wave(byte raw[], int length)
{
	if (length > sizeof(WAVE_H)) Wave_base_constructor(raw);
	else throw new exception("length invalid");
}

void Wave::Wave_base_constructor(byte raw[])
{
	this->raw = raw;
	h = (WAVE_H*)raw;
	if (memcmp(h->RIFFTag, "RIFF", 4) != 0 || memcmp(h->fmt_Tag, "fmt ", 4) != 0 || memcmp(h->WAVETag, "WAVE", 4)) { throw new exception("header check failed"); }
	byte* p = (byte*)h;

	switch (h->wFormatTag)
	{
	case WAVE_FORMAT_PCM:
		do
		{
			if (memcmp(p, "data", 4) == 0) break;
			p++;
		} while (true);

		data = (WAVE_CHUNK*)p;
		break;
	case WAVE_FORMAT_EXTENSIBLE:
		if (memcmp(((WAVE_H_EXTENDED*)h)->SubFormat, WAVE_MEDIASUBTYPE_PCM, 16))
		{
			do
			{
				if (memcmp(p, "data", 4) == 0) break;
				p++;
			} while (true);
			data = (WAVE_CHUNK*)p;
			break;
		}
		//else go to default, throw
	default:
		throw new exception("wave format not accepted");
	}
}

memblock* Wave::next()
{
	return next(1);
}

memblock* Wave::next(int nBlocks)
{
	int nBytes = nBlocks * h->nBlockAlign;

	if (p_data == nullptr) p_data = get_data_p();

	mem.p = (uintptr_t)p_data;

	if (p_data + nBytes > get_data_p() + data->ckSize)
	{
		mem.nBytes = get_data_p() + data->ckSize - (p_data);
	}
	else
	{
		mem.nBytes = nBytes;
	}
	p_data += mem.nBytes;

	return &mem;
}

DFTransform::DFTransform(memblock memory, int nSamples, int nChannels, int nSamplesPerSecond, int bytesPerSecond)
{
	this->memory = memory;
	this->k = 0;
	this->nSamples = nSamples;
	this->nChannels = nChannels;
	this->nSamplesPerSecond = nSamplesPerSecond;
	this->bytesPerSecond = bytesPerSecond;
}

DFTransform::DFTResult* DFTransform::next()
{
	if (!hasNext()) return nullptr;
	memset(&result, 0, sizeof(DFTResult));
	result.k = k;
	double xn, theta;
	for (int n = 0; n < nSamples; n++)
	{
		switch (bytesPerSecond)
		{
		case 1:
			xn = *(int8_t*)(memory.p + n*nChannels*bytesPerSecond);
			break;
		case 2:
			xn = *(int16_t*)(memory.p + n*nChannels*bytesPerSecond);
			break;
		case 3:
			xn = ((int24*)(memory.p + n*nChannels*bytesPerSecond))->data;
			break;
		case 4:
			xn = *(int32_t*)(memory.p + n*nChannels*bytesPerSecond);
			break;
		default:
			throw new std::exception("unsupported");
		}
		theta = (2 * M_PI * k * n / nSamples);
		result.real += (xn * std::cos(theta));
		result.imag -= (xn * std::sin(theta));
	}
	result.freq = k * nSamplesPerSecond / nSamples;
	result.mag = pow(result.real * result.real + result.imag * result.imag, 0.5);
	result.angle = std::atan2(result.imag, result.real);
	result.dbmag = log10(result.mag) * 20;
	k++;

	return &result;
}

bool DFTransform::hasNext()
{
	return memory.p + nSamples*nChannels*bytesPerSecond >= memory.nBytes && k < nSamples;
}

