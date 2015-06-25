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

#include "CDXAudioDuplexStream.h"
#include <math.h>

#define HANDLE_HR() if (FAILED(HandleHR(hr))) return

CDXAudioDuplexStream::CDXAudioDuplexStream() :
m_InputDeviceID(nullptr),
m_OutputDeviceID(nullptr),
m_Running(false)
{ }

CDXAudioDuplexStream::~CDXAudioDuplexStream() {
	Halt();
	WaitForThread();

	if (m_InputDeviceID != nullptr) {
		CoTaskMemFree(m_InputDeviceID);
		m_InputDeviceID = nullptr;
	}

	if (m_OutputDeviceID != nullptr) {
		CoTaskMemFree(m_OutputDeviceID);
		m_OutputDeviceID = nullptr;
	}
}

HRESULT CDXAudioDuplexStream::Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback) {
	HRESULT hr = S_OK;

	hr = pDXAudioCallback->QueryInterface (
		IID_PPV_ARGS(&m_ReadWriteCallback)
	);

	if (FAILED(hr)) return E_INVALIDARG;

	m_SampleRate = SampleRate;

	hr = CDXAudioStream::Initialize();

	if (FAILED(hr)) return E_FAIL;

	return S_OK;
}

void CDXAudioDuplexStream::ImplInitialize() {
	HRESULT hr = S_OK;

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&m_InputDevice
	); HANDLE_HR();

	hr = m_InputDevice->GetId (
		&m_InputDeviceID
	); HANDLE_HR();

	InitClientReader();

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&m_OutputDevice
	); HANDLE_HR();

	hr = m_OutputDevice->GetId (
		&m_OutputDeviceID
	); HANDLE_HR();

	InitClientWriter();
}

void CDXAudioDuplexStream::ImplStart() {
	m_Running = true;
	HRESULT hr = m_ClientReader.Start();
	HANDLE_HR();
	hr = m_ClientWriter.Start();
	HANDLE_HR();
}

void CDXAudioDuplexStream::ImplStop() {
	m_Running = false;
	HRESULT hr = m_ClientReader.Stop();
	HANDLE_HR();
	hr = m_ClientWriter.Stop();
	HANDLE_HR();
}

void CDXAudioDuplexStream::ImplDeviceChange() {
	HRESULT hr = S_OK;
	CComPtr<IMMDevice> DefaultDevice;
	LPWSTR DefaultDeviceID = nullptr;

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&DefaultDevice
	); HANDLE_HR();

	hr = DefaultDevice->GetId (
		&DefaultDeviceID
	); HANDLE_HR();

	if (wcscmp(DefaultDeviceID, m_OutputDeviceID) != 0) {
		m_ClientWriter.Clean();
		m_OutputDevice.Release();
		CoTaskMemFree(m_OutputDeviceID);

		m_OutputDevice = DefaultDevice;
		m_OutputDeviceID = DefaultDeviceID;

		InitClientWriter();

		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}
	}

	if (DefaultDeviceID != m_OutputDeviceID) {
		CoTaskMemFree(DefaultDeviceID);
	}

	DefaultDeviceID = nullptr;

	DefaultDevice.Release();

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&DefaultDevice
	); HANDLE_HR();

	hr = DefaultDevice->GetId (
		&DefaultDeviceID
	); HANDLE_HR();

	if (wcscmp(DefaultDeviceID, m_InputDeviceID) != 0) {
		m_ClientReader.Clean();
		m_InputDevice.Release();
		CoTaskMemFree(m_InputDeviceID);

		m_InputDevice = DefaultDevice;
		m_InputDeviceID = DefaultDeviceID;

		InitClientReader();

		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
		}
	}

	if (DefaultDeviceID != m_InputDeviceID) {
		CoTaskMemFree(DefaultDeviceID);
		DefaultDeviceID = nullptr;
	}
}

void CDXAudioDuplexStream::ImplPropertyChange() {
	HRESULT hr = m_ClientWriter.VerifyClient();

	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientWriter.Clean();
		InitClientWriter();

		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}
	} else HANDLE_HR();

	hr = m_ClientReader.VerifyClient();

	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientReader.Clean();
		InitClientReader();

		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
		}
	} else HANDLE_HR();
}

void CDXAudioDuplexStream::ImplProcess() {
	HRESULT hr = S_OK;
	UINT InputBufferSize = (UINT)(ceil((DOUBLE)(m_ClientReader.GetPeriodFrames()) * m_ClientReader.GetRatio()));
	FLOAT* InputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * InputBufferSize));
	UINT FramesRead = 0;

	hr = m_ClientReader.Read(InputBuffer, InputBufferSize, FramesRead);

	if (FAILED(hr)) {
		HANDLE_HR();
	}

	FLOAT* OutputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * FramesRead));

	m_ReadWriteCallback->Process(m_SampleRate, InputBuffer, OutputBuffer, FramesRead);

	hr = m_ClientWriter.Write(OutputBuffer, FramesRead);

	HANDLE_HR();
}

void CDXAudioDuplexStream::InitClientReader() {
	HRESULT hr = m_ClientReader.Initialize (
		false,
		m_SampleRate,
		GetWaitEvent(),
		m_InputDevice
	); HANDLE_HR();
}

void CDXAudioDuplexStream::InitClientWriter() {
	HRESULT hr = m_ClientWriter.Initialize (
		m_SampleRate,
		NULL,
		m_OutputDevice
	); HANDLE_HR();
}

HRESULT CDXAudioDuplexStream::HandleHR(HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		return E_FAIL;
	} else {
		m_ReadWriteCallback->OnObjectFailure(hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}