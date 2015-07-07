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
#include "QueryInterface.h"
#include "ClientWriter.h"

/* This class is a final implementation of IDXAudioStream.  It is used for streams
** that only output data to the default audio output endpoint. */
class CDXAudioOutputStream : public CDXAudioStream {
public:
	CDXAudioOutputStream();

	virtual ~CDXAudioOutputStream();

	//IDXAudioStream methods

	/* Returns the type of the stream */
	DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() final {
		return DXAUDIO_STREAM_TYPE_OUTPUT;
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

	/* Calls Process() on the callback object, then writes the given data to the stream */
	VOID ImplProcess() final;

	//New methods

	/* Checks to see if the callback is valid, then calls CDXAudioStream::Initialize() */
	HRESULT Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback);

private:
	CComPtr<IMMDevice> m_OutputDevice; //The device we're outputting to
	CComPtr<IDXAudioWriteCallback> m_WriteCallback; //The callback object
	LPWSTR m_DeviceID; //The output device's unique identifier
	ClientWriter m_ClientWriter; //Used for writing data to the endpoint
	DOUBLE m_SamplesNeeded; //Prevents padding loss by keeping track of decimal amounts of samples
	bool m_Running; //Indicates whether or not the stream is running (used for routing)

	/* Initializes the client writer object */
	VOID InitClientWriter();

	/* Responds to an HRESULT - if there is a failure, it will call the OnObjectFailure() method
	** on the callback object.  Otherwise, it will return S_OK. */
	HRESULT HandleHR(UINT Line, HRESULT hr);
};