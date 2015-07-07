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

#include "DXAudio.h"
#include "QueryInterface.h"
#include <atlbase.h>
#include <iostream>
#include <math.h>

#include "Read.hpp"
#include "Write.hpp"
#include "ReadWrite.hpp"

#pragma comment(lib, "DXAudio.lib")

void CheckHR(HRESULT hr) {
	if (FAILED(hr)) {
		_com_error e(hr);
		MessageBoxW(NULL, e.ErrorMessage(), L"Main failure", MB_ICONERROR | MB_OK);
		ExitProcess(hr);
	}
}

char promptUser() {
	std::cout << "What kind of stream should we test?\n";
	std::cout << "q: Output" << std::endl;
	std::cout << "w: Input" << std::endl;
	std::cout << "e: Loopback" << std::endl;
	std::cout << "r: Duplex" << std::endl;
	std::cout << "t: Echo" << std::endl << std::endl;
	std::cout << "Any other character to exit." << std::endl << std::endl;

	char c;

	std::cin >> c;
	std::cout << std::endl;

	return c;
}

int main() {
	while (true) {
		char c = promptUser();

		DXAUDIO_STREAM_TYPE Type;

		if (c == 'q') Type = DXAUDIO_STREAM_TYPE_OUTPUT;
		else if (c == 'w') Type = DXAUDIO_STREAM_TYPE_INPUT;
		else if (c == 'e') Type = DXAUDIO_STREAM_TYPE_LOOPBACK;
		else if (c == 'r') Type = DXAUDIO_STREAM_TYPE_DUPLEX;
		else if (c == 't') Type = DXAUDIO_STREAM_TYPE_ECHO;
		else break;

		CComPtr<IDXAudioStream> Stream;

		DXAUDIO_STREAM_DESC Desc;

		Desc.SampleRate = 22050.0f;
		Desc.Type = Type;

		if (Type == DXAUDIO_STREAM_TYPE_OUTPUT) {
			Write x;

			HRESULT hr = DXAudioCreateStream(&Desc, &x, &Stream);

			CheckHR(hr);

			Stream->Start();

			system("pause");

			Stream->Stop();

			system("cls");
		} else if (Type == DXAUDIO_STREAM_TYPE_INPUT || Type == DXAUDIO_STREAM_TYPE_LOOPBACK) {
			Read x;

			HRESULT hr = DXAudioCreateStream(&Desc, &x, &Stream);

			CheckHR(hr);

			Stream->Start();

			system("pause");

			Stream->Stop();

			system("cls");
		} else {
			ReadWrite x;

			HRESULT hr = DXAudioCreateStream(&Desc, &x, &Stream);

			CheckHR(hr);

			Stream->Start();

			system("pause");

			Stream->Stop();

			system("cls");
		}
	}

	return 0;
}