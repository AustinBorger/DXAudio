#include "CDXAudioLoopbackStream.h"
#include <math.h>

#define HANDLE_HR() if (FAILED(HandleHR(hr))) return

CDXAudioLoopbackStream::CDXAudioLoopbackStream() :
m_DeviceID(nullptr),
m_Running(false)
{ }

CDXAudioLoopbackStream::~CDXAudioLoopbackStream() {
	Halt();
	WaitForThread();

	if (m_DeviceID != nullptr) {
		CoTaskMemFree(m_DeviceID);
		m_DeviceID = nullptr;
	}
}

HRESULT CDXAudioLoopbackStream::Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback) {
	HRESULT hr = S_OK;

	hr = pDXAudioCallback->QueryInterface (
		IID_PPV_ARGS(&m_ReadCallback)
	);

	if (FAILED(hr)) return E_INVALIDARG;

	m_SampleRate = SampleRate;

	hr = CDXAudioStream::Initialize();

	if (FAILED(hr)) return E_FAIL;

	return S_OK;
}

void CDXAudioLoopbackStream::ImplInitialize() {
	HRESULT hr = S_OK;

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&m_OutputDevice
	); HANDLE_HR();

	hr = m_OutputDevice->GetId (
		&m_DeviceID
	); HANDLE_HR();

	InitClientReader();

	InitClientWriter();
}

void CDXAudioLoopbackStream::ImplStart() {
	m_Running = true;
	HRESULT hr = m_ClientReader.Start();
	HANDLE_HR();
	hr = m_ClientWriter.Start();
	HANDLE_HR();
}

void CDXAudioLoopbackStream::ImplStop() {
	m_Running = false;
	HRESULT hr = m_ClientReader.Stop();
	HANDLE_HR();
	hr = m_ClientWriter.Stop();
	HANDLE_HR();
}

void CDXAudioLoopbackStream::ImplDeviceChange() {
	HRESULT hr = S_OK;
	CComPtr<IMMDevice> DefaultDevice;
	LPWSTR DefaultDeviceID = nullptr;

	hr = m_Enumerator->GetDefaultAudioEndpoint (
		eRender,
		eConsole,
		&DefaultDevice
	); HANDLE_HR();

	hr = DefaultDevice->GetId (
		&DefaultDeviceID
	); HANDLE_HR();

	if (wcscmp(DefaultDeviceID, m_DeviceID) != 0) {
		m_ClientReader.Clean();
		m_ClientWriter.Clean();
		m_OutputDevice.Release();
		CoTaskMemFree(m_DeviceID);

		m_OutputDevice = DefaultDevice;
		m_DeviceID = DefaultDeviceID;

		InitClientReader();

		InitClientWriter();

		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}
	}

	if (DefaultDeviceID != m_DeviceID) {
		CoTaskMemFree(DefaultDeviceID);
		DefaultDeviceID = nullptr;
	}
}

void CDXAudioLoopbackStream::ImplPropertyChange() {
	HRESULT hr = m_ClientWriter.VerifyClient();

	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientWriter.Clean();
		InitClientWriter();

		if (m_Running) {
			hr = m_ClientWriter.Start();
			HANDLE_HR();
		}
	} else HANDLE_HR();

	hr = m_ClientReader.VerifyClient();

	if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		m_ClientReader.Clean();
		InitClientReader();

		if (m_Running) {
			hr = m_ClientReader.Start();
			HANDLE_HR();
		}
	} else HANDLE_HR();
}

void CDXAudioLoopbackStream::ImplProcess() {
	HRESULT hr = S_OK;
	UINT InputBufferSize = (UINT)(ceil((DOUBLE)(m_ClientReader.GetPeriodFrames()) * m_ClientReader.GetRatio()));
	FLOAT* InputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * InputBufferSize));
	UINT FramesRead = 0;

	hr = m_ClientReader.Read(InputBuffer, InputBufferSize, FramesRead);

	if (FAILED(hr)) {
		HANDLE_HR();
	}

	m_ReadCallback->Process(m_SampleRate, InputBuffer, FramesRead);

	FLOAT* OutputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * FramesRead));
	ZeroMemory(OutputBuffer, sizeof(FLOAT) * 2 * FramesRead);

	hr = m_ClientWriter.Write(OutputBuffer, FramesRead);

	HANDLE_HR();
}

void CDXAudioLoopbackStream::InitClientReader() {
	HRESULT hr = m_ClientReader.Initialize (
		true,
		m_SampleRate,
		NULL,
		m_OutputDevice
	); HANDLE_HR();
}

void CDXAudioLoopbackStream::InitClientWriter() {
	HRESULT hr = m_ClientWriter.Initialize (
		m_SampleRate,
		GetWaitEvent(),
		m_OutputDevice
	); HANDLE_HR();
}

HRESULT CDXAudioLoopbackStream::HandleHR(HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		return E_FAIL;
	} else {
		m_ReadCallback->OnObjectFailure(hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}