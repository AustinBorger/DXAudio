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

#pragma once

#include "DXAudio.h"
#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "CMMNotificationClientListener.h"
#include "QueryInterface.h"

/* This is the base class for all streams - it handles threading issues */
class CDXAudioStream abstract : public IDXAudioStream, public CMMNotificationClientListener {
public:
	CDXAudioStream();

	virtual ~CDXAudioStream();

	//IUnknown methods

	ULONG STDMETHODCALLTYPE AddRef() {
		return ++m_RefCount;
	}

	ULONG STDMETHODCALLTYPE Release() {
		m_RefCount--;

		if (m_RefCount <= 0) {
			delete this; //this can be implemented here, since the destructor is virtual
			return 0;
		}

		return m_RefCount;
	}

	/* Calling this will exit the thread gracefully */
	/* This must be the first thing called in the destructor of the child class, before WaitForThread() */
	VOID Halt() {
		SetEvent(m_HaltEvent);
	}

protected:
	/* Initializes the thread - must be called by child class in its Initialize() method */
	HRESULT Initialize(CComPtr<IDXAudioCallback> Callback);

	/* Returns a handle to the event used for waking the thread each device period */
	HANDLE GetWaitEvent() {
		return m_WaitEvent;
	}

	/* Calling this will wait for the thread to die */
	/* This must be the second thing called in the destructor of the child class, after Halt() */
	VOID WaitForThread() {
		WaitForSingleObject(m_Thread, INFINITE);
	}

	//To be implemented

	/* Child class must initialize the stream objects */
	virtual VOID ImplInitialize() PURE;

	/* Child class must call Start() on the client */
	virtual VOID ImplStart() PURE;
	
	/* Child class must call Stop() on the client */
	virtual VOID ImplStop() PURE;

	/* Child class must check to see if their device is no longer default and re-initialize */
	virtual VOID ImplDeviceChange() PURE;

	/* Child class must check if their client is invalidated and re-initialize */
	virtual VOID ImplPropertyChange() PURE;

	/* Child class must read/write stream data and call their callback's process method */
	virtual VOID ImplProcess() PURE;

	CComPtr<IMMDeviceEnumerator> m_Enumerator; //The WASAPI device enumerator
	FLOAT m_SampleRate; //The sample rate requested by the application - input/output will be resampled to this

private:
	long m_RefCount; //Reference counter

	HANDLE m_StartEvent; //Used as a message for starting the stream
	HANDLE m_StopEvent; //Used as a message for stopping the stream
	HANDLE m_DeviceChangeEvent; //Used as a message for checking for default device changes
	HANDLE m_PropertyChangeEvent; //Used as a message for checking for property value changes
	HANDLE m_WaitEvent; //Used as the callback event for WASAPI
	HANDLE m_HaltEvent; //Used for closing the thread/stream

	HANDLE m_Thread; //Handle to the thread (one thread for each stream)

	CComPtr<IDXAudioCallback> m_Callback; //Used for error reporting

	//IUnknown methods

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) final {
		QUERY_INTERFACE_CAST(IDXAudioStream);
		QUERY_INTERFACE_CAST(IUnknown);
		QUERY_INTERFACE_FAIL();
	}

	//IDXAudioStream methods

	/* Sets the start event, eventually causing the stream to start */
	VOID STDMETHODCALLTYPE Start() final {
		SetEvent(m_StartEvent);
	}

	/* Sets the stop event, eventually causing the stream to stop */
	VOID STDMETHODCALLTYPE Stop() final {
		SetEvent(m_StopEvent);
	}

	/* Returns the sample rate of the stream */
	FLOAT STDMETHODCALLTYPE GetSampleRate() final {
		return m_SampleRate;
	}

	//CMMNotificationClientListener methods

	/* Called when the user changes the default device for any data flow or role */
	virtual VOID OnDefaultDeviceChanged() final {
		SetEvent(m_DeviceChangeEvent);
	}

	/* Called when the user changes properties such as sample rate on an endpoint */
	virtual VOID OnPropertyValueChanged() final {
		SetEvent(m_PropertyChangeEvent);
	}

	/* The static thread entry point */
	static DWORD __stdcall StaticStreamThreadEntry(LPVOID Data);

	/* The non-static thread entry point, called by StaticStreamThreadEntry() */
	DWORD StreamThreadEntry();
};