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

#pragma once

#include "DXAudio.h"
#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "CMMNotificationClientListener.h"
#include "QueryInterface.h"

class CDXAudioStream abstract : public IDXAudioStream, public CMMNotificationClientListener {
public:
	CDXAudioStream();

	virtual ~CDXAudioStream();

	ULONG STDMETHODCALLTYPE AddRef() {
		return ++m_RefCount;
	}

	ULONG STDMETHODCALLTYPE Release() {
		m_RefCount--;

		if (m_RefCount <= 0) {
			delete this;
			return 0;
		}

		return m_RefCount;
	}

protected:
	HRESULT Initialize();

	HANDLE GetWaitEvent() {
		return m_WaitEvent;
	}

	void Halt() {
		SetEvent(m_HaltEvent);
	}

	void WaitForThread() {
		WaitForSingleObject(m_Thread, INFINITE);
	}

	//To be implemented

	virtual void ImplInitialize() PURE;

	virtual void ImplStart() PURE;
	
	virtual void ImplStop() PURE;

	virtual void ImplDeviceChange() PURE;

	virtual void ImplPropertyChange() PURE;

	virtual void ImplProcess() PURE;

	CComPtr<IMMDeviceEnumerator> m_Enumerator;
	FLOAT m_SampleRate;

private:
	long m_RefCount;

	HANDLE m_StartEvent;
	HANDLE m_StopEvent;
	HANDLE m_DeviceChangeEvent;
	HANDLE m_PropertyChangeEvent;
	HANDLE m_WaitEvent;
	HANDLE m_HaltEvent;

	HANDLE m_Thread;

	//IUnknown methods

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) final {
		QUERY_INTERFACE_CAST(IDXAudioStream);
		QUERY_INTERFACE_CAST(IUnknown);
		QUERY_INTERFACE_FAIL();
	}

	//IDXAudioStream methods

	VOID STDMETHODCALLTYPE Start() final {
		SetEvent(m_StartEvent);
	}

	VOID STDMETHODCALLTYPE Stop() final {
		SetEvent(m_StopEvent);
	}

	FLOAT STDMETHODCALLTYPE GetSampleRate() final {
		return m_SampleRate;
	}

	//CMMNotificationClientListener methods

	virtual void OnDefaultDeviceChanged() final {
		SetEvent(m_DeviceChangeEvent);
	}

	virtual void OnPropertyValueChanged() final {
		SetEvent(m_PropertyChangeEvent);
	}

	static DWORD __stdcall StaticStreamThreadEntry(LPVOID Data);

	DWORD StreamThreadEntry();
};