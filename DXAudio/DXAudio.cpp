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

#include "DXAudio.h"
#include "CDXAudioOutputStream.h"
#include "CDXAudioInputStream.h"
#include "CDXAudioLoopbackStream.h"
#include "CDXAudioDuplexStream.h"
#include "CDXAudioEchoStream.h"

#include <atlbase.h>

#pragma comment(lib, "libsamplerate.lib")

/* Creates an output stream. */
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

/* Creates an input stream. */
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

/* Creates a loopback stream. */
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

/* Creates a duplex stream. */
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

/* Creates an echo stream. */
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

/* Entry point into the dll - creates a stream as specified by the application. */
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