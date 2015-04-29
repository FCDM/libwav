#include "stdafx.h"
#include "libwav.h"

using namespace std;

#pragma warning(disable: 4244)	//precision lost



WASAPI::Audio::Audio(Wave& audioContent, REFERENCE_TIME hnsBufferDuration)	//nano seconds
{
	this->audioContent = &audioContent;
	this->hnsBufferDuration = hnsBufferDuration;

	selectDefaultAudioDevice(&audioContent);
	pAudioClient->Start();
}


int WASAPI::Audio::framesAvailable()
{
	uint32_t PaddingFrames;
	hr = pAudioClient->GetCurrentPadding(&PaddingFrames);
	if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
	{
		Release();
		selectDefaultAudioDevice(audioContent);
		pAudioClient->Start();
		return 0;
	}
	return bufferFrameCount - PaddingFrames;
}

bool WASAPI::Audio::fillBuffer(memblock* mem)
{
	int nBlocks = mem->nBytes / audioContent->getH()->nBlockAlign;
	byte* ptr;
	if (nBlocks == 0)
	{
		if (mem->p >= (uintptr_t)(audioContent->get_data_size() + audioContent->get_data_p()))pAudioClient->Stop();;
		return false;
	}
	hr = pAudioRenderClient->GetBuffer(nBlocks, &ptr);
	if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
	{
		Release();
		selectDefaultAudioDevice(audioContent);
		pAudioClient->Start();
		return false;
	}
	if (hr != S_OK) throw new std::exception();
	memcpy(ptr, (void*)(mem->p), mem->nBytes);
	pAudioRenderClient->ReleaseBuffer(nBlocks, NULL);
	return true;
}

void WASAPI::Audio::selectDefaultAudioDevice(Wave* audioContent)
{
	hr = CoInitialize(NULL);
	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&pEnumerator);

	pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)(&pAudioClient));

	if (audioContent->isExtendedWave()) 
		hr = pAudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_RATEADJUST,
			hnsBufferDuration,
			0,
			&(audioContent->getWaveFormatExtensible()),
			NULL);
	else
		hr = pAudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_RATEADJUST,
			hnsBufferDuration,
			0,
			&(audioContent->getWaveFormatEx()),
			NULL);

	if (hr != S_OK) throw std::exception();

	pAudioClient->GetBufferSize(&bufferFrameCount);

	pAudioClient->GetService(__uuidof(IAudioClock), (void**)(&pAudioClock));

	pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)(&pAudioRenderClient));

	pAudioClient->GetService(__uuidof(IAudioStreamVolume), (void**)(&pAudioStreamVolume));

}

void WASAPI::Audio::Release()
{

	RELEASEIF_NONNULL(pAudioClock);
	RELEASEIF_NONNULL(pAudioRenderClient);
	RELEASEIF_NONNULL(pAudioStreamVolume);

	RELEASEIF_NONNULL(pAudioClient);
	RELEASEIF_NONNULL(pDevice);
	RELEASEIF_NONNULL(pEnumerator);

}

