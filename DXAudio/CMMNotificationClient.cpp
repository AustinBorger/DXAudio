#include <atlbase.h>
#include <initguid.h>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

#include "CMMNotificationClient.h"
#include "QueryInterface.h"

#define CHECK_HR() if(FAILED(hr)) return hr

CMMNotificationClient::CMMNotificationClient(CMMNotificationClientListener& Listener) :
m_RefCount(1),
m_Listener(Listener)
{ }

CMMNotificationClient::~CMMNotificationClient() { }

HRESULT CMMNotificationClient::QueryInterface(REFIID riid, void** ppvObject) {
	QUERY_INTERFACE_CAST(IMMNotificationClient);
	QUERY_INTERFACE_CAST(IUnknown);
	QUERY_INTERFACE_FAIL();
}

ULONG CMMNotificationClient::AddRef() {
	return InterlockedIncrement(&m_RefCount);
}

ULONG CMMNotificationClient::Release() {
	return InterlockedDecrement(&m_RefCount);
}

HRESULT CMMNotificationClient::OnDefaultDeviceChanged(EDataFlow Flow, ERole Role, LPCWSTR DeviceID) {
	m_Listener.OnDefaultDeviceChanged();

	return S_OK;
}

HRESULT CMMNotificationClient::OnPropertyValueChanged(LPCWSTR DeviceID, const PROPERTYKEY Key) {
	m_Listener.OnPropertyValueChanged();

	return S_OK;
}