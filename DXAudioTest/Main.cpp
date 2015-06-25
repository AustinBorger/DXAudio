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

		Desc.SampleRate = 44100.0f;
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