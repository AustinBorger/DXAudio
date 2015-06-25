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

#pragma once

#include "DXAudio.h"
#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "CMMNotificationClientListener.h"
#include "CDXAudioStream.h"
#include "QueryInterface.h"
#include "ClientWriter.h"

class CDXAudioOutputStream : public CDXAudioStream {
public:
	CDXAudioOutputStream();

	virtual ~CDXAudioOutputStream();

	//IDXAudioStream methods

	DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() final {
		return DXAUDIO_STREAM_TYPE_OUTPUT;
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
	CComPtr<IMMDevice> m_OutputDevice;
	CComPtr<IDXAudioWriteCallback> m_WriteCallback;
	LPWSTR m_DeviceID;
	ClientWriter m_ClientWriter;
	DOUBLE m_SamplesNeeded;
	bool m_Running;

	void InitClientWriter();

	HRESULT HandleHR(HRESULT hr);
};