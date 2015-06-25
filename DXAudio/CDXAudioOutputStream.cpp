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

#include "CDXAudioOutputStream.h"
#include <math.h>

#define HANDLE_HR() if (FAILED(HandleHR(hr))) return

CDXAudioOutputStream::CDXAudioOutputStream() :
m_DeviceID(nullptr),
m_SamplesNeeded(0.0),
m_Running(false)
{ }

CDXAudioOutputStream::~CDXAudioOutputStream() {
	Halt();
	WaitForThread();

	if (m_DeviceID != nullptr) {
		CoTaskMemFree(m_DeviceID);
		m_DeviceID = nullptr;
	}
}

HRESULT CDXAudioOutputStream::Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback) {
	HRESULT hr = S_OK;

	hr = pDXAudioCallback->QueryInterface (
		IID_PPV_ARGS(&m_WriteCallback)
	);

	if (FAILED(hr)) return E_INVALIDARG;

	m_SampleRate = SampleRate;

	hr = CDXAudioStream::Initialize();

	if (FAILED(hr)) return E_FAIL;

	return S_OK;
}

void CDXAudioOutputStream::ImplInitialize() {
	HRESULT hr = S_OK;

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&m_OutputDevice
	); HANDLE_HR();

	hr = m_OutputDevice->GetId (
		&m_DeviceID
	); HANDLE_HR();

	InitClientWriter();
}

void CDXAudioOutputStream::ImplStart() {
	m_Running = true;
	HRESULT hr = m_ClientWriter.Start();
	HANDLE_HR();
}

void CDXAudioOutputStream::ImplStop() {
	m_Running = false;
	HRESULT hr = m_ClientWriter.Stop();
	HANDLE_HR();
}

void CDXAudioOutputStream::ImplDeviceChange() {
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

	if (wcscmp(DefaultDeviceID, m_DeviceID) != 0) {
		m_ClientWriter.Clean();
		m_OutputDevice.Release();
		CoTaskMemFree(m_DeviceID);

		m_OutputDevice = DefaultDevice;
		m_DeviceID = DefaultDeviceID;

		InitClientWriter();

		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}

		m_SamplesNeeded = 0.0;
	}

	if (DefaultDeviceID != m_DeviceID) {
		CoTaskMemFree(DefaultDeviceID);
		DefaultDeviceID = nullptr;
	}
}

void CDXAudioOutputStream::ImplPropertyChange() {
	HRESULT hr = m_ClientWriter.VerifyClient();

	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientWriter.Clean();

		InitClientWriter();

		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}

		m_SamplesNeeded = 0.0;
	} else HANDLE_HR();
}

void CDXAudioOutputStream::ImplProcess() {
	HRESULT hr = S_OK;
	m_SamplesNeeded += DOUBLE(m_ClientWriter.GetPeriodFrames()) / m_ClientWriter.GetRatio();
	const UINT SamplesGen = (UINT)(ceil(m_SamplesNeeded));
	FLOAT* OutputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * SamplesGen));

	m_WriteCallback->Process (
		m_SampleRate,
		OutputBuffer,
		SamplesGen
	);

	hr = m_ClientWriter.Write (
		OutputBuffer,
		SamplesGen
	); HANDLE_HR();

	m_SamplesNeeded -= SamplesGen;
}

void CDXAudioOutputStream::InitClientWriter() {
	HRESULT hr = m_ClientWriter.Initialize (
		m_SampleRate,
		GetWaitEvent(),
		m_OutputDevice
	); HANDLE_HR();
}

HRESULT CDXAudioOutputStream::HandleHR(HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		ImplPropertyChange();
		return E_FAIL;
	} else {
		m_WriteCallback->OnObjectFailure(hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}