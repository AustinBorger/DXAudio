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