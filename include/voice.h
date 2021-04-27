#pragma once

#include "public.sdk/samples/vst/common/voicebase.h"
#include "pluginterfaces/base/ibstream.h"

//#define _USE_MATH_DEFINES
//#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

namespace Benergy {
namespace BadTempered {

using namespace Steinberg;
using ParamValue = Vst::ParamValue;

struct GlobalParameterState
{
	ParamValue volume;
	ParamValue tuning;
	ParamValue rootNote;

	ParamValue sinusVolume;
	ParamValue squareVolume;
	ParamValue attack;
	ParamValue decay;
	ParamValue sustain;
	ParamValue release;

	bool bypass;

	tresult setState(IBStream* stream);
	tresult getState(IBStream* stream);
};

enum VoiceParameters
{
	kNumParameters = 1
};

//class VoiceStaticsOnce;

class VoiceStatics
{
	friend class VoiceStaticsOnce;

public:
	static double getPythagoreanOffset(int32 pitch, int32 rootPitch);
	static double getWerckmeisterIIIOffset(int32 pitch, int32 rootPitch);

private:
	static double pythagoreanOffsets[12]; // in Cents
	static double werckmeisterIIIOffsets[12]; // in Cents
};

template<class SamplePrecision>
class Voice : public Vst::VoiceBase<kNumParameters, SamplePrecision, 2, GlobalParameterState>
{
public:
	bool process(SamplePrecision* outputBuffers[2], int32 numSamples);
	void noteOn(int32 pitch, ParamValue velocity, float tuning, int32 sampleOffset, int32 noteId) SMTG_OVERRIDE;
	void noteOff(ParamValue velocity, int32 sampleOffset) SMTG_OVERRIDE;

private:
	//double phase = 0.0;
	ParamValue volume = 0.0;
	uint32 n = 0;

	double freq = 0.0;
};

template<class SamplePrecision>
bool Voice<SamplePrecision>::process(SamplePrecision* outputBuffers[2], int32 numSamples)
{
	//globalParameters->tuning;
	//double tuningInHz = 440.0 * pow(2.0, (double)(pitch - 69.0) / 12.0);

	for (int i = 0; i < numSamples; ++i)
	{
		SamplePrecision val = sin(n / sampleRate * freq * 2.0 * M_PI);
			
		outputBuffers[0][i] += volume * val;
		outputBuffers[1][i] += volume * val;

		++n;

		//phase += 1.0 / sampleRate * tuningInHz * 2 * M_PI;
	}

	return volume > 0.01;
}

template<class SamplePrecision>
void Voice<SamplePrecision>::noteOn(int32 pitch, ParamValue velocity, float tuning, int32 sampleOffset, int32 noteId)
{
	volume = globalParameters->volume;

	freq = 440.0 * pow(2.0, (pitch - 69.0) / 12.0); // Equal step tuning based on pitch

	if (globalParameters->tuning > 0.33) // Not equal step tuning, frequency needs update
	{
		//int32 rootNotePitch = 60 + round(globalParameters->rootNote * 11.0); // 60 = MIDI pitch of Middle C
		int32 rootNotePitch = globalParameters->rootNote; // 60 = MIDI pitch of Middle C
		//double rootNoteFreq = 440.0 * pow(2.0, (rootNotePitch - 69) / 12.0); // 69 = MIDI pitch of A = 440 Hz
		double offsetCents = 0.0;

		if (globalParameters->tuning < 0.66)
			offsetCents = VoiceStatics::getPythagoreanOffset(pitch, rootNotePitch);
		else
			offsetCents = VoiceStatics::getWerckmeisterIIIOffset(pitch, rootNotePitch);

		freq *= pow(2.0, offsetCents / 1200.0);
	}

	Vst::VoiceBase<kNumParameters, SamplePrecision, 2, GlobalParameterState>::noteOn(pitch, velocity, tuning, sampleOffset, noteId);
}

//template<class SamplePrecision>
//void Voice<SamplePrecision>::noteOn(int32 pitch, ParamValue velocity, float tuning, int32 sampleOffset, int32 noteId)
//{
//	volume = globalParameters->volume;
//
//	freq = 440.0 * pow(2.0, (pitch - 69.0) / 12.0); // Equal tuning based on pitch
//	
//	//freq = freq * pow(2.0, tuning / 1200.0); // Update frequency based on MIDI event tuning (1.f = 1 Cent)
//	
//	if (globalParameters->tuning > 0.33) // Not equal step tuning, frequency needs update
//	{
//		int32 rootNotePitch = 60 + round(globalParameters->rootNote * 11.0); // 60 = MIDI pitch of Middle C
//		double rootNoteFreq = 440.0 * pow(2.0, (rootNotePitch - 69) / 12.0); // 69 = MIDI pitch of A = 440 Hz
//					
//		//if (pitchDelta != 0)
//		//{
//		//int32 numOfOctavesBtwn = trunc(pitchDelta / 12.0);
//
//		// Transpose pitch and find integer number of fiths and octaves between pitch and root note
//		int32 pitchDelta = pitch - rootNotePitch;
//		int32 numOctavesBtwn;
//		int32 numFifthsBtwn;
//
//		for (int32 i = -6; i <= 6; ++i)
//		{
//			double numOctavesFlt;
//			if (abs(modf((7.0 * i - pitchDelta) / 12.0, &numOctavesFlt)) < 0.01)
//			{
//				numFifthsBtwn = i;
//				numOctavesBtwn = numOctavesFlt;
//				break;
//			}
//		}
//
//		//if (globalParameters->tuning < 0.66)
//		//{
//		// Pythagorean tuning
//		if (globalParameters->tuning < 0.66 && abs(numFifthsBtwn) == 6)
//		{
//			// This note is not playable in pythagorean tuning because not uniquely identifiable
//			freq = -1.0; 
//			return;
//		}
//
//		freq = rootNoteFreq * pow(3.0 / 2.0, numFifthsBtwn) * pow(2.0, -numOctavesBtwn);
//		//}
//		//else
//
//		if (globalParameters->tuning > 0.66)
//		{
//			// Werckmeister III tuning
//			// based on Pythagorean Tuning
//			double centOffset = 0.0;
//			if (numFifthsBtwn > 0)
//			{
//				static const double pythagoreanComma = 1200.0 * log2(pow(3.0 / 2.0, 12.0) / pow(2.0, 7.0));
//				if (numFifthsBtwn <= 3)
//				{
//					centOffset = numFifthsBtwn * 0.25 * pythagoreanComma;
//				}
//				else if (numFifthsBtwn >= 6)
//				{
//					centOffset = pythagoreanComma;
//				}
//				else
//				{
//					centOffset = 3 * 0.25 * pythagoreanComma;
//				}
//			}
//
//			if (centOffset != 0.0)
//			{
//				freq *= pow(2.0, centOffset / 1200.0);
//			}
//		}
//		//}
//
//			
//	
//	}
//
//	Vst::VoiceBase<kNumParameters, SamplePrecision, 2, GlobalParameterState>::noteOn(pitch, velocity, tuning, sampleOffset, noteId);
//}

template<class SamplePrecision>
void Voice<SamplePrecision>::noteOff(ParamValue velocity, int32 sampleOffset)
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



}
}