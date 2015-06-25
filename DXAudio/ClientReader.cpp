/*
** Copyright (C) 2015 Austin Borger <aaborger@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
** API documentation is available here:
**		https://github.com/AustinBorger/DXAudio
*/

#include "ClientReader.h"
#include <math.h>

#define CHECK_HR() if (FAILED(hr)) return hr

ClientReader::ClientReader() :
m_ResampleState(nullptr),
m_WaveFormat(nullptr)
{ }

ClientReader::~ClientReader() {
	if (m_ResampleState != nullptr) {
		src_delete(m_ResampleState);
		m_ResampleState = nullptr;
	}

	if (m_WaveFormat != nullptr) {
		CoTaskMemFree(m_WaveFormat);
		m_WaveFormat = nullptr;
	}
}

HRESULT ClientReader::Initialize(bool IsLoopback, FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> InputDevice) {
	HRESULT hr = S_OK;
	BYTE* Buffer = nullptr;
	int error = 0;

	m_ResampleState = src_new (
		SRC_SINC_FASTEST,
		2,
		&error
	);

	if (error != 0) {
		return E_FAIL;
	}

	hr = InputDevice->Activate (
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		(void**)(&m_Client)
	); CHECK_HR();

	hr = m_Client->GetDevicePeriod (
		&m_Period,
		nullptr
	); CHECK_HR();

	hr = m_Client->GetMixFormat (
		(WAVEFORMATEX**)(&m_WaveFormat)
	); CHECK_HR();

	hr = m_Client->Initialize (
		AUDCLNT_SHAREMODE_SHARED,
		WaitEvent ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : NULL |
		IsLoopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : NULL,
		m_Period * 2,
		m_Period,
		(WAVEFORMATEX*)(m_WaveFormat),
		NULL
	); CHECK_HR();

	hr = m_Client->GetService (
		IID_PPV_ARGS(&m_CaptureClient)
	); CHECK_HR();

	if (WaitEvent != NULL) {
		hr = m_Client->SetEventHandle (
			WaitEvent
		); CHECK_HR();
	}

	m_PeriodFrames = (UINT32)(ceil(DOUBLE(m_Period * m_WaveFormat->Format.nSamplesPerSec) / 10000000));

	m_ResampleRatio = DOUBLE(SampleRate) / DOUBLE(m_WaveFormat->Format.nSamplesPerSec); //Output sample rate / input sample rate

	return S_OK;
}

void ClientReader::Clean() {
	m_CaptureClient.Release();
	m_Client.Release();
	CoTaskMemFree(m_WaveFormat);
	m_WaveFormat = nullptr;
	src_delete(m_ResampleState);
	m_ResampleState = nullptr;
	m_ResampleRatio = 0.0;
	m_PeriodFrames = 0;
	m_Period = 0;
}

HRESULT ClientReader::Start() {
	return m_Client->Start();
}

HRESULT ClientReader::Stop() {
	return m_Client->Stop();
}

HRESULT ClientReader::Read(FLOAT* Buffer, UINT BufferLength, UINT& FramesRead) {
	HRESULT hr = S_OK;
	UINT32 ExcessChannels = m_WaveFormat->Format.nChannels - 2;
	BYTE* ByteBuffer = nullptr;
	UINT32 FramesToRead = 0;
	DWORD Flags = NULL;
	FLOAT* LocalBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * m_PeriodFrames));
	FLOAT* LocalBufferIndex = LocalBuffer;
	SRC_DATA Data;
	int error = 0;

	hr = m_CaptureClient->GetBuffer (
		&ByteBuffer,
		&FramesToRead,
		&Flags,
		NULL,
		NULL
	); CHECK_HR();

	if (m_WaveFormat->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
		if (m_WaveFormat->Samples.wValidBitsPerSample == 16) {
			for (UINT i = 0; i < FramesToRead; i++) {
				for (UINT j = 0; j < 2; j++) {
					*LocalBufferIndex++ = FLOAT(*((INT16*)(ByteBuffer))) / 32767;
					ByteBuffer += sizeof(INT16);
				}

				ByteBuffer += sizeof(INT16) * ExcessChannels;
			}
		} else if (m_WaveFormat->Format.wBitsPerSample == 24) {
			for (UINT i = 0; i < FramesToRead; i++) {
				for (UINT j = 0; j < 2; j++) {
					*LocalBufferIndex++ = (FLOAT(*((UINT32*)(ByteBuffer))) / 8388607) - 1.0f;
					ByteBuffer += 3;
				}

				ByteBuffer += 3 * ExcessChannels;
			}
		} else {
			for (UINT i = 0; i < FramesToRead; i++) {
				for (UINT j = 0; j < 2; j++) {
					*LocalBufferIndex++ = FLOAT(*((INT32*)(ByteBuffer))) / 2147483647;
					ByteBuffer += sizeof(INT32);
				}

				ByteBuffer += sizeof(INT32) * ExcessChannels;
			}
		}
	} else {
		for (UINT i = 0; i < FramesToRead; i++) {
			for (UINT j = 0; j < 2; j++) {
				*LocalBufferIndex++ = *((FLOAT*)(ByteBuffer));
				ByteBuffer += sizeof(FLOAT);
			}

			ByteBuffer += sizeof(FLOAT) * ExcessChannels;
		}
	}

	hr = m_CaptureClient->ReleaseBuffer (
		FramesToRead
	); CHECK_HR();

	ByteBuffer = nullptr;

	Data.data_in = LocalBuffer;
	Data.data_out = Buffer;
	Data.end_of_input = 0;
	Data.input_frames = m_PeriodFrames;
	Data.input_frames_used = 0;
	Data.output_frames = BufferLength;
	Data.output_frames_gen = 0;
	Data.src_ratio = m_ResampleRatio;

	error = src_process (
		m_ResampleState,
		&Data
	);

	FramesRead = Data.output_frames_gen;

	if (error != 0) {
		return E_FAIL;
	}

	return S_OK;
}

HRESULT ClientReader::VerifyClient() {
	UINT32 BufferFrames = 0;

	return m_Client->GetBufferSize(&BufferFrames);
}