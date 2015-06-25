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

#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "samplerate.h"

class ClientWriter {
public:
	ClientWriter();

	~ClientWriter();

	HRESULT Initialize(FLOAT SampleRate, HANDLE WaitEvent, CComPtr<IMMDevice> OutputDevice);

	void Clean();

	HRESULT Start();

	HRESULT Stop();

	HRESULT Write(FLOAT* Buffer, UINT BufferLength);

	HRESULT VerifyClient();

	REFERENCE_TIME GetPeriod() {
		return m_Period;
	}

	UINT32 GetPeriodFrames() {
		return m_PeriodFrames;
	}

	DOUBLE GetRatio() {
		return m_ResampleRatio;
	}

private:
	CComPtr<IAudioClient> m_Client;
	CComPtr<IAudioRenderClient> m_RenderClient;
	WAVEFORMATEXTENSIBLE* m_WaveFormat;
	DOUBLE m_ResampleRatio;
	SRC_STATE* m_ResampleState;
	UINT32 m_PeriodFrames;
	REFERENCE_TIME m_Period;
};