DXAudio
=========

DXAudio is a small layer over WASAPI written for C++ applications that
simplifies the process of creating an audio stream.  As
a Windows-specific library, it makes use of the COM ABI.
Its intent is to provide a fast, easy way to create a
real-time audio stream for use in games.

What it does.
-------------
DXAudio removes the application developer from the process
of enumerating audio endpoints, interpreting the audio
data format of the endpoint, handling different types of
streams, stream routing, and threading.  It cuts down several hundred
lines of boilerplate code that exists in the lowest level
of most games.

The library makes the assumption that only the default
audio endpoints are important.  As such, the concept of
an endpoint is abstracted away, leaving only the stream.
It also makes the assumption that all audio data will
be in a stereo format, which is the case
for most games.  To address real-time audio processing needs,
all samples are normalized floating-point.  All audio processing is assumed to occur on its own thread,
so DXAudio creates that thread for you, much like other
audio streaming APIs.  DXAudio also reacts to changes in
the default device and its properties, ensuring that
the stream is never interrupted.

What it doesn't do.
-------------------
DXAudio does not attempt to provide a cross-platform way
to create an audio stream.  It is only written for Windows
machines version 7 or later (although it probably works on
Vista too).  There are already libraries out there that do
this job well, such as [RtAudio](https://www.music.mcgill.ca/~gary/rtaudio/)
and [PortAudio](http://www.portaudio.com/), among others.  The advantage of this library is
that it is smaller, and provides its functionality in a different package
(COM), which some application programmers may prefer.  It is also free software.

DXAudio also does not attempt at providing the same set of functionality
that WASAPI does.  This library only deals with default audio endpoints.
Since this is all most games care about, there was no effort to include
the functionality to choose endpoints that are not the default.

DXAudio assumes an environment with no exclusive use of default audio endpoints.
If this occurs, the stream will be interrupted.  It is up to the application
to decide what to do in this situation.  This situation is unlikely to occur
in the first place, however, and when it does the user likely already expects
silence from other applications, games especially.

Usage
-------------
A stream of any type can be created in four simple steps.

#### 1. Create the stream description

`DXAUDIO_STREAM_DESC` is a structure with two variables: the sample rate of the stream, and the type of the stream.

    struct DXAUDIO_STREAM_DESC {
        FLOAT SampleRate;
        DXAUDIO_STREAM_TYPE Type;
    };

`DXAUDIO_STREAM_TYPE` is an enumeration with five members:

    enum DXAUDIO_STREAM_TYPE {
  	    DXAUDIO_STREAM_TYPE_OUTPUT = 1,
       	DXAUDIO_STREAM_TYPE_INPUT,
       	DXAUDIO_STREAM_TYPE_LOOPBACK,
       	DXAUDIO_STREAM_TYPE_DUPLEX,
       	DXAUDIO_STREAM_TYPE_ECHO
    };
    
`DXAUDIO_STREAM_TYPE_OUTPUT` refers to a stream that writes data to the default audio output endpoint. <br>
`DXAUDIO_STREAM_TYPE_INPUT` refers to a stream that reads data from the default audio input endpoint. <br>
`DXAUDIO_STREAM_TYPE_LOOPBACK` refers to a stream that reads the audio that's currently playing through the default
audio output endpoint. <br>
`DXAUDIO_STREAM_TYPE_DUPLEX` refers to a stream that both reads data from the default audio input endpoint and
writes data to the default audio output endpoint.<br>
`DXAUDIO_STREAM_TYPE_ECHO` refers to a stream that both reads the audio that's currently playing through the default
audio output endpoint and writes data to that same endpoint.

#### 2. Create the stream callback

There are three callback interfaces: `IDXAudioReadCallback`, `IDXAudioWriteCallback`, and `IDXAudioReadWriteCallback`.

`IDXAudioReadCallback` is used for input and loopback streams.<br>
`IDXAudioWriteCallback` is used for output streams.<br>
`IDXAudioReadWriteCallback` is used for duplex and echo streams.

You must implement one of these interfaces.  There are two methods in each interface:

    struct IDXAudioWriteCallback : public IDXAudioCallback {
        VOID OnObjectFailure(LPCWSTR File, UINT Line, HRESULT hr);
        VOID OnThreadInit();
        VOID OnProcess(FLOAT SampleRate, FLOAT* OutputBuffer, UINT Frames);
    };
    
The above `OnProcess()` method is for a write callback.  The other two callbacks have their own (very similar) versions of this method.  Both methods must be implemented by your child class.  As with all COM interfaces, `QueryInterface()`, `AddRef()`, and `Release()` must also be implemented.

`OnThreadInit()` is called when the stream thread is first created.  Since COM is initialized in apartment threaded mode
on the stream thread, any COM objects you wish to use must be initialized in this method with references kept in your
callback implementation or any of the objects associated with it.  This is also useful for initializing any audio
rendering stuff before `OnProcess()` is first called.

`OnObjectFailure()` is used for reporting that something went wrong.  The `HRESULT` received will be that which
was returned by WASAPI on one of its method calls.  It may also let you know that you've used the library correctly - it will provide an `E_INVALIDARG` `HRESULT` in this event.  The method also provides two other parameters: `File`, and `Line`.  These report the line of failure within the DLL source code.  If anything baffling happens, you've found a bug - luckily, it'll be easier to fix with this information in hand.

`OnProcess()` is the heart of the audio stream - it is called once every device period (~10ms).  This is where all
audio processing should occur.  `Frames` refers to the number of stereo samples to be read or generated per call.
This number is highly likely to change between calls, so you should not write your application to rely on a certain
buffer size.  This is primarily due to the fact that device periodicity is out of my hands, and constraining processing
to a constant buffer size would introduce additional latency.  This number also depends on the discrepancy between the
application's requested sample rate and that of the endpoint.  The format of the buffers is interleaved, meaning every two float values represents a pair of left and right channel samples, such that a sample at position `[i * 2]` is a left channel sample, and a sample at `[i * 2 + 1]` is a right channel sample.

Note that you should not call stream interface methods from within `OnProcess()`,
as this method is called on a separate thread - `IDXAudioStream` is not thread-safe.  COM is initialized in
apartment-threaded mode on the stream thread, and nothing is done within DXAudio to prevent race conditions on behalf of the application.

#### 3. Create the stream

A stream can be created through `DXAudioCreateStream()`:

    HRESULT DXAudioCreateStream (
        const DXAUDIO_STREAM_DESC* pDesc,
        IDXAudioCallback* pDXAudioCallback,
        IDXAudioStream** ppDXAudioStream
    );
    
The use of this function should be pretty self-explanatory.  This function does not handle nullptr exceptions, so make sure
the variables you hand it are valid.  This function will return `E_INVALIDARG` if the wrong type of callback interface is provided (checked via `QueryInterface()`, which you must implement correctly).

#### 4. Use the stream

The stream behaves according to the following state diagram:

![Stream State Diagram](https://github.com/AustinBorger/DXAudio/blob/master/UML/DXAudioStateDiagram.png)

The stream will automatically be stopped upon release of the COM object.

The `IDXAudioStream` interface is defined below:

    struct IDXAudioStream : public IUnknown {
    	VOID Start();
    	VOID Stop();
    	FLOAT GetSampleRate();
    	DXAUDIO_STREAM_TYPE GetStreamType();
    };
    
These methods should be pretty self-explanatory.

#### And that's it!

All you have to do to include DXAudio in your project is to download the "DXAudio.h" header and dll and link the library.
These builds will be kept up-to-date with the source code, so you don't have to build it if you don't want to.  Note that
only Visual Studio is supported for compilation and project files.

DXAudioResampler
-------------
DXAudio also exposes an interface for resampling audio to make better use of the code within it.  To use this functionality, include "DXAudioResampler.h" in your application.  The `IDXAudioResampler` interface is simple, having only one method:

    struct IDXAudioResampler : public IUnknown {
    	virtual VOID STDMETHODCALLTYPE Process (
    		FLOAT* InBuffer,
    		UINT InBufferFrames,
    		UINT* pInBufferFramesUsed,
    		FLOAT* OutBuffer,
    		UINT OutBufferFrames,
    		UINT* pOutBufferFramesGen,
    		DOUBLE Ratio
    	) PURE;
    };
    
`InBuffer` and `OutBuffer` are pointers to the input and output audio buffers, respectively, which the application must supply.  The buffer format is the same as used in the `OnProcess()` method.  `InBufferFrames` and `OutBufferFrames` are the number of stereo samples in the input and output buffers, respectively.  They are not necessarily the number of samples that will be used or generated.  `pInBufferFramesUsed` and `pOutBufferFramesGen` are used to determine the amount of data that was used and generated - these must not be `NULL`, otherwise a `nullptr` exception may occur.  Finally, `Ratio` is the ratio of the output sample rate to the input sample rate.  This cannot be greater than 256.

License
-------------
DXAudio is released under the GPLv3 license.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contributors
-------------
DXAudio uses Secret Rabbit Code for resampling.  This library was developed by
Erik de Castro Lopo, and is released under GPLv2.

New contributors are very welcome, as currently this code base is maintained by
just one person (me).  If DXAudio proves useful for you, but doesn't meet your
standards, please send feedback to my email at <aaborger@gmail.com>.
