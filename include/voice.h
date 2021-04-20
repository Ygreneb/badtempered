#pragma once

#include "public.sdk/samples/vst/common/voicebase.h"

namespace Benergy {
namespace BadTempered {

using namespace Steinberg;

struct GlobalParameterState
{
	Vst::ParamValue volume;
};

enum VoiceParameters
{
	kVolume,

	kNumParameters
};

template<class SamplePrecision>
class Voice : public Vst::VoiceBase<kNumParameters, SamplePrecision, 2, GlobalParameterState>
{
public:
	bool process(SamplePrecision* outputBuffers[2], int32 numSamples)
	{
		for (int c = 0; c < 2; ++c)
		{
			for (int i = 0; i < numSamples; ++i)
			{
				outputBuffers[c][i] = 0.0;
			}
		}
		return true;
	}
};

}
}