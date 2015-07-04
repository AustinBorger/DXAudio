#pragma once

#include "DXAudioResampler.h"
#include "samplerate.h"
#include "QueryInterface.h"

class CDXAudioResampler : public IDXAudioResampler {
public:
	CDXAudioResampler();

	~CDXAudioResampler();

	//IUnknown methods

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) final {
		QUERY_INTERFACE_CAST(IDXAudioResampler);
		QUERY_INTERFACE_CAST(IUnknown);
		QUERY_INTERFACE_FAIL();
	}

	ULONG STDMETHODCALLTYPE AddRef() {
		return ++m_RefCount;
	}

	ULONG STDMETHODCALLTYPE Release() {
		m_RefCount--;

		if (m_RefCount <= 0) {
			delete this;
			return 0;
		}

		return m_RefCount;
	}

	//IDXAudioResampler methods

	VOID STDMETHODCALLTYPE Process (
		FLOAT* InBuffer,
		UINT InBufferFrames,
		FLOAT* OutBuffer,
		UINT OutBufferFrames,
		DOUBLE Ratio
	) final;

	//New methods

	HRESULT Initialize();

private:
	long m_RefCount;
	SRC_STATE* m_SrcState;
};