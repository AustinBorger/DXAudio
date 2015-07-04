#include "DXAudioResampler.h"
#include "CDXAudioResampler.h"

#include <atlbase.h>

HRESULT DXAudioCreateResampler(IDXAudioResampler** ppDXAudioResampler) {
	HRESULT hr = S_OK;

	CComPtr<CDXAudioResampler> Resampler = new CDXAudioResampler();

	hr = Resampler->Initialize();

	if (FAILED(hr)) {
		*ppDXAudioResampler = nullptr;
		return hr;
	}

	*ppDXAudioResampler = Resampler;

	return S_OK;
}