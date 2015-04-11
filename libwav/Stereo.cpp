#include "stdafx.h"
#include "libwav.h"

using namespace std;

#pragma warning(disable: 4244)	//precision lost

int32_t Stereo::get(memblock& memory)
{
	switch (bytesPerSample)
	{
	case 1:
		return *(int8_t*)(memory.p + index*nChannels*bytesPerSample);
		break;
	case 2:
		return *(int16_t*)(memory.p + index*nChannels*bytesPerSample);
		break;
	case 3:
		return ((int24*)(memory.p + index*nChannels*bytesPerSample))->data;
		break;
	case 4:
		return *(int32_t*)(memory.p + index*nChannels*bytesPerSample);
		break;
	default:
		throw new std::exception("unsupported");
	}
}

int32_t Stereo::set(memblock& memory, int32_t val)
{
	switch (bytesPerSample)
	{
	case 1:
		val = (int8_t)val;
		*(int8_t*)(memory.p + index*nChannels*bytesPerSample) = val;
		break;
	case 2:
		val = (int16_t)val;
		*(int16_t*)(memory.p + index*nChannels*bytesPerSample) = val;
		break;
	case 3:
		val &= 0xffffff;
		((int24*)(memory.p + index*nChannels*bytesPerSample))->data = val;
		break;
	case 4:
		val = (int32_t)val;
		*(int32_t*)(memory.p + index*nChannels*bytesPerSample) = val;
		break;
	default:
		throw new std::exception("unsupported");
	}
	return val;
}

int32_t Stereo::maxAmp() const
{
	switch (bytesPerSample)
	{
	case 1:
		return INT8_MAX;
	case 2:
		return INT16_MAX;
	case 3:
		return 0xffffffff >> 1;
	case 4:
		return INT32_MAX;
	default:
		throw new std::exception("unsupported");
	}
}
