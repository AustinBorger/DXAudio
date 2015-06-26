DXAudio
=========

DXAudio is a small layer over WASAPI written in C++ that
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

`IDXReadCallback` is used for input and loopback streams.<br>
`IDXWriteCallback` is used for output streams.<br>
`IDXReadWriteCallback` is used for duplex and echo streams.

You must implement one of these interfaces.  There are two methods in each interface:

    virtual VOID STDMETHODCALLTYPE OnObjectFailure(HRESULT hr) PURE;
    virtual VOID STDMETHODCALLTYPE Process(FLOAT SampleRate, FLOAT* OutputBuffer, UINT Frames) PURE;

The above `Process()` method is for a write callback.  The other two callbacks have their own (very similar) versions of this
method.  Both methods must be implemented by your child class.  As with all COM interfaces, `QueryInterface()`, `AddRef()`,
and `Release()` must also be implemented.

`OnObjectFailure()` is used for reporting that something went wrong.  The HRESULT received will be that which
was returned by WASAPI on one of its method calls.  It will not let you know that you've used the library incorrectly, it is
solely for the purpose of dealing with uncertainty in the audio environment.

`Process()` is the heart of the audio stream - it is called once every device period (~10ms).  This is where all
audio processing should occur.  `Frames` refers to the number of stereo samples to be read or generated per call.
This number is highly likely to change between calls, so you should not write your application to rely on a certain
buffer size.  This is primarily due to the fact that device periodicity is out of my hands, and constraining processing
to a constant buffer size would introduce additional latency.  This number also depends on the discrepancy between the
application's requested sample rate and that of the endpoint.

Note that you should not call stream interface methods from within `Process()`,
as it occurs on a separate thread and the `IDXAudioStream` interface is not thread-safe.  COM is initialized in
apartment-threaded mode, and nothing is done within DXAudio to prevent race conditions on behalf of the application.

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
