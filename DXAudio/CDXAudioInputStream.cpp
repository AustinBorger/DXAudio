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

#include "CDXAudioInputStream.h"
#include <math.h>

#define FILENAME L"CDXAudioInputStream.cpp"
#define HANDLE_HR(Line) if (FAILED(HandleHR(Line, hr))) return
#define HALT_HR() if (FAILED(hr) && hr != AUDCLNT_E_DEVICE_INVALIDATED) { Halt(); return; }

//Zero out all data
CDXAudioInputStream::CDXAudioInputStream() :
m_ClientReader(*this),
m_DeviceID(nullptr),
m_Running(false)
{ }

//Close the thread before releasing data/COM objects
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

	CComPtr<IDXAudioCallback> Callback = pDXAudioCallback;

	//The callback must implement IDXAudioReadCallback
	hr = Callback->QueryInterface (
		IID_PPV_ARGS(&m_ReadCallback)
	);

	if (FAILED(hr)) {
		Callback->OnObjectFailure (
			FILENAME,
			__LINE__,
			E_INVALIDARG
		); return E_FAIL;
	}

	m_SampleRate = SampleRate;

	//Create the thread (done in CDXAudioStream)
	hr = CDXAudioStream::Initialize(Callback);

	if (FAILED(hr)) return hr;

	return S_OK;
}

VOID CDXAudioInputStream::ImplInitialize() {
	HRESULT hr = S_OK;

	//Get the default audio input device
	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&m_InputDevice
	); HANDLE_HR(__LINE__);

	//Get the id of this device
	hr = m_InputDevice->GetId (
		&m_DeviceID
	); HANDLE_HR(__LINE__);

	//Initialize the client reader
	InitClientReader();
}

//Start the stream
VOID CDXAudioInputStream::ImplStart() {
	m_Running = true;
	m_ClientReader.Start();
}

//Stop the stream
VOID CDXAudioInputStream::ImplStop() {
	m_Running = false;
	m_ClientReader.Stop();
}

VOID CDXAudioInputStream::ImplDeviceChange() {
	HRESULT hr = S_OK;
	CComPtr<IMMDevice> DefaultDevice;
	LPWSTR DefaultDeviceID = nullptr;

	//Get the default audio input device
	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&DefaultDevice
	); HANDLE_HR(__LINE__);

	//Get the id of this device
	hr = DefaultDevice->GetId (
		&DefaultDeviceID
	); HANDLE_HR(__LINE__);

	//Check to see if the id of the default device is the same as ours
	if (wcscmp(DefaultDeviceID, m_DeviceID) != 0) {
		//If it isn't, we don't have the default device. Release everything.
		m_ClientReader.Clean();
		m_InputDevice.Release();
		CoTaskMemFree(m_DeviceID);

		//Set our device and id to the new default one
		m_InputDevice = DefaultDevice;
		m_DeviceID = DefaultDeviceID;

		//Re-initialize the client reader
		InitClientReader();

		//If the stream was running before, we need to start it up again
		if (m_Running) {
			m_ClientReader.Start();
		}
	}

	//If the default device was on some other role or data flow, release the data we just created
	if (DefaultDeviceID != m_DeviceID) {
		CoTaskMemFree(DefaultDeviceID);
		DefaultDeviceID = nullptr;
	}
}

VOID CDXAudioInputStream::ImplPropertyChange() {
	//This will return AUDCLNT_E_DEVICE_INVALIDATED if we have a stream with outdated properties
	HRESULT hr = m_ClientReader.VerifyClient();

	//If that's the case, we need to get rid of this one and create a new one
	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientReader.Clean();

		InitClientReader();

		//If the stream was running before, we need to start it up again
		if (m_Running) {
			m_ClientReader.Start();
		}
	} else HANDLE_HR(__LINE__);
}

VOID CDXAudioInputStream::ImplProcess() {
	HRESULT hr = S_OK;

	//The number of samples we need to generate corresponds with the application sample rate.
	//m_ClientReader.GetPeriodFrames() returns the frames needed at the endpoint sample rate,
	//so we need to multiply by the resample ratio to get the correct number of frames.
	UINT InputBufferSize = (UINT)(ceil((DOUBLE)(m_ClientReader.GetPeriodFrames()) * m_ClientReader.GetRatio()));
	FLOAT* InputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * InputBufferSize)); //Create the buffer on the stack (_alloca is safe here)
	UINT FramesRead = 0; //Used to find out how many frames were actually read

	//Read the input data from the stream
	m_ClientReader.Read (
		InputBuffer,
		InputBufferSize,
		FramesRead
	);

	//Send that data to the application
	m_ReadCallback->OnProcess (
		m_SampleRate,
		InputBuffer,
		FramesRead
	);
}

//Initialize the client reader
VOID CDXAudioInputStream::InitClientReader() {
	CComPtr<IDXAudioCallback> Callback = m_ReadCallback;

	HRESULT hr = m_ClientReader.Initialize (
		false,
		m_SampleRate,
		GetWaitEvent(),
		m_InputDevice,
		Callback
	); HALT_HR();
}

//Handle bad or good HRESULTS
HRESULT CDXAudioInputStream::HandleHR(UINT Line, HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		return E_FAIL;
	} else {
		//Something bad happened, stop the stream and alert the application
		m_ReadCallback->OnObjectFailure(FILENAME, Line, hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}