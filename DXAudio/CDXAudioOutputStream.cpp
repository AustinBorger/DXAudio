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

//Zero out all data
CDXAudioOutputStream::CDXAudioOutputStream() :
m_DeviceID(nullptr),
m_SamplesNeeded(0.0),
m_Running(false)
{ }

//Close the thread before releasing data/COM objects
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

	//The callback must implement IDXAudioWriteCallback
	hr = pDXAudioCallback->QueryInterface (
		IID_PPV_ARGS(&m_WriteCallback)
	);

	if (FAILED(hr)) return E_INVALIDARG;

	m_SampleRate = SampleRate;

	//Create the thread (done in CDXAudioStream)
	hr = CDXAudioStream::Initialize();

	if (FAILED(hr)) return E_FAIL;

	return S_OK;
}

void CDXAudioOutputStream::ImplInitialize() {
	HRESULT hr = S_OK;

	//Get the default audio output device
	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&m_OutputDevice
	); HANDLE_HR();

	//Get the id of this device
	hr = m_OutputDevice->GetId (
		&m_DeviceID
	); HANDLE_HR();

	//Initialize the client writer
	InitClientWriter();
}

//Start the stream
void CDXAudioOutputStream::ImplStart() {
	m_Running = true;
	HRESULT hr = m_ClientWriter.Start();
	HANDLE_HR();
}

//Stop the stream
void CDXAudioOutputStream::ImplStop() {
	m_Running = false;
	HRESULT hr = m_ClientWriter.Stop();
	HANDLE_HR();
}

void CDXAudioOutputStream::ImplDeviceChange() {
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
	if (wcscmp(DefaultDeviceID, m_DeviceID) != 0) {
		//If it isn't, we don't have the default device. Release everything.
		m_ClientWriter.Clean();
		m_OutputDevice.Release();
		CoTaskMemFree(m_DeviceID);

		//Set our device and id to the new default one
		m_OutputDevice = DefaultDevice;
		m_DeviceID = DefaultDeviceID;

		//Re-initialize the client writer
		InitClientWriter();

		//If the stream was running before, we need to start it up again
		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}

		//Reset decimal sample counter
		m_SamplesNeeded = 0.0;
	}

	//If the default device was on some other role or data flow, release the data we just created
	if (DefaultDeviceID != m_DeviceID) {
		CoTaskMemFree(DefaultDeviceID);
		DefaultDeviceID = nullptr;
	}
}

void CDXAudioOutputStream::ImplPropertyChange() {
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

		//Reset decimal sample counter
		m_SamplesNeeded = 0.0;
	} else HANDLE_HR();
}

void CDXAudioOutputStream::ImplProcess() {
	HRESULT hr = S_OK;

	//The number of samples we need to generate corresponds with the application sample rate.
	//m_ClientWriter.GetPeriodFrames() returns the frames needed at the endpoint sample rate,
	//so we need to divide by the resample ratio to get the correct number of frames.
	m_SamplesNeeded += DOUBLE(m_ClientWriter.GetPeriodFrames()) / m_ClientWriter.GetRatio();
	const UINT SamplesGen = (UINT)(ceil(m_SamplesNeeded)); //We'll generate an integral number of samples
	FLOAT* OutputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * SamplesGen)); //Create the buffer on the stack (_alloca is safe here)

	//Get the application to generate new output data
	m_WriteCallback->Process (
		m_SampleRate,
		OutputBuffer,
		SamplesGen
	);

	//Write that output data to the stream
	hr = m_ClientWriter.Write (
		OutputBuffer,
		SamplesGen
	); HANDLE_HR();

	//Subtract the integral number of samples from the decimal number of samples.
	//If there is a remainder > 0.5, it is used in the next period to generate an extra
	//sample.  This keeps the stream from running out of padding - if we ignored
	//the extra decimal, eventually we wouldn't be giving the stream enough samples.
	//When this happens, we'll hear an annoying buzz and the stream will be temporally
	//behind.
	m_SamplesNeeded -= SamplesGen;
}

//Initialize the client writer
void CDXAudioOutputStream::InitClientWriter() {
	HRESULT hr = m_ClientWriter.Initialize (
		m_SampleRate,
		GetWaitEvent(),
		m_OutputDevice
	); HANDLE_HR();
}

//Handle bad or good HRESULTS
HRESULT CDXAudioOutputStream::HandleHR(HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		return E_FAIL;
	} else {
		//Something bad happened, stop the stream and alert the application
		m_WriteCallback->OnObjectFailure(hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}