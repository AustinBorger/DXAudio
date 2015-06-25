#pragma once

#define _USE_MATH_DEFINES

#include "DXAudio.h"
#include "QueryInterface.h"
#include <math.h>

class Write : public IDXAudioWriteCallback {
public:
	Write() : m_RefCount(1), pos(0.0F) { }

	~Write() { }

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) {
		QUERY_INTERFACE_CAST(IDXAudioWriteCallback);
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

	VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* OutputBuffer, UINT BufferFrames) final {
		for (UINT i = 0; i < BufferFrames; i++) {
			OutputBuffer[i * 2] = 0.3F * sin(pos);
			OutputBuffer[i * 2 + 1] = OutputBuffer[i * 2];
			pos += 440.0F * FLOAT(M_PI) * 2.0F / SampleRate;

			if (pos >= FLOAT(M_PI) * 2.0F) {
				pos -= FLOAT(M_PI) * 2.0F;
			}
		}
	}

private:
	long m_RefCount;
	float pos;
};