#pragma once

#include <Windows.h>
#include <comdef.h>

/* The resampler interface.  This exposes functionality courtesy of secret rabbit code. */
struct __declspec(uuid("4170135b-1f5b-4a32-9e22-08de2626b5f7")) IDXAudioResampler : public IUnknown {
	/* Resamples the data.  [InBuffer] is a pointer to the input buffer, and [InBufferFrames] is the number
	** of stereo floating-point samples in this buffer.  [OutBuffer] is the pointer to the output buffer,
	** and [OutBufferFrames] is the number of frames available in the buffer.  You can set this to a number
	** higher than the expected number of received samples - in fact, you should by one sample.  Finally, [Ratio]
	** refers to the ratio of the output sample rate over the input sample rate.  */
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

/* Creates the resampler object. */
HRESULT _DXAUDIO_EXPORT_TAG DXAudioCreateResampler(IDXAudioResampler** ppDXAudioResampler);