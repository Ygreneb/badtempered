//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : plugcontroller.cpp
// Created by  : Steinberg, 01/2018
// Description : HelloWorld Example for VST 3
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2020, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "../include/plugcontroller.h"
#include "../include/plugids.h"
#include "../include/voice.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"

using namespace VSTGUI;

namespace Benergy {
namespace BadTempered {

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugController::initialize (FUnknown* context)
{
	tresult result = EditController::initialize (context);
	if (result == kResultTrue)
	{
		Vst::Parameter* param;

		//---Create Parameters------------
		parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
		                         Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
		                         BadTemperedParams::kBypassId);

		auto range = GlobalParameterState::getMinMaxDefaultForParam(kVolumeId);
		param = new Vst::RangeParameter(L"Volume", kVolumeId, L"dB", std::get<0>(range), std::get<1>(range), std::get<2>(range), 0, Vst::ParameterInfo::kCanAutomate, 0, L"Vol");
		param->setPrecision(2);
		parameters.addParameter(param);

		auto listParam = new Vst::StringListParameter(L"Tuning", kTuningId, nullptr, Vst::ParameterInfo::kIsList, 0, L"Tun");
		listParam->appendString(L"Equal Step");
		listParam->appendString(L"Pythagorean");
		listParam->appendString(L"Werckmeister III");
		listParam->appendString(L"Meantone 1/4 comma");
		parameters.addParameter(listParam);

		param = new Vst::Parameter(L"RootNote", kRootNoteId, nullptr, 0.0, 12, Vst::ParameterInfo::kNoFlags, 0, L"Root");
		param->setPrecision(3);
		parameters.addParameter(param);

		range = GlobalParameterState::getMinMaxDefaultForParam(kAttackId);
		param = new Vst::RangeParameter(L"Attack", kAttackId, L"ms", std::get<0>(range), std::get<1>(range), std::get<2>(range), 0, Vst::ParameterInfo::kCanAutomate, 0, L"Atk");
		param->setPrecision(1);
		parameters.addParameter(param);

		range = GlobalParameterState::getMinMaxDefaultForParam(kDecayId);
		param = new Vst::RangeParameter(L"Decay", kDecayId, L"ms", std::get<0>(range), std::get<1>(range), std::get<2>(range), 0, Vst::ParameterInfo::kCanAutomate, 0, L"Dcy");
		param->setPrecision(1);
		parameters.addParameter(param);

		param = new Vst::Parameter(L"Sustain", kSustainId, nullptr, 1.0, 0, Vst::ParameterInfo::kCanAutomate, 0, L"Sus");
		param->setPrecision(2);
		parameters.addParameter(param);

		range = GlobalParameterState::getMinMaxDefaultForParam(kReleaseId);
		param = new Vst::RangeParameter(L"Release", kReleaseId, L"ms", std::get<0>(range), std::get<1>(range), std::get<2>(range), 0, Vst::ParameterInfo::kCanAutomate, 0, L"Rls");
		param->setPrecision(1);
		parameters.addParameter(param);

		param = new Vst::Parameter(L"Sinus Volume", kSinusVolumeId, nullptr, 0.0, 0, Vst::ParameterInfo::kCanAutomate, 0, L"SinVol");
		param->setPrecision(2);
		parameters.addParameter(param);
		
		param = new Vst::Parameter(L"Square Volume", kSquareVolumeId, nullptr, 0.0, 0, Vst::ParameterInfo::kCanAutomate, 0, L"SqrVol");
		param->setPrecision(2);
		parameters.addParameter(param);

		param = new Vst::Parameter(L"Sawtooth Volume", kSawVolumeId, nullptr, 0.0, 0, Vst::ParameterInfo::kCanAutomate, 0, L"SawVol");
		param->setPrecision(2);
		parameters.addParameter(param);

		param = new Vst::Parameter(L"Triangle Volume", kTriVolumeId, nullptr, 0.0, 0, Vst::ParameterInfo::kCanAutomate, 0, L"TriVol");
		param->setPrecision(2);
		parameters.addParameter(param);
	}
	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API PlugController::createView (const char* name)
{
	// someone wants my editor
	if (name && strcmp (name, "editor") == 0)
	{
		auto* view = new VST3Editor (this, "view", "plug.uidesc");
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugController::setComponentState (IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read our parameters and bypass value...
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	GlobalParameterState gps;
	tresult res = gps.setState(state);

	if (res == kResultTrue)
	{
		setParamNormalized(kBypassId, gps.bypass);
		setParamNormalized(kVolumeId, gps.volume);
		setParamNormalized(kTuningId, gps.tuning);
		setParamNormalized(kRootNoteId, gps.rootNote);

		setParamNormalized(kAttackId, gps.attack);
		setParamNormalized(kDecayId, gps.decay);
		setParamNormalized(kSustainId, gps.sustain);
		setParamNormalized(kReleaseId, gps.release);

		setParamNormalized(kSinusVolumeId, gps.sinusVolume);
		setParamNormalized(kSquareVolumeId, gps.squareVolume);
		setParamNormalized(kSawVolumeId, gps.sawVolume);
		setParamNormalized(kTriVolumeId, gps.triVolume);
	}

	return res;
}

//------------------------------------------------------------------------
} // namespace
} // namespace Benergy
