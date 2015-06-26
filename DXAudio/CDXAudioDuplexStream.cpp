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

//Zero out all data
CDXAudioDuplexStream::CDXAudioDuplexStream() :
m_InputDeviceID(nullptr),
m_OutputDeviceID(nullptr),
m_Running(false)
{ }

//Close the thread before releasing data/COM objects
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

	//The callback must implement IDXAudioReadWriteCallback
	hr = pDXAudioCallback->QueryInterface (
		IID_PPV_ARGS(&m_ReadWriteCallback)
	);

	if (FAILED(hr)) return E_INVALIDARG;

	m_SampleRate = SampleRate;

	//Create the thread (done in CDXAudioStream)
	hr = CDXAudioStream::Initialize();

	if (FAILED(hr)) return E_FAIL;

	return S_OK;
}

void CDXAudioDuplexStream::ImplInitialize() {
	HRESULT hr = S_OK;

	//Get the default audio input device
	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&m_InputDevice
	); HANDLE_HR();

	//Get the id of this device
	hr = m_InputDevice->GetId (
		&m_InputDeviceID
	); HANDLE_HR();

	//Initialize the client reader
	InitClientReader();

	//Get the default audio output device
	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&m_OutputDevice
	); HANDLE_HR();

	//Get the id of this device
	hr = m_OutputDevice->GetId (
		&m_OutputDeviceID
	); HANDLE_HR();

	//Initialize the client writer
	InitClientWriter();
}

//Start the stream(s)
void CDXAudioDuplexStream::ImplStart() {
	m_Running = true;
	HRESULT hr = m_ClientReader.Start();
	HANDLE_HR();
	hr = m_ClientWriter.Start();
	HANDLE_HR();
}

//Stop the stream(s)
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

	//Get the default audio output device
	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&DefaultDevice
	); HANDLE_HR();

	//Get the id of this device
	hr = DefaultDevice->GetId (
		&DefaultDeviceID
	); HANDLE_HR();

	//Check to see if the id of the default device is the same as ours
	if (wcscmp(DefaultDeviceID, m_OutputDeviceID) != 0) {
		//If it isn't, we don't have the default device. Release everything involving the output stream.
		m_ClientWriter.Clean();
		m_OutputDevice.Release();
		CoTaskMemFree(m_OutputDeviceID);

		//Set our device and id to the new default one
		m_OutputDevice = DefaultDevice;
		m_OutputDeviceID = DefaultDeviceID;

		//Re-initialize the client writer
		InitClientWriter();

		//If the stream was running before, we need to start it up again
		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}
	}

	//If the default device was on some other role or data flow, release the data we just created
	if (DefaultDeviceID != m_OutputDeviceID) {
		CoTaskMemFree(DefaultDeviceID);
	}

	DefaultDeviceID = nullptr;

	//Release this com ptr since we're reusing it for checking the input device
	DefaultDevice.Release();

	//Get the default audio input device
	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eCapture,
		eConsole,
		&DefaultDevice
	); HANDLE_HR();

	//Get the id of this device
	hr = DefaultDevice->GetId (
		&DefaultDeviceID
	); HANDLE_HR();

	//Check to see if the id of the default device is the same as ours
	if (wcscmp(DefaultDeviceID, m_InputDeviceID) != 0) {
		//If it isn't, we don't have the default device. Release everything involving the input stream.
		m_ClientReader.Clean();
		m_InputDevice.Release();
		CoTaskMemFree(m_InputDeviceID);

		//Set our device and id to the new default one
		m_InputDevice = DefaultDevice;
		m_InputDeviceID = DefaultDeviceID;

		//Re-initialize the client reader
		InitClientReader();

		//If the stream was running before, we need to start it up again
		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
		}
	}

	//If the default device was on some other role or data flow, release the data we just created
	if (DefaultDeviceID != m_InputDeviceID) {
		CoTaskMemFree(DefaultDeviceID);
		DefaultDeviceID = nullptr;
	}
}

void CDXAudioDuplexStream::ImplPropertyChange() {
	//This will return AUDCLNT_E_DEVICE_INVALIDATED if we have a stream with outdated properties
	HRESULT hr = m_ClientWriter.VerifyClient();

	//If that's the case, we need to get rid of this one and create a new one
	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientWriter.Clean();
		InitClientWriter();

		//If the stream was running before, we need to start it up again
		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}
	} else HANDLE_HR();

	//This will probably fail too, but it doesn't hurt to check
	hr = m_ClientReader.VerifyClient();

	//If it does fail, do the same as above
	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientReader.Clean();
		InitClientReader();

		//If the stream was running before, we need to start it up again
		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
		}
	} else HANDLE_HR();
}

void CDXAudioDuplexStream::ImplProcess() {
	HRESULT hr = S_OK;

	//The number of samples we need to generate corresponds with the application sample rate.
	//m_ClientReader.GetPeriodFrames() returns the frames needed at the endpoint sample rate,
	//so we need to multiply by the resample ratio to get the correct number of frames.
	UINT InputBufferSize = (UINT)(ceil((DOUBLE)(m_ClientReader.GetPeriodFrames()) * m_ClientReader.GetRatio()));
	FLOAT* InputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * InputBufferSize)); //Create the buffer on the stack (_alloca is safe here)
	UINT FramesRead = 0;

	//Read the resampled data from the device
	hr = m_ClientReader.Read (
		InputBuffer,
		InputBufferSize,
		FramesRead
	); HANDLE_HR();

	//Generate an output buffer of equal size to the input buffer, also on the stack
	FLOAT* OutputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * FramesRead));

	//Give the application the input data and tell it to generate output
	m_ReadWriteCallback->Process (
		m_SampleRate,
		InputBuffer,
		OutputBuffer,
		FramesRead
	);

	//Write this data to the stream
	hr = m_ClientWriter.Write (
		OutputBuffer,
		FramesRead
	); HANDLE_HR();
}

//Initialize the client reader
void CDXAudioDuplexStream::InitClientReader() {
	HRESULT hr = m_ClientReader.Initialize (
		false,
		m_SampleRate,
		GetWaitEvent(),
		m_InputDevice
	); HANDLE_HR();
}

//Initialize the client writer
void CDXAudioDuplexStream::InitClientWriter() {
	HRESULT hr = m_ClientWriter.Initialize (
		m_SampleRate,
		NULL,
		m_OutputDevice
	); HANDLE_HR();
}

//Handle bad or good HRESULTS
HRESULT CDXAudioDuplexStream::HandleHR(HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		return E_FAIL;
	} else {
		//Something bad happened, stop the stream and alert the application
		m_ReadWriteCallback->OnObjectFailure(hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}