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

#include "CDXAudioStream.h"
#include "CMMNotificationClient.h"

#define EVENT_INIT(x) x = CreateEventW(NULL, FALSE, FALSE, NULL); if (x == NULL) { return HRESULT_FROM_WIN32(GetLastError()); }
#define EVENT_CLEANUP(x) if (x != NULL) { CloseHandle(x); x = NULL; }
#define CHECK_HR() if (FAILED(hr)) return hr

CDXAudioStream::CDXAudioStream() :
m_RefCount(1),
m_StartEvent(NULL),
m_StopEvent(NULL),
m_DeviceChangeEvent(NULL),
m_PropertyChangeEvent(NULL),
m_WaitEvent(NULL),
m_HaltEvent(NULL),
m_Thread(NULL)
{ }

CDXAudioStream::~CDXAudioStream() {
	//Expects thread to be halted by child class

	if (m_Thread != NULL) {
		CloseHandle(m_Thread);
		m_Thread = NULL;
	}

	EVENT_CLEANUP(m_StartEvent);
	EVENT_CLEANUP(m_StopEvent);
	EVENT_CLEANUP(m_DeviceChangeEvent);
	EVENT_CLEANUP(m_PropertyChangeEvent);
	EVENT_CLEANUP(m_WaitEvent);
	EVENT_CLEANUP(m_HaltEvent);
}

HRESULT CDXAudioStream::Initialize() {
	EVENT_INIT(m_StartEvent);
	EVENT_INIT(m_StopEvent);
	EVENT_INIT(m_DeviceChangeEvent);
	EVENT_INIT(m_PropertyChangeEvent);
	EVENT_INIT(m_WaitEvent);
	EVENT_INIT(m_HaltEvent);

	//Everything will happen on a separate thread
	m_Thread = CreateThread (
		NULL,
		0,
		StaticStreamThreadEntry,
		this,
		NULL,
		NULL
	);

	//If m_Thread is NULL, an error occurred
	if (m_Thread == NULL) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

DWORD __stdcall CDXAudioStream::StaticStreamThreadEntry(LPVOID Data) {
	CDXAudioStream* l_Stream = reinterpret_cast<CDXAudioStream*>(Data);

	return l_Stream->StreamThreadEntry();
}

DWORD CDXAudioStream::StreamThreadEntry() {
	bool run = true;
	DWORD dwResult = 0;
	HRESULT hr = S_OK;
	HANDLE Events[] = {
		m_StartEvent,
		m_StopEvent,
		m_DeviceChangeEvent,
		m_PropertyChangeEvent,
		m_WaitEvent,
		m_HaltEvent
	};

	bool StreamRunning = false;

	static const DWORD SM_START = WAIT_OBJECT_0;
	static const DWORD SM_STOP = WAIT_OBJECT_0 + 1;
	static const DWORD SM_DEVICECHANGE = WAIT_OBJECT_0 + 2;
	static const DWORD SM_PROPERTYCHANGE = WAIT_OBJECT_0 + 3;
	static const DWORD SM_PROCESS = WAIT_OBJECT_0 + 4;
	static const DWORD SM_CLOSE = WAIT_OBJECT_0 + 5;

	static const UINT nEvents = sizeof(Events) / sizeof(HANDLE);

	//Initialize the COM server
	hr = CoInitializeEx (
		NULL,
		COINIT_SPEED_OVER_MEMORY |
		COINIT_APARTMENTTHREADED
	); CHECK_HR();

	//Create the device enumerator
	hr = CoCreateInstance (
		__uuidof(MMDeviceEnumerator),
		NULL,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)(&m_Enumerator)
	); CHECK_HR();

	CMMNotificationClient NotificationClient(*this);

	//Register the callback for default device / property changes
	hr = m_Enumerator->RegisterEndpointNotificationCallback (
		&NotificationClient
	); CHECK_HR();

	//Initialize the child object
	ImplInitialize();

	hr = S_OK;

	while (run) {
		//Wait for a message (event)
		dwResult = WaitForMultipleObjectsEx (
			nEvents,
			Events,
			FALSE,
			INFINITE,
			FALSE
		);

		switch (dwResult) {
			case SM_PROCESS: { //Process
				ImplProcess();
			} break;

			case SM_START: { //Start the stream
				ImplStart();
			} break;

			case SM_STOP: { //Stop the stream
				ImplStop();
			} break;

			case SM_DEVICECHANGE: { //The default device has changed somewhere
				ImplDeviceChange();
			} break;

			case SM_PROPERTYCHANGE: { //A property has changed somewhere
				ImplPropertyChange();
			} break;

			case SM_CLOSE: { //Close the stream
				run = false;
			} break;

			default: { //Error occurred
				run = false;
				hr = E_FAIL;
			} break;
		}
	}

	// Prevent any more notifications
	m_Enumerator->UnregisterEndpointNotificationCallback (
		&NotificationClient
	);

	return hr;
}