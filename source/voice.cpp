
#include "../include/voice.h"

#include "base/source/fstreamer.h"

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
	if (!s.readDouble(rootNote))
		return kResultFalse;
	if (!s.readBool(bypass))
		return kResultFalse;
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
	if (!s.writeDouble(rootNote))
		return kResultFalse;
	if (!s.writeBool(bypass))
		return kResultFalse;
}

}
}