#pragma once

#include "DXAudio.h"
#include <comdef.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include "CMMNotificationClientListener.h"
#include "CDXAudioStream.h"
#include "QueryInterface.h"
#include "ClientWriter.h"

class CDXAudioOutputStream : public CDXAudioStream {
public:
	CDXAudioOutputStream();

	virtual ~CDXAudioOutputStream();

	//IDXAudioStream methods

	DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() final {
		return DXAUDIO_STREAM_TYPE_OUTPUT;
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
	CComPtr<IDXAudioWriteCallback> m_WriteCallback;
	LPWSTR m_DeviceID;
	ClientWriter m_ClientWriter;
	DOUBLE m_SamplesNeeded;
	bool m_Running;

	void InitClientWriter();

	HRESULT HandleHR(HRESULT hr);
};