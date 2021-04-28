#pragma once

#include "public.sdk/samples/vst/common/voicebase.h"
#include "pluginterfaces/base/ibstream.h"


//#define _USE_MATH_DEFINES
//#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifndef M_PI_MUL_2
#define M_PI_MUL_2 6.283185307179586476925286766559
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

	static std::tuple<ParamValue, ParamValue, ParamValue> getMinMaxDefaultForParam(int paramID);
	static ParamValue paramToPlain(ParamValue normalized, int paramID);
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
	void reset() SMTG_OVERRIDE;

private:
	inline ParamValue dBToFactor(ParamValue val_dB)
	{
		return pow(10, val_dB / 20.0);
	}

	ParamValue frequency = 0.0;
	ParamValue volume = 0.0;
	//ParamValue rampTime = 0.0;
	ParamValue rampMultiplier = 0.0;

	ParamValue currentVol = 0.0;

	//ParamValue currentSinusPhase = 0.0;
	//ParamValue currentSinusVolume = 0.0;
	//ParamValue currentSinusFreq = 0.0;

	uint32 n = 0;
	bool pastAttack = false;
	bool noteOffReceived = false;

};

template<class SamplePrecision>
bool Voice<SamplePrecision>::process(SamplePrecision* outputBuffers[2], int32 numSamples)
{
	//ParamValue sinusFreq = frequency;
	//if (currentSinusFreq != sinusFreq)
	//{
	//	sinusFreq = (sinusFreq - currentSinusFreq) / 20.0 + currentSinusFreq; // Takes 20 calls of process() to adjust to target frequency
	//	currentSinusPhase += (currentSinusFreq - sinusFreq);
	//	currentSinusFreq = sinusFreq;
	//}

	// Update ramptime and volume after initial attack
	if (!noteOffReceived && !pastAttack && n >= sampleRate * GlobalParameterState::paramToPlain(globalParameters->attack, kAttackId) * 0.001)
	{
		volume = globalParameters->sustain * dBToFactor(GlobalParameterState::paramToPlain(globalParameters->volume, kVolumeId));
		ParamValue rampTime = GlobalParameterState::paramToPlain(globalParameters->decay, kDecayId) * 0.001; // in s
		rampMultiplier = (log(volume) - log(currentVol)) / (rampTime * sampleRate);
		pastAttack = true;
	}

	//ParamValue vol = volume;
	if (abs(currentVol - volume) > 0.0001)
	{
		// TODO adjust volume not per block but per sample
		//currentVol += ((volume - currentVol) / rampTime) * ((ParamValue)numSamples / sampleRate);
		currentVol += rampMultiplier * numSamples * currentVol;
	}

	if (pastAttack && currentVol < 0.0002)
	{
		// No volume, return false for voice processor to reset voice
		return false;
	}

	for (int i = 0; i < numSamples; ++i)
	{
		//SamplePrecision val = sin(n / sampleRate * currentSinusFreq * M_PI_MUL_2 + currentSinusPhase);
		SamplePrecision sample = sin((double)n / sampleRate * frequency * M_PI_MUL_2);
			
		outputBuffers[0][i] += currentVol * sample;
		outputBuffers[1][i] += currentVol * sample;

		++n;

		//phase += 1.0 / sampleRate * tuningInHz * 2 * M_PI;
	}

	return true;
}

template<class SamplePrecision>
void Voice<SamplePrecision>::noteOn(int32 pitch, ParamValue velocity, float tuning, int32 sampleOffset, int32 noteId)
{
	volume = GlobalParameterState::paramToPlain(globalParameters->volume, kVolumeId);
	volume = dBToFactor(volume);
	ParamValue rampTime = GlobalParameterState::paramToPlain(globalParameters->attack, kAttackId) * 0.001;
	rampMultiplier = (log(volume) - log(currentVol)) / (rampTime * sampleRate);
	
	frequency = 440.0 * pow(2.0, (pitch - 69.0) / 12.0); // Equal step tuning based on pitch

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

		frequency *= pow(2.0, offsetCents / 1200.0);
	}

	pastAttack = false;
	noteOffReceived = false;

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

	//volume = -0.05; // This is needed to get currentVol < 0 and trigger an reset
	volume = 0.0001;
	ParamValue rampTime = GlobalParameterState::paramToPlain(globalParameters->release, kReleaseId) * 0.001;
	rampMultiplier = (log(volume) - log(currentVol)) / (rampTime * sampleRate);

	noteOffReceived = true;
}

template<class SamplePrecision>
void Voice<SamplePrecision>::reset()
{
	Vst::VoiceBase<kNumParameters, SamplePrecision, 2, GlobalParameterState>::reset();
	n = 0;
	currentVol = 0.0001;
	//pastAttack = false;
	//noteOffReceived = false;
}



}
}