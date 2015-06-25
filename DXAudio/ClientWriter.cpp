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

#include "ClientWriter.h"
#include <math.h>

#define CHECK_HR() if (FAILED(hr)) return hr

ClientWriter::ClientWriter() :
m_ResampleState(nullptr),
m_WaveFormat(nullptr)
{ }

ClientWriter::~ClientWriter() {
	//Free all dynamically allocated data
	//The interfaces will be freed for us
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

	//Create the SRC_STATE object
	m_ResampleState = src_new (
		SRC_SINC_FASTEST, //More than adequate for a real time stream
		2,				  //Two channels (stereo)
		&error
	); if (error != 0) return E_FAIL;

	//"Activate" the device (create the IAudioClient interface)
	hr = OutputDevice->Activate (
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		(void**)(&m_Client)
	); CHECK_HR();

	//Retrieves the device period - this is how often we need to provide new information
	//to the stream, in 100-nanosecond units.
	hr = m_Client->GetDevicePeriod (
		&m_Period,
		nullptr
	); CHECK_HR();

	//Retrieves the mix format - this is the format in which we must deliver the audio data
	//to the endpoint.
	hr = m_Client->GetMixFormat (
		(WAVEFORMATEX**)(&m_WaveFormat)
	); CHECK_HR();

	//Initialize the client, marking how we're going to be using it
	hr = m_Client->Initialize (
		AUDCLNT_SHAREMODE_SHARED, //Always use shared - exclusive is meant for drivers and is unpredictable otherwise
		WaitEvent ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : NULL, //Use an event callback if specified
		m_Period * 4, //Creates a buffer large enough to store four packets - important for duplex streams
		m_Period, //Use the endpoint's periodicity (this can't ve any other value)
		(WAVEFORMATEX*)(m_WaveFormat), //Pass in the wave format we just retrieved
		NULL //No audio session stuff
	); CHECK_HR();

	//Create the IAudioRenderClient interface
	hr = m_Client->GetService (
		IID_PPV_ARGS(&m_RenderClient)
	); CHECK_HR();

	//If using an event callback mechanism, provide the event handle (this is from CDXAudioStream)
	if (WaitEvent != NULL) {
		hr = m_Client->SetEventHandle (
			WaitEvent
		); CHECK_HR();
	}

	//Calculate the number of frames the endpoint is going to need from us each period.
	m_PeriodFrames = (UINT32)(ceil(DOUBLE(m_Period * m_WaveFormat->Format.nSamplesPerSec) / 10000000));

	//We need to initialize the client with a little bit of slience.  The endpoint requires one period
	//worth of silence before Start() is called to even work.  We should give it two just in case the stream
	//runs a little bit behind to prevent pops and clicks, which happen even under a light CPU load.
	//This increases latency by ~10ms, but that's a tradeoff we have to take for the audio to "sound good."
	hr = m_RenderClient->GetBuffer (
		m_PeriodFrames * 2,
		&Buffer
	); CHECK_HR();

	//ReleaseBuffer() has to be called to unlock the buffer resource for the audio engine to use.
	hr = m_RenderClient->ReleaseBuffer (
		m_PeriodFrames * 2,
		AUDCLNT_BUFFERFLAGS_SILENT
	); CHECK_HR();

	//Calculate the resample ratio - this is the ratio of the output sample rate to the input sample rate, IE
	//the sample rate specified used by the endpoint divided by that which is specified by the application developer.
	//This value is used by libsamplerate.
	m_ResampleRatio = DOUBLE(m_WaveFormat->Format.nSamplesPerSec) / DOUBLE(SampleRate);

	return S_OK;
}

void ClientWriter::Clean() {
	//Release all interfaces, free all memory, zero all values...
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

	//_alloca is safe here, as this is not recursive and only takes up a few KB at most
	const UINT LocalBufferSize = (UINT)(ceil(m_PeriodFrames * m_ResampleRatio));
	FLOAT* LocalBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * LocalBufferSize));
	FLOAT* LocalBufferIndex = LocalBuffer;
	SRC_DATA Data;
	int error = 0;

	LocalBufferIndex = LocalBuffer;

	//Start by resampling the data given to us by the application developer.
	Data.data_in = Buffer; //Use the data given to us
	Data.data_out = LocalBuffer; //Store the resampled frames in the local buffer
	Data.end_of_input = 0; //Since this is realtime, there is never an end of input
	Data.input_frames = BufferLength; //This is equal to m_PeriodFrames / m_ResampleRatio
	Data.input_frames_used = 0;	//Zero out this value (it's an out value generated by src_process)
	Data.output_frames = LocalBufferSize; //This is the number of frames to give the endpoint
	Data.output_frames_gen = 0; //Zero out this value (it's an out value generated by src_process)
	Data.src_ratio = m_ResampleRatio; //Use the current resample ratio

	//Resample the data to the sample rate used by the endpoint
	error = src_process (
		m_ResampleState,
		&Data
	); if (error != 0) return E_FAIL;

	//Lock the buffer resource
	hr = m_RenderClient->GetBuffer (
		Data.output_frames_gen,
		&ByteBuffer
	);

	if (FAILED(hr)) {
		//If GetBuffer() returns AUDCLNT_E_BUFFER_TOO_LARGE, this is because
		//the endpoint is in the middle of switching properties, but hasn't
		//notified us of the change just yet.  Until then, the stream remains
		//in a stopped state, but may still call our event (or, if a duplex stream,
		//the input device will be the one calling the event, basically pushing
		//the stream data into a brick wall).  In this case, just skip this frame.
		if (hr == AUDCLNT_E_BUFFER_TOO_LARGE) {
			return S_OK;
		} else {
			return hr;
		}
	}

	//Convert the local buffer into the endpoint format and store it
	//in the buffer resource
	if (m_WaveFormat->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) { //PCM data
		if (m_WaveFormat->Samples.wValidBitsPerSample == 16) { //16-bit signed int
			for (long i = 0; i < Data.output_frames_gen; i++) {
				for (UINT j = 0; j < 2; j++) { //Two channels
					*(INT16*)(ByteBuffer) = (INT16)(*LocalBufferIndex * 32767); //Convert from normalized float [-1.0, 1.0]
					ByteBuffer += sizeof(INT16); //Move the byte pointer to the next channel or sample
					LocalBufferIndex++; //Move the local buffer pointer to the next channel or sample
				}

				//Ignore excess channels by zeroing out those bytes
				ZeroMemory(ByteBuffer, ExcessChannels * sizeof(INT16));
				ByteBuffer += sizeof(INT16) * ExcessChannels;
			}
		} else if (m_WaveFormat->Format.wBitsPerSample == 24) { //24-bit unsigned (never signed)
			for (long i = 0; i < Data.output_frames_gen; i++) {
				for (UINT j = 0; j < 2; j++) {
					*(UINT32*)(ByteBuffer) = (UINT32)((*LocalBufferIndex + 1.0f) * 8388607);
					ByteBuffer += 3;
					LocalBufferIndex++;
				}

				ZeroMemory(ByteBuffer, ExcessChannels * 3);
				ByteBuffer += 3 * ExcessChannels;
			}
		} else { //32-bit signed
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
	} else { //32-bit floating-point
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

	//We're done using the data
	hr = m_RenderClient->ReleaseBuffer (
		Data.output_frames_gen,
		NULL
	); CHECK_HR();

	return S_OK;
}

HRESULT ClientWriter::VerifyClient() {
	UINT32 BufferFrames = 0;

	//This will return AUDCLNT_E_DEVICE_INVALIDATED if the IAudioClient object
	//becomes outdated.  This occurs if a property has been changed, such as
	//the sample rate or bit depth of the endpoint.  Otherwise, it will return S_OK,
	//verifying that the client is still valid.
	return m_Client->GetBufferSize(&BufferFrames);
}