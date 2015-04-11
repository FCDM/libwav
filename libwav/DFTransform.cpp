#include "stdafx.h"
#include "libwav.h"

using namespace std;

#pragma warning(disable: 4244)	//precision lost

DFTransform::DFTResult* DFTransform::next(DFTransform::nextResult type)
{
	if (!hasNext()) return nullptr;
	memset(&result, 0, sizeof(result));
	int nSamples = stereo->getnSamples();

	if (type == ALL || type == LEFT) DFT(result.left, [&](){return stereo->getLeft(); });
	if (type == ALL || type == RIGHT) DFT(result.right, [&](){return stereo->getRight(); });
	if (type == ALL || type == STEREO) DFT(result.stereo, [&](){return stereo->getAvg(); });

	k++;
	return &result;
}

bool DFTransform::hasNext() const
{
	return k < stereo->getnSamples();
	//	return memory.p + nSamples*nChannels*bytesPerSecond >= memory.nBytes && k < nSamples;
}

void DFTransform::DFT(DFTransform::DFTChannelResult& r, std::function<double(void)> stereo_fetch)
{
	double xn, theta;
	int nSamples = stereo->getnSamples();
	r.k = k;

	stereo->reset();
	do
	{
		xn = stereo_fetch();
		int n = stereo->getIndex();

		theta = (2 * M_PI * k * n / nSamples);
		r.real += (xn * std::cos(theta));
		r.imag -= (xn * std::sin(theta));
	} while (stereo->next());
	//r.real /= nSamples; r.real *= 2;
	//r.imag /= nSamples; r.imag *= 2;

	r.freq = k * nSamplesPerSecond / nSamples;
	r.mag = pow(r.real * r.real + r.imag * r.imag, 0.5);
	r.angle = std::atan2(r.imag, r.real);
	r.dbmag = log10(r.mag) * 20;
}
