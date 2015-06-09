#include "stdafx.h"
#include "libwav.h"

using namespace std;

#pragma warning(disable: 4244)	//precision lost

double StatBeatDetection::next()
{
	if (!hasNext()) throw std::exception();
	Stereo* stereo = wave->getStereoObject(*(wave->next(nSamplesPrecision)));

	double insEnergy = 0;
	do
	{
		insEnergy += pow(stereo->scale(stereo->getLeft()), 2) + pow(stereo->scale(stereo->getRight()), 2);
	} while (stereo->next());

	double histAvg = 0;
	for (double i : *buffer)
	{
		histAvg += pow(i, 1);
	}
	histAvg /= buffer->size();

	double variance = 0;
	for (double i : *buffer)
	{
		variance += pow((i - histAvg), 2);
	}
	variance /= buffer->size();

	buffer->pop_front();
	buffer->push_back(insEnergy);


	double C = (-0.0025714*variance) + 1.5142857;
	
	if (C < 1) C = 1.1;

	return insEnergy >= C*histAvg;
}

