#pragma once

#include <Windows.h>
#include <comdef.h>

struct __declspec(uuid("4170135b-1f5b-4a32-9e22-08de2626b5f7")) IDXAudioResampler : public IUnknown {
	virtual VOID STDMETHODCALLTYPE Process (
		FLOAT* InBuffer,
		UINT InBufferFrames,
		FLOAT* OutBuffer,
		UINT OutBufferFrames,
		DOUBLE Ratio
	) PURE;
};

#ifndef _DXAUDIO_EXPORT_TAG
	#ifdef _DXAUDIO_DLL_PROJECT
		#define _DXAUDIO_EXPORT_TAG __declspec(dllexport)
	#else
		#define _DXAUDIO_EXPORT_TAG __declspec(dllimport)
	#endif
#endif

HRESULT _DXAUDIO_EXPORT_TAG DXAudioCreateResampler(IDXAudioResampler** ppDXAudioResampler);