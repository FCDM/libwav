// libwav.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "libwav.h"

using namespace std;

Wave::Wave(string filename)
{
	ifstream infile(filename, ios::ate | ios::binary);
	if (!infile.is_open() || infile.fail())
	{
		throw new exception("cannot load the specified file");
	}
	int fsize = (int)infile.tellg();
	infile.clear();
	infile.seekg(0);
	raw = (char*)malloc(fsize);
	infile.read(raw, fsize);
	infile.close();

	if (fsize > sizeof(WAVE_H)) Wave_base_constructor(raw);
	else throw new exception("length invalid");
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
	switch (h->wFormatTag)
	{
	case WAVE_FORMAT_PCM:
		data = &((WAVE_H_PCM*)h)->data;
		break;
	case WAVE_FORMAT_EXTENSIBLE:
		data = &((WAVE_H_EXTENDED*)h)->data;
		break;
	default:
		throw new exception("wave format not accepted");
	}
}

