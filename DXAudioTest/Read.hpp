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

#define _USE_MATH_DEFINES

#include "DXAudio.h"
#include "QueryInterface.h"
#include <iostream>
#include <math.h>

class Read : public IDXAudioReadCallback {
public:
	Read() : m_RefCount(1) { }

	~Read() { }

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) {
		QUERY_INTERFACE_CAST(IDXAudioReadCallback);
		QUERY_INTERFACE_CAST(IDXAudioCallback);
		QUERY_INTERFACE_CAST(IUnknown);
		QUERY_INTERFACE_FAIL();
	}

	ULONG STDMETHODCALLTYPE AddRef() {
		return ++m_RefCount;
	}

	ULONG STDMETHODCALLTYPE Release() {
		return --m_RefCount;
	}

	VOID STDMETHODCALLTYPE OnObjectFailure(HRESULT hr) final {
		_com_error e(hr);

		MessageBoxW(NULL, e.ErrorMessage(), L"Object Failure", MB_ICONERROR | MB_OK);

		ExitProcess(hr);
	}

	VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* InputBuffer, UINT BufferFrames) final {
		FLOAT RMS = 0.0F;

		for (UINT i = 0; i < BufferFrames; i++) {
			FLOAT Sample = InputBuffer[i * 2] + InputBuffer[i * 2 + 1];
			Sample /= 2.0F;
			RMS += Sample * Sample;
		}

		RMS /= BufferFrames;
		RMS = sqrt(RMS);

		std::cout << "RMS: " << RMS << std::endl;
	}

private:
	long m_RefCount;
};