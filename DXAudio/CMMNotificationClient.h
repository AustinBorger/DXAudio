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