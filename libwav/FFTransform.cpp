#include "stdafx.h"
#include "libwav.h"

using namespace std;

#pragma warning(disable: 4244)	//precision lost

void FFTransform::ComplexFFT(double real[], double imag[], int nSamples, double outputr[], double outputi[])
{
	std::vector<std::complex<double>> v;

	for (int i = 0; i < nSamples; i++)
	{
		v.push_back(*new std::complex<double>(real[i], imag[i]));
	}

	std::valarray<std::complex<double>> data(v.data(), v.size());

	ComplexFFT(data);

	for (int i = 0; i < nSamples; i++)
	{
		outputr[i] = data[i].real();
		outputi[i] = data[i].imag();
	}
}

void FFTransform::ComplexFFT(std::valarray<std::complex<double>>& x)
{
	int nSamples = x.size();
	if (nSamples <= 1) return;
	else
	{
		std::valarray<std::complex<double>> e = x[std::slice(0, nSamples / 2, 2)];
		std::valarray<std::complex<double>> o = x[std::slice(1, nSamples / 2, 2)];

		ComplexFFT(e);
		ComplexFFT(o);

		for (int i = 0; i < nSamples / 2; i++)
		{
			std::complex<double> tmp = (std::polar(1.0, -2 * M_PI*i / nSamples)) * (o[i]);
			x[i] = e[i] + tmp;
			x[i + nSamples / 2] = e[i] - tmp;
		}
	}
}


void FFTransform::FFTinit(nextResult type)
{
	real = new double[nSamples];
	imag = new double[nSamples];
	outputr = new double[nSamples];
	outputi = new double[nSamples];

	int i = 0;
	do
	{
		switch (type)
		{
		case LEFT:
			real[i] = stereo->getLeft();
			break;
		case RIGHT:
			real[i] = stereo->getRight();
			break;
		case STEREO:
			real[i] = stereo->getAvg();
			break;
		default:
			throw new std::exception();
		}
		i++;
	} while (stereo->next());

	memset(imag, 0, nSamples);
}

void FFTransform::fillResult(DFTChannelResult r)
{
	r.real = outputr[k];
	r.imag = outputi[k];

	r.freq = k * nSamplesPerSecond / nSamples;
	r.mag = pow(r.real * r.real + r.imag * r.imag, 0.5);
	r.angle = std::atan2(r.imag, r.real);
	r.dbmag = log10(r.mag) * 20;
}

FFTransform::DFTResult* FFTransform::next(FFTransform::nextResult type)	//only taking the first input
{
	memset(&result, 0, sizeof(result));
	if (real == nullptr)
	{
		FFTinit(type);
		ComplexFFT(real, imag, nSamples, outputr, outputi);
		this->type = type;
	}

	k++;
	switch (this->type)
	{
	case LEFT:
		fillResult(result.left);
		return &result;
	case RIGHT:
		fillResult(result.right);
		return &result;
	case STEREO:
		fillResult(result.stereo);
		return &result;
	default:
		throw new std::exception();
	}
}

