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

#define FILENAME L"ClientReader.cpp"
#define RETURN_HR(Line) if (FAILED(hr)) { if (hr != AUDCLNT_E_DEVICE_INVALIDATED) { m_Callback->OnObjectFailure(FILENAME, Line, hr); return E_FAIL; } else return hr; }
#define HALT_HR(Line) if (FAILED(hr)) { if (hr != AUDCLNT_E_DEVICE_INVALIDATED) { m_Callback->OnObjectFailure(FILENAME, Line, hr); m_Stream.Halt(); return; } else return; }

ClientReader::ClientReader(CDXAudioStream& Stream) :
m_Stream(Stream),
m_ResampleState(nullptr),
m_WaveFormat(nullptr)
{ }

ClientReader::~ClientReader() {
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

HRESULT ClientReader::Initialize(bool IsLoopback, FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> InputDevice, CComPtr<IDXAudioCallback> Callback) {
	HRESULT hr = S_OK;
	BYTE* Buffer = nullptr;
	int error = 0;

	m_Callback = Callback;

	//Create the SRC_STATE object
	m_ResampleState = src_new (
		SRC_SINC_FASTEST, //More than adequate for a real time stream
		2,				  //Two channels (stereo)
		&error
	); if (error != 0) {
		m_Callback->OnObjectFailure (
			FILENAME,
			__LINE__,
			E_FAIL
		); return E_FAIL;
	}

	//"Activate" the device (create the IAudioClient interface)
	hr = InputDevice->Activate (
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		(void**)(&m_Client)
	); RETURN_HR(__LINE__);

	//Retrieves the device period - this is how often new information is provided
	//to the stream, in 100-nanosecond units.
	hr = m_Client->GetDevicePeriod (
		&m_Period,
		nullptr
	); RETURN_HR(__LINE__);

	//Retrieves the mix format - this is the format in which the audio endpoint gives
	//us the data.  This data will need to be resampled, so we need to know what form
	//it's in.
	hr = m_Client->GetMixFormat (
		(WAVEFORMATEX**)(&m_WaveFormat)
	); RETURN_HR(__LINE__);

	//Initialize the client, marking how we're going to be using it
	hr = m_Client->Initialize (
		AUDCLNT_SHAREMODE_SHARED, //Always use shared - exclusive is meant for drivers and is unpredictable otherwise
		WaitEvent ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : NULL | //Use an event callback if specified
		IsLoopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : NULL, //Make this a loopback stream if specified
		m_Period, //Creates a buffer large enough to store one packet
		m_Period, //Use the endpoint's periodicity (this can't ve any other value)
		(WAVEFORMATEX*)(m_WaveFormat), //Pass in the wave format we just retrieved
		NULL //No audio session stuff
	); RETURN_HR(__LINE__);

	//Create the IAudioCaptureClient interface
	hr = m_Client->GetService (
		IID_PPV_ARGS(&m_CaptureClient)
	); RETURN_HR(__LINE__);

	//If using an event callback mechanism, provide the event handle (this is from CDXAudioStream)
	if (WaitEvent != NULL) {
		hr = m_Client->SetEventHandle (
			WaitEvent
		); RETURN_HR(__LINE__);
	}

	//Calculate the number of frames the endpoint is going to give us each period.
	m_PeriodFrames = (UINT32)(ceil(DOUBLE(m_Period * m_WaveFormat->Format.nSamplesPerSec) / 10000000));

	//Calculate the resample ratio - this is the ratio of the output sample rate to the input sample rate, IE
	//the sample rate specified by the application developer divided by the sample rate used by the endpoint.
	//This value is used by libsamplerate.
	m_ResampleRatio = DOUBLE(SampleRate) / DOUBLE(m_WaveFormat->Format.nSamplesPerSec); //Output sample rate / input sample rate

	return S_OK;
}

VOID ClientReader::Clean() {
	//Release all interfaces, free all memory, zero all values...
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

VOID ClientReader::Start() {
	HRESULT hr = S_OK;
	hr = m_Client->Start();
	HALT_HR(__LINE__);
}

VOID ClientReader::Stop() {
	HRESULT hr = S_OK;
	hr = m_Client->Stop();
	HALT_HR(__LINE__);
}

VOID ClientReader::Read(FLOAT* Buffer, UINT BufferLength, UINT& FramesRead) {
	HRESULT hr = S_OK;
	const UINT32 ExcessChannels = m_WaveFormat->Format.nChannels - 2;
	BYTE* ByteBuffer = nullptr;
	UINT32 FramesToRead = 0;
	DWORD Flags = NULL;

	//_alloca is safe here, as this is not recursive and only takes up a few KB at most
	FLOAT* LocalBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * m_PeriodFrames));
	FLOAT* LocalBufferIndex = LocalBuffer;
	SRC_DATA Data;
	int error = 0;

	//Start using the input data, reading only one packet.
	hr = m_CaptureClient->GetBuffer (
		&ByteBuffer,
		&FramesToRead,
		&Flags,
		NULL,
		NULL
	);

	//If the buffer is empty, that probably means we're in the middle of switching properties.
	//There is a slight delay between when the stream stops and the message is sent that
	//a property was changed, in which case GetBuffer will return AUDCLNT_S_BUFFER_EMPTY.
	//This should not happen in any other situation, as the input device always drives
	//the wait event, which is only set once every period.
	if (hr != AUDCLNT_S_BUFFER_EMPTY) {
		HALT_HR(__LINE__);
	} else return;

	//Convert the byte buffer into a stereo floating-point format and store
	//in LocalBuffer
	if (m_WaveFormat->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) { //PCM data
		if (m_WaveFormat->Samples.wValidBitsPerSample == 16) { //16-bit signed int
			for (UINT i = 0; i < FramesToRead; i++) {
				for (UINT j = 0; j < 2; j++) { //Two channels
					*LocalBufferIndex++ = FLOAT(*((INT16*)(ByteBuffer))) / 32767; //Convert to normalized float [-1.0, 1.0]
					ByteBuffer += sizeof(INT16); //Move the byte pointer to the next channel or sample
				}

				//Ignore excess channels by skipping over those bytes
				ByteBuffer += sizeof(INT16) * ExcessChannels;
			}
		} else if (m_WaveFormat->Format.wBitsPerSample == 24) { //24-bit unsigned (never signed)
			for (UINT i = 0; i < FramesToRead; i++) {
				for (UINT j = 0; j < 2; j++) {
					*LocalBufferIndex++ = (FLOAT(*((UINT32*)(ByteBuffer))) / 8388607) - 1.0f;
					ByteBuffer += 3;
				}

				ByteBuffer += 3 * ExcessChannels;
			}
		} else { //32-bit signed
			for (UINT i = 0; i < FramesToRead; i++) {
				for (UINT j = 0; j < 2; j++) {
					*LocalBufferIndex++ = FLOAT(*((INT32*)(ByteBuffer))) / 2147483647;
					ByteBuffer += sizeof(INT32);
				}

				ByteBuffer += sizeof(INT32) * ExcessChannels;
			}
		}
	} else { //32-bit floating-point
		for (UINT i = 0; i < FramesToRead; i++) {
			for (UINT j = 0; j < 2; j++) {
				*LocalBufferIndex++ = *((FLOAT*)(ByteBuffer));
				ByteBuffer += sizeof(FLOAT);
			}

			ByteBuffer += sizeof(FLOAT) * ExcessChannels;
		}
	}

	//We're done using the input data
	hr = m_CaptureClient->ReleaseBuffer (
		FramesToRead
	); HALT_HR(__LINE__);

	ByteBuffer = nullptr;

	//Fill the SRC_DATA structure
	Data.data_in = LocalBuffer;	//Use the converted stereo samples
	Data.data_out = Buffer;	//Store the result in the output buffer
	Data.end_of_input = 0; //Since this is realtime, there is never an end of input
	Data.input_frames = FramesToRead; //This is equal to m_PeriodFrames
	Data.input_frames_used = 0;	//Zero out this value (it's an out value generated by src_process)
	Data.output_frames = BufferLength; //Notify src_process of the length of the output buffer (always enough to store everything)
	Data.output_frames_gen = 0;	//Zero out this value (it's an out value generated by src_process)
	Data.src_ratio = m_ResampleRatio; //Use the current resample ratio

	//Resample the data to the sample rate specified by the application developer
	error = src_process (
		m_ResampleState,
		&Data
	); if (error != 0) {
		m_Callback->OnObjectFailure (
			FILENAME,
			__LINE__,
			E_FAIL
		); m_Stream.Halt(); return;
	}

	//Let the application developer know how many samples are available
	FramesRead = Data.output_frames_gen;
}

HRESULT ClientReader::VerifyClient() {
	UINT32 BufferFrames = 0;

	//This will return AUDCLNT_E_DEVICE_INVALIDATED if the IAudioClient object
	//becomes outdated.  This occurs if a property has been changed, such as
	//the sample rate or bit depth of the endpoint.  Otherwise, it will return S_OK,
	//verifying that the client is still valid.
	return m_Client->GetBufferSize(&BufferFrames);
}