#include "CDXAudioEchoStream.h"
#include <math.h>

#define HANDLE_HR() if (FAILED(HandleHR(hr))) return

CDXAudioEchoStream::CDXAudioEchoStream() :
m_DeviceID(nullptr),
m_Running(false)
{ }

CDXAudioEchoStream::~CDXAudioEchoStream() {
	Halt();
	WaitForThread();

	if (m_DeviceID != nullptr) {
		CoTaskMemFree(m_DeviceID);
		m_DeviceID = nullptr;
	}
}

HRESULT CDXAudioEchoStream::Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback) {
	HRESULT hr = S_OK;

	hr = pDXAudioCallback->QueryInterface (
		IID_PPV_ARGS(&m_ReadWriteCallback)
	);

	if (FAILED(hr)) return E_INVALIDARG;

	m_SampleRate = SampleRate;

	hr = CDXAudioStream::Initialize();

	if (FAILED(hr)) return E_FAIL;

	return S_OK;
}

void CDXAudioEchoStream::ImplInitialize() {
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

void CDXAudioEchoStream::ImplStart() {
	m_Running = true;
	HRESULT hr = m_ClientReader.Start();
	HANDLE_HR();
	hr = m_ClientWriter.Start();
	HANDLE_HR();
}

void CDXAudioEchoStream::ImplStop() {
	m_Running = false;
	HRESULT hr = m_ClientReader.Stop();
	HANDLE_HR();
	hr = m_ClientWriter.Stop();
	HANDLE_HR();
}

void CDXAudioEchoStream::ImplDeviceChange() {
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

void CDXAudioEchoStream::ImplPropertyChange() {
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

void CDXAudioEchoStream::ImplProcess() {
	HRESULT hr = S_OK;
	UINT InputBufferSize = (UINT)(ceil((DOUBLE)(m_ClientReader.GetPeriodFrames()) * m_ClientReader.GetRatio()));
	FLOAT* InputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * InputBufferSize));
	UINT FramesRead = 0;

	hr = m_ClientReader.Read(InputBuffer, InputBufferSize, FramesRead);

	if (FAILED(hr)) {
		HANDLE_HR();
	}

	FLOAT* OutputBuffer = (FLOAT*)(_alloca(sizeof(FLOAT) * 2 * FramesRead));

	m_ReadWriteCallback->Process(m_SampleRate, InputBuffer, OutputBuffer, FramesRead);

	hr = m_ClientWriter.Write(OutputBuffer, FramesRead);

	HANDLE_HR();
}

void CDXAudioEchoStream::InitClientReader() {
	HRESULT hr = m_ClientReader.Initialize (
		true,
		m_SampleRate,
		NULL,
		m_OutputDevice
	); HANDLE_HR();
}

void CDXAudioEchoStream::InitClientWriter() {
	HRESULT hr = m_ClientWriter.Initialize (
		m_SampleRate,
		GetWaitEvent(),
		m_OutputDevice
	); HANDLE_HR();
}

HRESULT CDXAudioEchoStream::HandleHR(HRESULT hr) {
	if (hr == S_OK) {
		//Obviously nothing went wrong
	} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
		//Don't halt, just wait for a call to ImplPropertyChange
		return E_FAIL;
	} else {
		m_ReadWriteCallback->OnObjectFailure(hr);
		Halt();
		return E_FAIL;
	}

	return S_OK;
}