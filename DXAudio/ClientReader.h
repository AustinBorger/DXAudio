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

#pragma once

#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "DXAudio.h"
#include "samplerate.h"
#include "CDXAudioStream.h"

/* ClientReader is used to read stream data from an endpoint.  This can be used
** for both an input device or an output device for a loopback stream. */
class ClientReader {
public:
	ClientReader(CDXAudioStream& Stream);

	~ClientReader();

	/* This initializes the reader by creating the necessary interfaces and data.  [IsLoopback] is
	** used to indicate whether or not this is a loopback stream.  [SampleRate] is the desired sample
	** rate to be used by the stream callback.  The endpoint data will automatically be resampled
	** to this format.  [WaitEvent] is the event handle for the event callback mechanism - if NULL,
	** there will be no event callback on this end. */
	HRESULT Initialize(bool IsLoopback, FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> InputDevice, CComPtr<IDXAudioCallback> Callback);

	/* This releases all interfaces and dynamically allocated data and sets the object to a pre-initialized state. */
	VOID Clean();

	/* This starts the stream. */
	VOID Start();

	/* This stops the stream. */
	VOID Stop();

	/* This should be called to read the input data from the stream.  [BufferLength] is the size of the buffer,
	** which may or may not be the expected number of frames to be generated.  [FramesRead] stores the actual
	** number of frames read from the input stream. */
	VOID Read(FLOAT* Buffer, UINT BufferLength, UINT& FramesRead);

	/* This determines if the client is still in a valid, usable state. */
	HRESULT VerifyClient();

	/* Returns the periodicity of the stream in 100-nanosecond units. */
	REFERENCE_TIME GetPeriod() {
		return m_Period;
	}

	/* Returns the number of frames in a period at the sample rate of the endpoint. */
	UINT32 GetPeriodFrames() {
		return m_PeriodFrames;
	}


	/* Returns the resample ratio, which is equal to (application sample rate) / (endpoint sample rate) */
	DOUBLE GetRatio() {
		return m_ResampleRatio;
	}

private:
	CComPtr<IDXAudioCallback> m_Callback; //Used for error reporting
	CComPtr<IAudioClient> m_Client; //Audio client interface (WASAPI)
	CComPtr<IAudioCaptureClient> m_CaptureClient; //Capture client interface (WASAPI)
	WAVEFORMATEXTENSIBLE* m_WaveFormat; //The wave format of the endpoint
	DOUBLE m_ResampleRatio; //The resample ratio for the stream
	SRC_STATE* m_ResampleState; //The resample state (libsamplerate object)
	UINT32 m_PeriodFrames; //Number of frames in a period
	REFERENCE_TIME m_Period; //Periodicity of the endpoint
	CDXAudioStream& m_Stream; //Stream reference
};