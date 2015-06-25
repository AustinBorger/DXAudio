#pragma once

#include "DXAudio.h"
#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "CDXAudioStream.h"
#include "ClientReader.h"
#include "ClientWriter.h"

class CDXAudioLoopbackStream : public CDXAudioStream {
public:
	CDXAudioLoopbackStream();

	~CDXAudioLoopbackStream();

	//IDXAudioStream methods

	DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() final {
		return DXAUDIO_STREAM_TYPE_LOOPBACK;
	}

	//CDXAudioStream methods

	void ImplInitialize() final;

	void ImplStart() final;

	void ImplStop() final;

	void ImplDeviceChange() final;

	void ImplPropertyChange() final;

	void ImplProcess() final;

	//New methods

	HRESULT Initialize(FLOAT SampleRate, IDXAudioCallback* pDXAudioCallback);

private:
	CComPtr<IMMDevice> m_OutputDevice;
	CComPtr<IDXAudioReadCallback> m_ReadCallback;
	LPWSTR m_DeviceID;
	ClientReader m_ClientReader;
	ClientWriter m_ClientWriter;
	bool m_Running;

	void InitClientReader();

	void InitClientWriter();

	HRESULT HandleHR(HRESULT hr);
};