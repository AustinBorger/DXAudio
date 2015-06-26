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

#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>

/* Interface used by CMMNotificationClient to notify changes in the default device
** and the properties of audio endpoints.
** The reason no data is used in these callbacks is because the calls are handled on a separate
** thread via events, and a mutex would be overkill for something we can figure out without the data. */
struct CMMNotificationClientListener {
	/* Called when the default device is changed on some flow or role - must be implemented */
	virtual void OnDefaultDeviceChanged() PURE;

	/* Called when a property of an endpoint has changed - must be implemented */
	virtual void OnPropertyValueChanged() PURE;
};