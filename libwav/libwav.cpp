// libwav.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "libwav.h"

using namespace std;

#pragma warning(disable: 4244)	//precision lost


Wave::Wave(string filename)
{
	FILE *f = fopen(filename.c_str(), "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	raw = new BYTE[fsize];
	fread(raw, 1, fsize, f);
	fclose(f);

	if (fsize > sizeof(WAVE_H)) base_constructor(raw, fsize);
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
	if (length > sizeof(WAVE_H)) base_constructor(raw, length);
	else throw new exception("length invalid");
}

void Wave::base_constructor(byte raw[], int length)
{
	memset(&(this->mem), 0, sizeof(this->mem));
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

	if (p_data == nullptr) p_data = (byte*)get_data_p();

	mem.p = (uintptr_t)p_data;

	if (p_data + nBytes > (byte*)get_data_p() + data->ckSize)
	{
		mem.nBytes = (byte*)get_data_p() + data->ckSize - (p_data);
	}
	else
	{
		mem.nBytes = nBytes;
	}
	p_data += mem.nBytes;

	return &mem;
}

void Wave::DFTWindowTransform(memblock& memory, std::function<double(int)> transform)
{
	int nChannels = h->nChannels;
	int nSamples = memory.nBytes / (h->wBitsPerSample / 8 * h->nChannels);
	int bytesPerSecond = h->wBitsPerSample / 8;

	for (int index = 0; index < nSamples; index++)
		switch (bytesPerSecond)
	{
		case 1:
			*(int8_t*)(memory.p + index*nChannels*bytesPerSecond) *= transform(index);
			break;
		case 2:
			*(int16_t*)(memory.p + index*nChannels*bytesPerSecond) *= transform(index);
			break;
		case 3:
			((int24*)(memory.p + index*nChannels*bytesPerSecond))->data *= transform(index);
			break;
		case 4:
			*(int32_t*)(memory.p + index*nChannels*bytesPerSecond) *= transform(index);
			break;
		default:
			throw new std::exception("unsupported");
	}

	return;
}


double Wave::BPMc(double* ta, double* tb, double* l, double* j, double* tl, double* tj, int nSamples, int maxAmp, int BPM)
{
	int Ti = getTi(BPM);

	for (int k = 0; k < nSamples; k++)
	{
		if ((k % Ti) == 0)
			l[k] = j[k] = maxAmp;
		else
			l[k] = j[k] = 0;
	}

	FFTransform::ComplexFFT(l, j, nSamples, tl, tj);

	double e = 0;
	for (int k = 0; k < nSamples; k++)
	{
		double real = ta[k] * tl[k] - tb[k] * tj[k];
		double imag = ta[k] * tj[k] + tb[k] * tl[k];
		e += pow(real*real + imag*imag, 0.5);
	}

	return e;
}

int Wave::detectBPM(memblock& m, int startBPM, int endBPM, int stepBPM)
{
	Stereo& stereo = *getStereoObject(m);

	int nSamples = (int)stereo.getnSamples();
	double* a = (double*)malloc(nSamples*sizeof(double));
	double* b = (double*)malloc(nSamples*sizeof(double));

	int i = 0;
	do
	{
		a[i] = stereo.getLeft();
		b[i] = stereo.getRight();
		i++;
	} while (stereo.next());


	//derivation filter
	//for (int k = 1; k <= nSamples - 2; k++)
	//{
	//	a[k] = 0.5*(h->nSamplesPerSec)*(a[k + 1] - a[k - 1]);
	//	b[k] = 0.5*(h->nSamplesPerSec)*(b[k + 1] - b[k - 1]);
	//}

	double* ta = (double*)malloc(nSamples*sizeof(double));
	double* tb = (double*)malloc(nSamples*sizeof(double));
	double* l = (double*)malloc(nSamples*sizeof(double));
	double* j = (double*)malloc(nSamples*sizeof(double));
	double* tl = (double*)malloc(nSamples*sizeof(double));
	double* tj = (double*)malloc(nSamples*sizeof(double));

	FFTransform::ComplexFFT(a, b, nSamples, ta, tb);

	int BPMmax = -1;
	double maxT = -DBL_MAX;

	for (int bpm = startBPM; bpm < endBPM; bpm += stepBPM)
	{
		double t = BPMc(ta, tb, l, j, tl, tj, nSamples, stereo.maxAmp(), bpm);
		if (t > maxT)
		{
			maxT = t;
			BPMmax = bpm;
		}
	}

	free(a);
	free(b);
	free(ta);
	free(tb);
	free(l);
	free(j);
	free(tl);
	free(tj);
	delete &stereo;


	return BPMmax;
}

