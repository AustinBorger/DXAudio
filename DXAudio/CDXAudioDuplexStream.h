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

class CDXAudioDuplexStream : public CDXAudioStream {
public:
	CDXAudioDuplexStream();

	~CDXAudioDuplexStream();

	//IDXAudioStream methods

	DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() final {
		return DXAUDIO_STREAM_TYPE_DUPLEX;
	}

	//CDXAudioStream methods

	void ImplInitialize() final;

	void ImplStart() final;

	void ImplStop() final;

	void ImplDeviceChange() final;

	void ImplPropertyChange() final;

	void ImplProcess() final;

	//New methods

	HRESULT Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback);

private:
	CComPtr<IMMDevice> m_InputDevice;
	CComPtr<IMMDevice> m_OutputDevice;
	CComPtr<IDXAudioReadWriteCallback> m_ReadWriteCallback;
	LPWSTR m_InputDeviceID;
	LPWSTR m_OutputDeviceID;
	ClientReader m_ClientReader;
	ClientWriter m_ClientWriter;
	bool m_Running;

	void InitClientReader();

	void InitClientWriter();

	HRESULT HandleHR(HRESULT hr);
};