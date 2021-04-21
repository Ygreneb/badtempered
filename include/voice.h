#pragma once

#include "public.sdk/samples/vst/common/voicebase.h"
#include "pluginterfaces/base/ibstream.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Benergy {
namespace BadTempered {

using namespace Steinberg;
using ParamValue = Vst::ParamValue;

struct GlobalParameterState
{
	ParamValue volume;
	ParamValue rootNote;

	bool bypass;

	tresult setState(IBStream* stream);
	tresult getState(IBStream* stream);
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
		double tuningInHz = 440.0 * pow(2.0, (double)(pitch - 69.0) / 12.0);

		for (int i = 0; i < numSamples; ++i)
		{
			SamplePrecision val = sin(n / sampleRate * tuningInHz * 2.0 * M_PI);
			
			outputBuffers[0][i] += volume * val;
			outputBuffers[1][i] += volume * val;

			++n;

			//phase += 1.0 / sampleRate * tuningInHz * 2 * M_PI;
		}

		return volume > 0.01;
	}

	void noteOn(int32 pitch, ParamValue velocity, float tuning, int32 sampleOffset, int32 noteId) SMTG_OVERRIDE
	{
		volume = this->globalParameters->volume;

		Vst::VoiceBase<kNumParameters, SamplePrecision, 2, GlobalParameterState>::noteOn(pitch, velocity, tuning, sampleOffset, noteId);
	}

	void noteOff(ParamValue velocity, int32 sampleOffset) SMTG_OVERRIDE
	{
		Vst::VoiceBase<kNumParameters, SamplePrecision, 2, GlobalParameterState>::noteOff(velocity, sampleOffset);

		volume = 0;
	}

	//virtual void reset()
	//{
	//	noteOnSampleOffset = -1;
	//	noteOffSampleOffset = -1;
	//	noteId = -1;
	//	tuning = 0;
	//}

private:
	//double phase = 0.0;
	ParamValue volume = 0.0;
	uint32 n = 0;
};

}
}