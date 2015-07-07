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

#include "DXAudio.h"
#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "CDXAudioStream.h"
#include "ClientReader.h"
#include "ClientWriter.h"

/* This class is a final implementation of IDXAudioStream.  It is used for streams
** that both read/write data to/from the default audio input/output endpoints. */
class CDXAudioDuplexStream : public CDXAudioStream {
public:
	CDXAudioDuplexStream();

	~CDXAudioDuplexStream();

	//IDXAudioStream methods

	/* Returns the type of the stream */
	DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() final {
		return DXAUDIO_STREAM_TYPE_DUPLEX;
	}

	//CDXAudioStream methods

	/* Initializes all stream objects and data */
	VOID ImplInitialize() final;

	/* Calls Start() on the client */
	VOID ImplStart() final;

	/* Calls Stop() on the client */
	VOID ImplStop() final;

	/* Reacts to a default device change by checking to see if the held device is no longer default,
	** then re-initializes to the new default device */
	VOID ImplDeviceChange() final;

	/* Reacts to a change in an endpoint property value by checking to see if the client is still valid -
	** if not, the client is re-initialized */
	VOID ImplPropertyChange() final;

	/* Reads data from the input stream, calls Process() on the callback object, then writes the given data
	** to the output stream. */
	VOID ImplProcess() final;

	//New methods

	/* Checks to see if the callback is valid, then calls CDXAudioStream::Initialize() */
	HRESULT Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback);

private:
	CComPtr<IMMDevice> m_InputDevice; //The device we're reading from
	CComPtr<IMMDevice> m_OutputDevice; //The device we're writing to
	CComPtr<IDXAudioReadWriteCallback> m_ReadWriteCallback; //The callback object
	LPWSTR m_InputDeviceID; //The input device's unique identifier
	LPWSTR m_OutputDeviceID; //The output device's unique identifier
	ClientReader m_ClientReader; //Used for reading input data from the stream
	ClientWriter m_ClientWriter; //Used for writing output data to the stream
	bool m_Running; //Indicates whether or not the stream is running (used for routing)

	/* Initializes the client reader object */
	VOID InitClientReader();

	/* Initializes the client writer object */
	VOID InitClientWriter();

	/* Responds to an HRESULT - if there is a failure, it will call the OnObjectFailure() method
	** on the callback object.  Otherwise, it will return S_OK. */
	HRESULT HandleHR(UINT Line, HRESULT hr);
};
