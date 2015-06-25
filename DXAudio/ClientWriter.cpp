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
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*/

/*
** API documentation is available here:
**		https://github.com/AustinBorger/DXAudio
*/

#include "ClientWriter.h"
#include <math.h>

#define CHECK_HR() if (FAILED(hr)) return hr

ClientWriter::ClientWriter() :
m_ResampleState(nullptr),
m_WaveFormat(nullptr)
{ }

ClientWriter::~ClientWriter() {
	if (m_ResampleState != nullptr) {
		src_delete(m_ResampleState);
		m_ResampleState = nullptr;
	}

	if (m_WaveFormat != nullptr) {
		CoTaskMemFree(m_WaveFormat);
		m_WaveFormat = nullptr;
	}
}

HRESULT ClientWriter::Initialize(FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> OutputDevice) {
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

	hr = OutputDevice->Activate (
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
		WaitEvent ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : NULL,
		m_Period * 4,
		m_Period,
		(WAVEFORMATEX*)(m_WaveFormat),
		NULL
	); CHECK_HR();

	hr = m_Client->GetService (
		IID_PPV_ARGS(&m_RenderClient)
	); CHECK_HR();

	if (WaitEvent != NULL) {
		hr = m_Client->SetEventHandle (
			WaitEvent
		); CHECK_HR();
	}

	m_PeriodFrames = (UINT32)(ceil(DOUBLE(m_Period * m_WaveFormat->Format.nSamplesPerSec) / 10000000));

	hr = m_RenderClient->GetBuffer (
		m_PeriodFrames * 2,
		&Buffer
	); CHECK_HR();

	hr = m_RenderClient->ReleaseBuffer (
		m_PeriodFrames * 2,
		AUDCLNT_BUFFERFLAGS_SILENT
	); CHECK_HR();

	m_ResampleRatio = DOUBLE(m_WaveFormat->Format.nSamplesPerSec) / DOUBLE(SampleRate);

	return S_OK;
}

void ClientWriter::Clean() {
	m_RenderClient.Release();
	m_Client.Release();
	CoTaskMemFree(m_WaveFormat);
	m_WaveFormat = nullptr;
	src_delete(m_ResampleState);
	m_ResampleState = nullptr;
	m_ResampleRatio = 0.0;
	m_PeriodFrames = 0;
	m_Period = 0;
}

HRESULT ClientWriter::Start() {
	return m_Client->Start();
}

HRESULT ClientWriter::Stop() {
	return m_Client->Stop();
}

HRESULT ClientWriter::Write(FLOAT* Buffer, UINT BufferLength) {
	HRESULT hr = S_OK;
	UINT32 ExcessChannels = m_WaveFormat->Format.nChannels - 2;
	BYTE* ByteBuffer = nullptr;
	FLOAT* LocalBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * m_PeriodFrames));
	FLOAT* LocalBufferIndex = LocalBuffer;
	SRC_DATA Data;
	int error = 0;

	LocalBufferIndex = LocalBuffer;

	Data.data_in = Buffer;
	Data.data_out = LocalBuffer;
	Data.end_of_input = 0;
	Data.input_frames = BufferLength;
	Data.input_frames_used = 0;
	Data.output_frames = m_PeriodFrames;
	Data.output_frames_gen = 0;
	Data.src_ratio = m_ResampleRatio;

	error = src_process (
		m_ResampleState,
		&Data
	);

	if (error != 0) {
		return E_FAIL;
	}

	//Temp
	if (Data.output_frames_gen == 0 && BufferLength != 0) {
		return E_ACCESSDENIED;
	}

	hr = m_RenderClient->GetBuffer (
		Data.output_frames_gen,
		&ByteBuffer
	);

	if (FAILED(hr)) {
		if (hr == AUDCLNT_E_BUFFER_TOO_LARGE) {
			return S_OK;
		} else {
			return hr;
		}
	}

	if (m_WaveFormat->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
		if (m_WaveFormat->Samples.wValidBitsPerSample == 16) {
			for (long i = 0; i < Data.output_frames_gen; i++) {
				for (UINT j = 0; j < 2; j++) {
					*(INT16*)(ByteBuffer) = (INT16)(*LocalBufferIndex * 32767);
					ByteBuffer += sizeof(INT16);
					LocalBufferIndex++;
				}

				ZeroMemory(ByteBuffer, ExcessChannels * sizeof(INT16));
				ByteBuffer += sizeof(INT16) * ExcessChannels;
			}
		} else if (m_WaveFormat->Format.wBitsPerSample == 24) {
			for (long i = 0; i < Data.output_frames_gen; i++) {
				for (UINT j = 0; j < 2; j++) {
					*(UINT32*)(ByteBuffer) = (UINT32)((*LocalBufferIndex + 1.0f) * 8388607);
					ByteBuffer += 3;
					LocalBufferIndex++;
				}

				ZeroMemory(ByteBuffer, ExcessChannels * 3);
				ByteBuffer += 3 * ExcessChannels;
			}
		} else {
			for (long i = 0; i < Data.output_frames_gen; i++) {
				for (UINT j = 0; j < 2; j++) {
					*(INT32*)(ByteBuffer) = (INT32)(*LocalBufferIndex * 2147483647);
					ByteBuffer += sizeof(INT32);
					LocalBufferIndex++;
				}

				ZeroMemory(ByteBuffer, ExcessChannels * sizeof(INT32));
				ByteBuffer += sizeof(INT32) * ExcessChannels;
			}
		}
	} else {
		for (long i = 0; i < Data.output_frames_gen; i++) {
			for (UINT j = 0; j < 2; j++) {
				*(FLOAT*)(ByteBuffer) = *LocalBufferIndex;
				ByteBuffer += sizeof(FLOAT);
				LocalBufferIndex++;
			}

			ZeroMemory(ByteBuffer, ExcessChannels * sizeof(FLOAT));
			ByteBuffer += sizeof(FLOAT) * ExcessChannels;
		}
	}

	hr = m_RenderClient->ReleaseBuffer (
		Data.output_frames_gen,
		NULL
	);

	CHECK_HR();

	return S_OK;
}

HRESULT ClientWriter::VerifyClient() {
	UINT32 BufferFrames = 0;

	return m_Client->GetBufferSize(&BufferFrames);
}