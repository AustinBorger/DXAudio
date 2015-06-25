#pragma once

#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>

struct CMMNotificationClientListener {
	virtual void OnDefaultDeviceChanged() PURE;

	virtual void OnPropertyValueChanged() PURE;
};