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