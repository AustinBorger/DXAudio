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
** that only read what's curently playing on the default audio output endpoint. */
class CDXAudioLoopbackStream final : public CDXAudioStream {
public:
	CDXAudioLoopbackStream();

	~CDXAudioLoopbackStream();

	//IDXAudioStream methods

	/* Returns the type of the stream */
	DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() final {
		return DXAUDIO_STREAM_TYPE_LOOPBACK;
	}

	//CDXAudioStream methods

	/* Initializes all stream objects and data */
	void ImplInitialize() final;

	/* Calls Start() on the client */
	void ImplStart() final;

	/* Calls Stop() on the client */
	void ImplStop() final;

	/* Reacts to a default device change by checking to see if the held device is no longer default,
	** then re-initializes to the new default device */
	void ImplDeviceChange() final;

	/* Reacts to a change in an endpoint property value by checking to see if the client is still valid -
	** if not, the client is re-initialized */
	void ImplPropertyChange() final;

	/* Reads data from the stream, then calls Process() on the callback object */
	void ImplProcess() final;

	//New methods

	/* Checks to see if the callback is valid, then calls CDXAudioStream::Initialize() */
	HRESULT Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback);

private:
	CComPtr<IMMDevice> m_OutputDevice; //The device we're reading from (output)
	CComPtr<IDXAudioReadCallback> m_ReadCallback; //The callback object
	LPWSTR m_DeviceID; //The output device's unique identifier
	ClientReader m_ClientReader; //Used for reading data from the stream
	ClientWriter m_ClientWriter; //Used only for event callback purposes (only silence is output)
	bool m_Running; //Indicates whether or not the stream is running (used for routing)

	/* Initializes the client reader object */
	void InitClientReader();

	/* Initializes the client writer object */
	void InitClientWriter();

	/* Responds to an HRESULT - if there is a failure, it will call the OnObjectFailure() method
	** on the callback object.  Otherwise, it will return S_OK. */
	HRESULT HandleHR(HRESULT hr);
};