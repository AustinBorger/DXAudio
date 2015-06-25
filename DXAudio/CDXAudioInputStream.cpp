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

#include "CDXAudioInputStream.h"
#include <math.h>

#define HANDLE_HR() if (FAILED(HandleHR(hr))) return

CDXAudioInputStream::CDXAudioInputStream() :
m_DeviceID(nullptr),
m_Running(false)
{ }

CDXAudioInputStream::~CDXAudioInputStream() {
	Halt();
	WaitForThread();

	if (m_DeviceID != nullptr) {
		CoTaskMemFree(m_DeviceID);
		m_DeviceID = nullptr;
	}
}

HRESULT CDXAudioInputStream::Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback) {
	HRESULT hr = S_OK;

	hr = pDXAudioCallback->QueryInterface (
		IID_PPV_ARGS(&m_ReadCallback)
	);

	if (FAILED(hr)) return E_INVALIDARG;

	m_SampleRate = SampleRate;

	hr = CDXAudioStream::Initialize();

	if (FAILED(hr)) return E_FAIL;

	return S_OK;
}

void CDXAudioInputStream::ImplInitialize() {
	HRESULT hr = S_OK;

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&m_InputDevice
	); HANDLE_HR();

	hr = m_InputDevice->GetId (
		&m_DeviceID
	); HANDLE_HR();

	InitClientReader();
}

void CDXAudioInputStream::ImplStart() {
	HRESULT hr = m_ClientReader.Start();
	m_Running = true;
	HANDLE_HR();
}

void CDXAudioInputStream::ImplStop() {
	HRESULT hr = m_ClientReader.Stop();
	m_Running = false;
	HANDLE_HR();
}

void CDXAudioInputStream::ImplDeviceChange() {
	HRESULT hr = S_OK;
	CComPtr<IMMDevice> DefaultDevice;
	LPWSTR DefaultDeviceID = nullptr;

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&DefaultDevice
	); HANDLE_HR();

	hr = DefaultDevice->GetId (
		&DefaultDeviceID
	); HANDLE_HR();

	if (wcscmp(DefaultDeviceID, m_DeviceID) != 0) {
		m_ClientReader.Clean();
		m_InputDevice.Release();
		CoTaskMemFree(m_DeviceID);

		m_InputDevice = DefaultDevice;
		m_DeviceID = DefaultDeviceID;

		InitClientReader();

		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
		}
	}

	if (DefaultDeviceID != m_DeviceID) {
		CoTaskMemFree(DefaultDeviceID);
		DefaultDeviceID = nullptr;
	}
}

void CDXAudioInputStream::ImplPropertyChange() {
	HRESULT hr = m_ClientReader.VerifyClient();

	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientReader.Clean();

		InitClientReader();

		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
		}
	} else HANDLE_HR();
}

void CDXAudioInputStream::ImplProcess() {
	HRESULT hr = S_OK;
	UINT InputBufferSize = (UINT)(ceil((DOUBLE)(m_ClientReader.GetPeriodFrames()) * m_ClientReader.GetRatio()));
	FLOAT* InputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * InputBufferSize));
	UINT FramesRead = 0;

	hr = m_ClientReader.Read(InputBuffer, InputBufferSize, FramesRead);

	if (FAILED(hr)) {
		HANDLE_HR();
	}

	m_ReadCallback->Process(m_SampleRate, InputBuffer, FramesRead);
}

void CDXAudioInputStream::InitClientReader() {
	HRESULT hr = m_ClientReader.Initialize (
		false,
		m_SampleRate,
		GetWaitEvent(),
		m_InputDevice
	); HANDLE_HR();
}

HRESULT CDXAudioInputStream::HandleHR(HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		return E_FAIL;
	} else {
		m_ReadCallback->OnObjectFailure(hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}