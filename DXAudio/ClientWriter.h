#pragma once

#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "samplerate.h"

class ClientWriter {
public:
	ClientWriter();

	~ClientWriter();

	HRESULT Initialize(FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> OutputDevice);

	void Clean();

	HRESULT Start();

	HRESULT Stop();

	HRESULT Write(FLOAT* Buffer, UINT BufferLength);

	HRESULT VerifyClient();

	REFERENCE_TIME GetPeriod() {
		return m_Period;
	}

	UINT32 GetPeriodFrames() {
		return m_PeriodFrames;
	}

	DOUBLE GetRatio() {
		return m_ResampleRatio;
	}

private:
	CComPtr<IAudioClient> m_Client;
	CComPtr<IAudioRenderClient> m_RenderClient;
	WAVEFORMATEXTENSIBLE* m_WaveFormat;
	DOUBLE m_ResampleRatio;
	SRC_STATE* m_ResampleState;
	UINT32 m_PeriodFrames;
	REFERENCE_TIME m_Period;
};