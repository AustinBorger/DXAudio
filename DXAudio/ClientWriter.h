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

/* ClientWriter is used to write stream data to an endpoint.  This can only be
** used with output endpoints. */
class ClientWriter {
public:
	ClientWriter(CDXAudioStream& Stream);

	~ClientWriter();

	/* This initializes the writer by creating the necessary interfaces and data. [SampleRate] is the desired
	** sample rate to be used by the stream callback.  The endpoint data will automatically be resampled
	** from this format.  [WaitEvent] is the event handle for the event callback mechanism - if NULL,
	** there will be no event callback on this end. */
	HRESULT Initialize(FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> OutputDevice, CComPtr<IDXAudioCallback> Callback);

	/* This releases all interfaces and dynamically allocated data and sets the object to a pre-initialized state. */
	VOID Clean();

	/* This starts the stream. */
	VOID Start();

	/* This stops the stream. */
	VOID Stop();

	/* This should be called to write the output data to the stream.  [BufferLength] is the size of the buffer,
	** which is the number of frames to be provided. */
	VOID Write(FLOAT* Buffer, UINT BufferLength);

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

	/* Returns the resample ratio, which is equal to (endpoint sample rate) / (application sample rate) */
	DOUBLE GetRatio() {
		return m_ResampleRatio;
	}

private:
	CComPtr<IDXAudioCallback> m_Callback; //Used for error reporting
	CComPtr<IAudioClient> m_Client; //Audio client interface (WASAPI)
	CComPtr<IAudioRenderClient> m_RenderClient; //Render client interface (WASAPI)
	WAVEFORMATEXTENSIBLE* m_WaveFormat; //The wave format of the endpoint
	DOUBLE m_ResampleRatio; //The resample ratio for the stream
	SRC_STATE* m_ResampleState; //The resample state (libsamplerate object)
	UINT32 m_PeriodFrames; //Number of frames in a period
	REFERENCE_TIME m_Period; //Periodicity of the endpoint
	CDXAudioStream& m_Stream; //Stream reference
};