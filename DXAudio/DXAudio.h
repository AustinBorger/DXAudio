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

/* DXAUDIO_STREAM_TYPE is used to determine the type of stream that will be created by DXAudioCreateStream */
enum DXAUDIO_STREAM_TYPE {
	DXAUDIO_STREAM_TYPE_OUTPUT = 1, //An output stream to the default audio output endpoint
	DXAUDIO_STREAM_TYPE_INPUT,      //An input stream from the default audio input endpoint
	DXAUDIO_STREAM_TYPE_LOOPBACK,   //An input stream from the default audio output endpoint (reads what's currently playing)
	DXAUDIO_STREAM_TYPE_DUPLEX,		//A duplex stream between the default audio input and output endpoints
	DXAUDIO_STREAM_TYPE_ECHO		//A duplex stream between the default audio output endpoint and itself (IE, a loopback stream with output functionality)
};

/* DXAUDIO_STREAM_DESC is used for creating an audio stream to determine its properties */
struct DXAUDIO_STREAM_DESC {
	FLOAT SampleRate; //Sample rate of the stream
	DXAUDIO_STREAM_TYPE Type; //Type of the stream to be created (see enum above)
};

/* IDXAudioStream is the interface for all DXAudio streams. */
struct __declspec(uuid("58127943-2ecc-4e74-845b-e4933263a880")) IDXAudioStream : public IUnknown {
	/* Start() causes the stream to become active.  When this happens, your stream callback will
	** be called repeatedly on a separate thread that is unique to the stream.  Note that the
	** stream is initialized in a stopped state, so this must be called for the stream to
	** begin playing. */
	virtual VOID STDMETHODCALLTYPE Start() PURE;

	/* Stop() pauses the stream until Start() is called a second time. */
	virtual VOID STDMETHODCALLTYPE Stop() PURE;

	/* GetSampleRate() returns the sample rate of the stream.  Note that this value remains
	** constant throughout the lifetime of the stream.  If you want to change the sample
	** rate, you will need to re-create the stream object. */
	virtual FLOAT STDMETHODCALLTYPE GetSampleRate() PURE;

	/* GetStreamType() returns the type of the stream.  Like the sample rate, this is bound
	** to the stream object.  To use a different stream type, you will need to create a
	** different stream object. */
	virtual DXAUDIO_STREAM_TYPE STDMETHODCALLTYPE GetStreamType() PURE;
};

/* IDXAudioCallback is the parent interface for all stream callbacks.   This should not be directly inherited.
** Instead, inherit from either of IDXAudioReadCallback, IDXAudioWriteCallback, or IDXAudioReadWriteCallback. */
struct __declspec(uuid("b19d3575-b174-409c-9a27-1b8bf5d938d4")) IDXAudioCallback : public IUnknown {
	/* OnObjectFailure() is a callback mechanism for error reporting.  If something unexpected happens,
	** or the library is operating out of its expected conditions, then this will be called with the
	** HRESULT that was provided by the Windows API when the error occurred. Note that this must be implemented. */
	virtual VOID STDMETHODCALLTYPE OnObjectFailure(HRESULT hr) PURE;
};

/* IDXAudioReadCallback is the callback interface for input and loopback streams. */
struct __declspec(uuid("63366a5b-5a66-43bf-8d3b-36421d4036d3")) IDXAudioReadCallback : public IDXAudioCallback {
	/* Process() is called once every stream period.  This provides the input data from the default endpoint
	** as it arrives, at the given sample rate.  [Frames] represents the number of floating-point stereo samples
	** available in the [AudioIn] buffer.  Note that this value is likely to frequently change between calls due to
	** the process of resampling the input.  You should write your application to be flexible of this number.
	** Note that this must be implemented. */
	virtual VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* AudioIn, UINT Frames) PURE;
};

/* IDXAudioWriteCallback is the callback interface for output streams. */
struct __declspec(uuid("34ae23e3-6e51-4c41-86dd-37d0461ac6ae")) IDXAudioWriteCallback : public IDXAudioCallback {
	/* Process() is called once every stream period.  This delivers your output data to the default endpoint
	** at the given sample rate.  [Frames] represents the number of floating-point stereo samples you must produce
	** to the [AudioOut] buffer.  Note that this value is likely to frequently change between calls due to
	** the process of resampling the output.  You should write your application to be flexible of this number.
	** Note that this must be implemented. */
	virtual VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* AudioOut, UINT Frames) PURE;
};

/* IDXAudioReadWriteCallback is the callback interface for duplex and echo streams. */
struct __declspec(uuid("857d0781-1b48-4494-b829-24f3b731ff6b")) IDXAudioReadWriteCallback : public IDXAudioCallback {
	/* Process() is called once every stream period.  This retrieves input data from the default input endpoint and
	** delivers your output data to the default output endpoint at the given sample rate.
	** [Frames] represents the number of floating-point stereo samples available in the [AudioIn] buffer, as well as
	** the number of samples you must produce to the [AudioOut] buffer.
	** Note that this value is likely to frequently change between calls due to the process of resampling.
	** You should write your application to be flexible of this number.
	** Note that this must be implemented. */
	virtual VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* AudioIn, FLOAT* AudioOut, UINT Frames) PURE;
};

#ifdef _DXAUDIO_DLL_PROJECT
#define _DXAUDIO_EXPORT_TAG __declspec(dllexport)
#else
#define _DXAUDIO_EXPORT_TAG __declspec(dllimport)
#endif

/* DXAudioCreateStream() is used to create any audio stream.  [ppDXAudioCallback] must inherit from
** one of either IDXAudioReadCallback, IDXAudioWriteCallback, or IDXAudioReadWriteCallback and must
** be the appropriate callback interface for the stream you want to create. */
extern "C" HRESULT _DXAUDIO_EXPORT_TAG DXAudioCreateStream (
	const DXAUDIO_STREAM_DESC* pDesc,
	IDXAudioCallback* pDXAudioCallback,
	IDXAudioStream** ppDXAudioStream
);
