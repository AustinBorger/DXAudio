#pragma once

#include <comdef.h>

enum DXAUDIO_STREAM_TYPE {
	DXAUDIO_STREAM_TYPE_OUTPUT = 1,
	DXAUDIO_STREAM_TYPE_INPUT,
	DXAUDIO_STREAM_TYPE_LOOPBACK,
	DXAUDIO_STREAM_TYPE_DUPLEX,
	DXAUDIO_STREAM_TYPE_ECHO
};

struct DXAUDIO_STREAM_DESC {
	FLOAT SampleRate; //Sample rate of the stream
	DXAUDIO_STREAM_TYPE Type;
};

struct __declspec(uuid("58127943-2ecc-4e74-845b-e4933263a880")) IDXAudioStream : public IUnknown {
	virtual VOID STDMETHODCALLTYPE Start() PURE;

	virtual VOID STDMETHODCALLTYPE Stop() PURE;

	virtual FLOAT STDMETHODCALLTYPE GetSampleRate() PURE;

	virtual DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() PURE;
};

struct __declspec(uuid("b19d3575-b174-409c-9a27-1b8bf5d938d4")) IDXAudioCallback : public IUnknown {
	virtual VOID STDMETHODCALLTYPE OnObjectFailure(HRESULT hr) PURE;
};

struct __declspec(uuid("63366a5b-5a66-43bf-8d3b-36421d4036d3")) IDXAudioReadCallback : public IDXAudioCallback {
	virtual VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* AudioIn, UINT Frames) PURE;
};

struct __declspec(uuid("34ae23e3-6e51-4c41-86dd-37d0461ac6ae")) IDXAudioWriteCallback : public IDXAudioCallback {
	virtual VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* AudioOut, UINT Frames) PURE;
};

struct __declspec(uuid("857d0781-1b48-4494-b829-24f3b731ff6b")) IDXAudioReadWriteCallback : public IDXAudioCallback {
	virtual VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* AudioIn, FLOAT* AudioOut, UINT Frames) PURE;
};

#ifdef _DXAUDIO_DLL_PROJECT
#define _DXAUDIO_EXPORT_TAG __declspec(dllexport)
#else
#define _DXAUDIO_EXPORT_TAG __declspec(dllimport)
#endif

extern "C" HRESULT _DXAUDIO_EXPORT_TAG DXAudioCreateStream (
	const DXAUDIO_STREAM_DESC* pDesc,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
);