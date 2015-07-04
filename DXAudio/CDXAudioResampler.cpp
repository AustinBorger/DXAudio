#include "CDXAudioResampler.h"

CDXAudioResampler::CDXAudioResampler() :
m_RefCount(1),
m_SrcState(nullptr)
{ }

CDXAudioResampler::~CDXAudioResampler() {
	if (m_SrcState != nullptr) {
		src_delete(m_SrcState);
		m_SrcState = nullptr;
	}
}

HRESULT CDXAudioResampler::Initialize() {
	int error = 0;

	m_SrcState = src_new(SRC_SINC_FASTEST, 2, &error);

	if (m_SrcState == nullptr) {
		return E_FAIL;
	}

	return S_OK;
}

VOID CDXAudioResampler::Process (
	FLOAT* InBuffer,
	UINT InBufferFrames,
	FLOAT* OutBuffer,
	UINT OutBufferFrames,
	DOUBLE Ratio
) {
	int error = 0;
	SRC_DATA SrcData;

	SrcData.data_in = InBuffer;
	SrcData.data_out = OutBuffer;
	SrcData.end_of_input = 0;
	SrcData.input_frames = InBufferFrames;
	SrcData.input_frames_used = 0;
	SrcData.output_frames = OutBufferFrames;
	SrcData.output_frames_gen = 0;
	SrcData.src_ratio = Ratio;

	error = src_process(m_SrcState, &SrcData);
}