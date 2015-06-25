#include "DXAudio.h"
#include "CDXAudioOutputStream.h"
#include "CDXAudioInputStream.h"
#include "CDXAudioLoopbackStream.h"
#include "CDXAudioDuplexStream.h"
#include "CDXAudioEchoStream.h"

#include <atlbase.h>

#pragma comment(lib, "libsamplerate.lib")

static HRESULT DXAudioCreateOutputStream (
	FLOAT SampleRate,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
) {
	HRESULT hr = S_OK;

	CComPtr<CDXAudioOutputStream> OutputStream = new CDXAudioOutputStream();

	hr = OutputStream->Initialize(SampleRate, pDXAudioCallback);

	if (FAILED(hr)) {
		*ppDXAudioStream = nullptr;
		return hr;
	}

	*ppDXAudioStream = OutputStream;

	return S_OK;
}

static HRESULT DXAudioCreateInputStream (
	FLOAT SampleRate,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
) {
	HRESULT hr = S_OK;

	CComPtr<CDXAudioInputStream> InputStream = new CDXAudioInputStream();

	hr = InputStream->Initialize(SampleRate, pDXAudioCallback);

	if (FAILED(hr)) {
		*ppDXAudioStream = nullptr;
		return hr;
	}

	*ppDXAudioStream = InputStream;

	return S_OK;
}

static HRESULT DXAudioCreateLoopbackStream (
	FLOAT SampleRate,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
) {
	HRESULT hr = S_OK;

	CComPtr<CDXAudioLoopbackStream> LoopbackStream = new CDXAudioLoopbackStream();

	hr = LoopbackStream->Initialize(SampleRate, pDXAudioCallback);

	if (FAILED(hr)) {
		*ppDXAudioStream = nullptr;
		return hr;
	}

	*ppDXAudioStream = LoopbackStream;

	return S_OK;
}

static HRESULT DXAudioCreateDuplexStream (
	FLOAT SampleRate,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
) {
	HRESULT hr = S_OK;

	CComPtr<CDXAudioDuplexStream> DuplexStream = new CDXAudioDuplexStream();

	hr = DuplexStream->Initialize(SampleRate, pDXAudioCallback);

	if (FAILED(hr)) {
		*ppDXAudioStream = nullptr;
		return hr;
	}

	*ppDXAudioStream = DuplexStream;

	return S_OK;
}

static HRESULT DXAudioCreateEchoStream (
	FLOAT SampleRate,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
) {
	HRESULT hr = S_OK;

	CComPtr<CDXAudioEchoStream> EchoStream = new CDXAudioEchoStream();

	hr = EchoStream->Initialize(SampleRate, pDXAudioCallback);

	if (FAILED(hr)) {
		*ppDXAudioStream = nullptr;
		return hr;
	}

	*ppDXAudioStream = EchoStream;

	return S_OK;
}

HRESULT DXAudioCreateStream (
	const DXAUDIO_STREAM_DESC* pDesc,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
) {
	switch (pDesc->Type) {
		case DXAUDIO_STREAM_TYPE_OUTPUT: {
			return DXAudioCreateOutputStream (
				pDesc->SampleRate,
				pDXAudioCallback,
				ppDXAudioStream
			);
		} break;

		case DXAUDIO_STREAM_TYPE_INPUT: {
			return DXAudioCreateInputStream (
				pDesc->SampleRate,
				pDXAudioCallback,
				ppDXAudioStream
			);
		} break;

		case DXAUDIO_STREAM_TYPE_LOOPBACK: {
			return DXAudioCreateLoopbackStream (
				pDesc->SampleRate,
				pDXAudioCallback,
				ppDXAudioStream
			);
		} break;

		case DXAUDIO_STREAM_TYPE_DUPLEX: {
			return DXAudioCreateDuplexStream (
				pDesc->SampleRate,
				pDXAudioCallback,
				ppDXAudioStream
			);
		} break;

		case DXAUDIO_STREAM_TYPE_ECHO: {
			return DXAudioCreateEchoStream (
				pDesc->SampleRate,
				pDXAudioCallback,
				ppDXAudioStream
			);
		} break;
	}

	return E_INVALIDARG;
}