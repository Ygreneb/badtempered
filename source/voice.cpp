
#include "../include/voice.h"
#include "../include/plugids.h"

#include "base/source/fstreamer.h"

#include <tuple>

namespace Benergy {
namespace BadTempered {

static uint64 currentParameterStateVersion = 0;

tresult GlobalParameterState::setState(IBStream* stream)
{
	if (!stream)
		return kResultFalse;

	IBStreamer s (stream, kLittleEndian);
	uint64 version = 0;

	if (!s.readInt64u(version))
		return kResultFalse;
	if (!s.readDouble(volume))
		return kResultFalse;
	if (!s.readDouble(tuning))
		return kResultFalse;
	if (!s.readDouble(rootNote))
		return kResultFalse;
	if (!s.readBool(bypass))
		return kResultFalse;

	if (!s.readDouble(sinusVolume))
		return kResultFalse;
	if (!s.readDouble(squareVolume))
		return kResultFalse;
	if (!s.readDouble(attack))
		return kResultFalse;
	if (!s.readDouble(decay))
		return kResultFalse;
	if (!s.readDouble(sustain))
		return kResultFalse;
	if (!s.readDouble(release))
		return kResultFalse;

	return kResultTrue;
}

tresult GlobalParameterState::getState(IBStream* stream)
{
	if (!stream)
		return kResultFalse;

	IBStreamer s(stream, kLittleEndian);

	if (!s.writeInt64u(currentParameterStateVersion))
		return kResultFalse;
	if (!s.writeDouble(volume))
		return kResultFalse;
	if (!s.writeDouble(tuning))
		return kResultFalse;
	if (!s.writeDouble(rootNote))
		return kResultFalse;
	if (!s.writeBool(bypass))
		return kResultFalse;

	if (!s.writeDouble(sinusVolume))
		return kResultFalse;
	if (!s.writeDouble(squareVolume))
		return kResultFalse;
	if (!s.writeDouble(attack))
		return kResultFalse;
	if (!s.writeDouble(decay))
		return kResultFalse;
	if (!s.writeDouble(sustain))
		return kResultFalse;
	if (!s.writeDouble(release))
		return kResultFalse;

	return kResultTrue;
}

std::tuple<ParamValue, ParamValue, ParamValue> GlobalParameterState::getMinMaxDefaultForParam(int paramID)
{
	switch (paramID)
	{
	case kVolumeId:
		return std::make_tuple(-60.0, 60.0, 0.0);
	case kAttackId:
	case kDecayId:
	case kReleaseId:
		return std::make_tuple(3.0, 10000.0, 10.0);
	}

	return std::make_tuple(0.0, 1.0, 0.0);
}

ParamValue GlobalParameterState::paramToPlain(ParamValue normalized, int paramID)
{
	std::tuple<ParamValue, ParamValue, ParamValue> minMaxDefault = getMinMaxDefaultForParam(paramID);
	return normalized * (std::get<1>(minMaxDefault) - std::get<0>(minMaxDefault)) + std::get<0>(minMaxDefault);
}

double VoiceStatics::pythagoreanOffsets[12];
double VoiceStatics::werckmeisterIIIOffsets[12];

double VoiceStatics::getPythagoreanOffset(int32 pitch, int32 rootPitch)
{
	int32 pitchMod = (pitch - rootPitch) % 12;
	//double octaves = trunc((double)pitch - rootPitch);
	return pythagoreanOffsets[pitchMod];// +octaves * 1200.0;
}

double VoiceStatics::getWerckmeisterIIIOffset(int32 pitch, int32 rootPitch)
{
	int32 pitchMod = (pitch - rootPitch) % 12;
	//double octaves = trunc((double)pitch - rootPitch);
	return werckmeisterIIIOffsets[pitchMod];// +octaves * 1200.0;
}

class VoiceStaticsOnce
{
public:
	VoiceStaticsOnce()
	{
		static const double pComma = 1200.0 * log2(pow(1.5, 12.0) / pow(2.0, 7.0)); // in Cents

		//VoiceStatics::pythagoreanOffsets[0] = 0.0;																// unison
		//VoiceStatics::pythagoreanOffsets[1] = 1200.0 * log2( pow(3.0 / 2.0, -5.0) * pow(2.0,  3.0) ) - 100.0;	// minor second
		//VoiceStatics::pythagoreanOffsets[2] = 1200.0 * log2( pow(3.0 / 2.0,  2.0) * 0.5			   ) - 200.0;	// major second
		//VoiceStatics::pythagoreanOffsets[3] = 1200.0 * log2( pow(3.0 / 2.0, -3.0) * 4.0            ) - 300.0;	// minor third
		//VoiceStatics::pythagoreanOffsets[4] = 1200.0 * log2( pow(3.0 / 2.0,  4.0) * 1.0 / 4.0	   ) - 400.0;	// major third
		//VoiceStatics::pythagoreanOffsets[5] = 1200.0 * log2( 4.0 / 3.0							   ) - 500.0;	// fourth
		//VoiceStatics::pythagoreanOffsets[6] = 0.0;																// augmented fourth / diminished fifth => undefined
		//VoiceStatics::pythagoreanOffsets[7] = 1200.0 * log2( 3.0 / 2.0							   ) - 700.0;	// fifth
		//VoiceStatics::pythagoreanOffsets[8] = 1200.0 * log2( pow(3.0 / 2.0, -4.0) * pow(2.0,  3.0) ) - 800.0;	// minor sixth
		//VoiceStatics::pythagoreanOffsets[9] = 1200.0 * log2( pow(3.0 / 2.0,  3.0) * 0.5			   ) - 900.0;	// major sixth
		//VoiceStatics::pythagoreanOffsets[9] = 1200.0 * log2( pow(3.0 / 2.0, -2.0) * 4.0			   ) - 1000.0;	// minor seventh
		//VoiceStatics::pythagoreanOffsets[9] = 1200.0 * log2( pow(3.0 / 2.0,  5.0) * 1.0 / 4.0      ) - 1100.0;	// major seventh

		for (int i = 0; i < 12; ++i)
		{
			const int numFifths = (i * 7 + 5) % 12 - 5; // in range [-5:6]
			const int numOctaves = (i * 3 + 3) % 7 - 3; // in range [-3:3]
			VoiceStatics::pythagoreanOffsets[i] = 1200.0 * log2(pow(1.5, numFifths) * pow(2.0, numOctaves)) - i * 100.0;
			VoiceStatics::werckmeisterIIIOffsets[i] = VoiceStatics::pythagoreanOffsets[i];
			if (numFifths >= 1 && numFifths <= 3)
				VoiceStatics::werckmeisterIIIOffsets[i] -= 0.25 * pComma * numFifths;
			else if (numFifths == 6)
				VoiceStatics::werckmeisterIIIOffsets[i] -= pComma;
		}
	}
};

static VoiceStaticsOnce _gVoiceStaticsOnce;

}
}