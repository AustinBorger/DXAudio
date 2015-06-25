#pragma once

#include "DXAudio.h"
#include <mmdeviceapi.h>
#include <atlbase.h>
#include <vector>

#include "CMMNotificationClientListener.h"

class CDXAudioFactory;

class CMMNotificationClient : public IMMNotificationClient {
public:
	CMMNotificationClient(CMMNotificationClientListener& Listener);

	~CMMNotificationClient();

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) final;

	ULONG STDMETHODCALLTYPE AddRef();

	ULONG STDMETHODCALLTYPE Release();

	STDMETHODIMP OnDefaultDeviceChanged(EDataFlow Flow, ERole Role, LPCWSTR DeviceID) final;

	STDMETHODIMP OnDeviceAdded(LPCWSTR DeviceID) final { return S_OK; }

	STDMETHODIMP OnDeviceRemoved(LPCWSTR DeviceID) final { return S_OK; }

	STDMETHODIMP OnDeviceStateChanged(LPCWSTR DeviceID, DWORD State) final { return S_OK; }

	STDMETHODIMP OnPropertyValueChanged(LPCWSTR DeviceID, const PROPERTYKEY Key) final;

private:
	long m_RefCount;

	CMMNotificationClientListener& m_Listener;
};