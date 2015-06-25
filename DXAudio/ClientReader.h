#pragma once

#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "samplerate.h"

class ClientReader {
public:
	ClientReader();

	~ClientReader();

	HRESULT Initialize(bool IsLoopback, FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> InputDevice);

	void Clean();

	HRESULT Start();

	HRESULT Stop();

	HRESULT Read(FLOAT* Buffer, UINT BufferLength, UINT& FramesRead);

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
	CComPtr<IAudioCaptureClient> m_CaptureClient;
	WAVEFORMATEXTENSIBLE* m_WaveFormat;
	DOUBLE m_ResampleRatio;
	SRC_STATE* m_ResampleState;
	UINT32 m_PeriodFrames;
	REFERENCE_TIME m_Period;
};